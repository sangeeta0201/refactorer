/*
Writing posit applications manually which have nice built in floating point version, could be time and resources consuming.
This clang pass uses AST matcher and libtool to automatically rewrite floating point applications to use psot type instead.
 
AST matcher matches 
1. Variable declaration - 
	double x, y; => posit32_t x, y;
2. Variable definition:
	double x = 2.3; => posit32_t = convertDoubletoP32(2.3);
3. Floating point add,sub, mul,div
	double x = y + z; => pisit32_t x = p32_add(y,z);
4. It also finds opportunity to use quire automatically
	double x = y*z+k => quire x = q16_fdp_add(y,z,k)
*/

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>

using namespace std;
using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

#define DoubleSize 6
std::string PositTY = "posit32_t ";
std::stringstream SSBefore;
//track temp variables
unsigned tmpCount = 0;

std::map<const BinaryOperator*, std::string> BinOp_Temp; 
std::stack<const BinaryOperator*> BOStack; 
SmallVector<unsigned, 8> ProcessedVD;
SmallVector<const BinaryOperator*, 8> ProcessedBO;

static llvm::cl::OptionCategory MatcherSampleCategory("Matcher Sample");

unsigned getLocationOffsetAndFileID(SourceLocation Loc,                                                                               
                                                  FileID &FID,
                                                  SourceManager *SrcManager)
{
  assert(Loc.isValid() && "Invalid location");
  std::pair<FileID,unsigned> V = SrcManager->getDecomposedLoc(Loc);
  FID = V.first;
  return V.second;
}

///TODO:
/*
int getOffsetUntil(const char *Buf, char Symbol)
{
  int Offset = 0;
  while (*Buf != Symbol) {
    Buf++;
    if (*Buf == '\0' )
      break;
		if( *Buf == '*'){
    	Offset++;
      break;
		}
    Offset++;
  }
  return Offset;
}
*/
int getOffsetUntil(const char *Buf, char Symbol)
{
  int Offset = 0;
  while (*Buf != Symbol) {
    Buf++;
    Offset++;
  }
  return Offset;
}


class FloatVarDeclHandler : public MatchFinder::MatchCallback {
public:
	unsigned getLineNo(SourceLocation StmtStartLoc){
		SourceManager &SM = Rewrite.getSourceMgr();
		if (StmtStartLoc.isMacroID()) {
    	StmtStartLoc = SM.getFileLoc(StmtStartLoc);
  	}
  
  	FileID FID;
  	unsigned StartOffset = 
    	getLocationOffsetAndFileID(StmtStartLoc, FID, &SM);
  
  	StringRef MB = SM.getBufferData(FID);
  
  	unsigned lineNo = SM.getLineNumber(FID, StartOffset) - 1;
  
  	return lineNo;
	}

	std::string getStmtIndentString(SourceLocation StmtStartLoc){
		SourceManager &SM = Rewrite.getSourceMgr();
		if (StmtStartLoc.isMacroID()) {
    	StmtStartLoc = SM.getFileLoc(StmtStartLoc);
  	}
  
  	FileID FID;
  	unsigned StartOffset = 
    	getLocationOffsetAndFileID(StmtStartLoc, FID, &SM);
  
  	StringRef MB = SM.getBufferData(FID);
  	unsigned lineNo = SM.getLineNumber(FID, StartOffset) - 1;
  	const SrcMgr::ContentCache *
      	Content = SM.getSLocEntry(FID).getFile().getContentCache();
  	unsigned lineOffs = Content->SourceLineCache[lineNo];
	// Find the whitespace at the start of the line.
  	StringRef indentSpace;
  
  	unsigned I = lineOffs;
  	while (isspace(MB[I]))
   	 ++I;
  	indentSpace = MB.substr(lineOffs, I-lineOffs);
  
  	return indentSpace;
	}
	//In: double arr[10][5];
	//Out: [10][5];
	char* getArrayDim(const VarDecl *VD){
  	int Offset = 0;
  	int StartOffset = 0;
		SourceLocation StartLoc =  VD->getSourceRange().getBegin();
		SourceManager &SM = Rewrite.getSourceMgr();
		const char *OrigBuf = SM.getCharacterData(StartLoc);
		const char *StartBuf = SM.getCharacterData(StartLoc);
		
    while (*StartBuf != '[' ){
    	StartBuf++;
			StartOffset++;
		}
  	while (*StartBuf != ';') {
  		if(*StartBuf == ',') 
				break;
    	StartBuf++;
			Offset++;
		}
		
		char *result = (char *)malloc(Offset+2);
		
		strncpy(result, OrigBuf+StartOffset, Offset);
		result[Offset] = ';'; 
		result[Offset+1] = '\0'; 
		llvm::errs()<<"result:"<<result<<"\n";
		return result;
	}

	void removeLine(SourceLocation StartLoc){
    int Offset = 0;
    int StartOffset = 0;
    SourceManager &SM = Rewrite.getSourceMgr();
    const char *StartBuf = SM.getCharacterData(StartLoc);
		unsigned lineNo = getLineNo(StartLoc);
    //is this the last vardecl in stmt? if yes, then removeLine the statement
    while (*StartBuf != ';') {
      StartBuf++;
      Offset++;
    }
		SmallVector<unsigned, 8>::iterator it;
		it = std::find(ProcessedVD.begin(), ProcessedVD.end(), lineNo);		
		if(it == ProcessedVD.end()){
			Rewriter::RewriteOptions Opts;
			Opts.RemoveLineIfEmpty = true;
    	const char *StartBuf1 = SM.getCharacterData(StartLoc.getLocWithOffset(StartOffset));
			llvm::errs()<<"Offset:"<<Offset<<"\n";
			Rewrite.RemoveText(StartLoc.getLocWithOffset(StartOffset), Offset+1, Opts); 
			//Rewrite.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
			ProcessedVD.push_back(lineNo);
		}
	}

	//returns conversion function for int type to posit
	std::string convertIntToPosit(const IntegerLiteral *IL){
		llvm::APInt intval = IL->getValue();
		llvm::SmallVector<char, 32> string;
		intval.toStringUnsigned(string);
		std::stringstream SSBefore;
		for(int i = 0;i<string.size();i++)
			SSBefore <<string[i];
		std::string convert = " = convertDoubleToP32(" + SSBefore.str()+");";
		return convert ;
	}

	//returns conversion function for floating type to posit
	std::string convertFloatToPosit(const FloatingLiteral *FL){
		llvm::APFloat floatval = FL->getValue();
		llvm::SmallVector<char, 32> string;
		floatval.toString(string, 32, 0);
		std::stringstream SSBefore;
		for(int i = 0;i<string.size();i++)
			SSBefore <<string[i];
		std::string convert = " = convertDoubleToP32(" + SSBefore.str()+");";
		return convert ;
	}

	//This function returns temp variables
	std::string getTempDest(){
		tmpCount++;
		return "tmp"+std::to_string(tmpCount);
	}

	std::string getPositFuncName(unsigned Opcode){
		string funcName;
		switch(Opcode){
			case 2:
				funcName = "p32_mul";
				break;
			case 3:
				funcName = "p32_div";
				break;
			case 5:
				funcName = "p32_add";
				break;
			case 6:
				funcName = "p32_sub";
				break;
			case 21:
				funcName = "p32_sub";
				break;
			default:
				assert("This opcode is not handled!!!");
		}
		return funcName;
	}

	//func(double x) => func(posit32_t x)
	void ReplaceParmVDWithPosit(const VarDecl *VD, char positLiteral){
		SourceLocation StartLoc =  VD->getSourceRange().getBegin();
		if(!StartLoc.isValid())
			return;
		SourceManager &SM = Rewrite.getSourceMgr();
		const char *StartBuf = SM.getCharacterData(StartLoc);
		int Offset = getOffsetUntil(StartBuf, positLiteral);
		Rewrite.ReplaceText(SourceRange(StartLoc, StartLoc.getLocWithOffset(Offset-1)), 
													PositTY);	
	}

	//double x => posit32_t x
	void ReplaceVDWithPosit(SourceLocation StartLoc, std::string positLiteral){
		if(!StartLoc.isValid())
			return;
		SourceManager &SM = Rewrite.getSourceMgr();
//		Rewrite.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
		const char *StartBuf = SM.getCharacterData(StartLoc);
		int Offset = getOffsetUntil(StartBuf, ';');
		std::string IndentStr = getStmtIndentString(StartLoc);
		Rewrite.InsertTextAfterToken(StartLoc.getLocWithOffset(Offset), 
					"\n"+IndentStr+PositTY+positLiteral);
	
		removeLine(StartLoc);
	}
//	x = y * 0.3 => t1 = convertdoubletoposit(0.3)
	void ReplaceBOLiteralWithPosit(const BinaryOperator *BO, std::string lhs, std::string rhs){
		SourceLocation StartLoc =  BO->getSourceRange().getBegin();
		if(!StartLoc.isValid())
			return;
		SourceManager &SM = Rewrite.getSourceMgr();
		const char *StartBuf = SM.getCharacterData(StartLoc);
		int Offset = getOffsetUntil(StartBuf, ';');
		SourceLocation StartLoc1 = BO->getSourceRange().getBegin();
		std::string IndentStr = getStmtIndentString(StartLoc1);
		Rewrite.InsertTextAfterToken(StartLoc.getLocWithOffset(Offset), 
					"\n"+IndentStr+PositTY+lhs +rhs);		
	}

	//x = y * 0.3 => x = posit_mul(y, t1);
	//removes the line
	void ReplaceBOEqWithPosit(const BinaryOperator *BO, std::string Op1, std::string Op2){
		SourceLocation StartLoc =  BO->getSourceRange().getBegin();
		if(!StartLoc.isValid())
			return;
		SourceManager &SM = Rewrite.getSourceMgr();
		const char *StartBuf = SM.getCharacterData(StartLoc);
		int Offset = getOffsetUntil(StartBuf, ';');
		SourceLocation StartLoc1 = BO->getSourceRange().getBegin();
		std::string IndentStr = getStmtIndentString(StartLoc1);
	
		BinOp_Temp.insert(std::pair<const BinaryOperator*, std::string>(BO, Op1));	
		removeLine(BO->getSourceRange().getBegin());
	}

	//handle all binary operators except assign
	// x = *0.4*y*z => t1 = posit_mul(y,z);
	void ReplaceBOWithPosit(const BinaryOperator *BO, std::string Op1, std::string Op2){
		
		std::string func = getPositFuncName(BO->getOpcode());
		SourceLocation StartLoc =  BO->getSourceRange().getBegin();
		if(!StartLoc.isValid())
			return;
		SourceManager &SM = Rewrite.getSourceMgr();
		const char *StartBuf = SM.getCharacterData(StartLoc);
		int Offset = getOffsetUntil(StartBuf, ';');
		std::string IndentStr = getStmtIndentString(StartLoc);
		const char *StartBuf1 = SM.getCharacterData(StartLoc.getLocWithOffset(Offset));
		std::string temp, newline;
		temp = getTempDest();
		newline = "\0";
		switch(BO->getOpcode()){
			case::BO_Add:
			case::BO_Mul:
			case::BO_Div:
			case::BO_Sub:
				Rewrite.InsertTextAfterToken(StartLoc.getLocWithOffset(Offset), 
					"\n"+IndentStr+PositTY+temp+" = "+func+"("+Op1+","+Op2+");");
				break;
			case::BO_Assign:
				Rewrite.InsertTextAfterToken(StartLoc.getLocWithOffset(Offset), 
					"\n"+IndentStr+Op1+" = "+Op2+";\n");
				removeLine(BO->getSourceRange().getBegin());
				break;
			case::BO_DivAssign:{
				std::string func = getPositFuncName(BO_Div);
				Rewrite.InsertTextAfterToken(StartLoc.getLocWithOffset(Offset), 
					"\n"+IndentStr+PositTY+Op1+" = "+func+"("+Op1+","+Op2+");");
				removeLine(BO->getSourceRange().getBegin());
				break;
			}
			case::BO_MulAssign:{
				std::string func = getPositFuncName(BO_Mul);
				Rewrite.InsertTextAfterToken(StartLoc.getLocWithOffset(Offset), 
					"\n"+IndentStr+PositTY+Op1+" = "+func+"("+Op1+","+Op2+");");
				removeLine(BO->getSourceRange().getBegin());
				break;
			}
			case::BO_AddAssign:{
				std::string func = getPositFuncName(BO_Add);
				Rewrite.InsertTextAfterToken(StartLoc.getLocWithOffset(Offset), 
					"\n"+IndentStr+PositTY+Op1+" = "+func+"("+Op1+","+Op2+");");
				removeLine(BO->getSourceRange().getBegin());
				break;
			}
			case::BO_SubAssign:{
				std::string func = getPositFuncName(BO_Sub);
				Rewrite.InsertTextAfterToken(StartLoc.getLocWithOffset(Offset), 
					"\n"+IndentStr+PositTY+Op1+" = "+func+"("+Op1+","+Op2+");");
				removeLine(BO->getSourceRange().getBegin());
				break;
			}
			default:
				llvm::errs()<<"Error!!! Operand is unknown\n\n";
		}
		BinOp_Temp.insert(std::pair<const BinaryOperator*, std::string>(BO, temp));	
	}

	void handleBinOp(){
		llvm::errs()<<"foooooooooooo********\n"<<"BOStack.size():"<<BOStack.size()<<"\n";
		while(BOStack.size() > 0){
			const BinaryOperator *BO = BOStack.top();
			BOStack.pop();
			llvm::errs()<<"BinaryOperator popped;"<<BO<<"\n";
			llvm::errs()<<"processBinOPVD bo:\n";
			BO->dump();
			//check if this binaryoperator is processed
			//check if LHS is processed, get its temp variable
			//check if RHS is processed, get its temp variable
			//create new varible, if this BO doesnt have a VD as its LHS
			//push this in map
			SmallVector<const BinaryOperator*, 8>::iterator it;
			
			Expr *Op1 = removeParen(BO->getLHS());
			Expr *Op2 = removeParen(BO->getRHS());

			std::string Op1Str = handleOperand(BO, Op1);
			std::string Op2Str = handleOperand(BO, Op2);

			llvm::errs()<<"Op1:"<<Op1Str<<"\n";
			llvm::errs()<<"Op2:"<<Op2Str<<"\n";

			ReplaceBOWithPosit(BO, Op1Str, Op2Str);
			
		}		
	}
	Expr* removeParen(Expr *Op){
		while (isa<ParenExpr>(Op)) {
			ParenExpr *PE = llvm::dyn_cast<ParenExpr>(Op);
			Op = PE->getSubExpr();
  	}
		return Op;
	}

	std::string handleOperand(const BinaryOperator *BO, Expr *Op){
			std::string Op1, lhs, rhs;
			if(FloatingLiteral *FL_lhs = dyn_cast<FloatingLiteral>(Op)){
				llvm::errs()<<"FL_lhs\n";	
				lhs = getTempDest();
				rhs  = convertFloatToPosit(FL_lhs);
				ReplaceBOLiteralWithPosit(BO, lhs, rhs);
				Op1 = lhs;
			}
			else if(BinaryOperator *BO_lhs = dyn_cast<BinaryOperator>(Op)){
				llvm::errs()<<"BO_lhs\n";	
				if(BinOp_Temp.count(BO_lhs) != 0){
					Op1 = BinOp_Temp.at(BO_lhs);
				}
			}
			else if(ImplicitCastExpr *ASE_lhs = dyn_cast<ImplicitCastExpr>(Op)){
				llvm::errs()<<"ASE_lhs\n";	
      	llvm::raw_string_ostream stream(Op1);
      	ASE_lhs->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
      	stream.flush();
      	llvm::errs()<<"opName:"<<Op1<<"\n"; 
				//handle inttoD
			}
/*
			else if(ASE_lhsItoD){
				llvm::errs()<<"ASE_lhsItoD\n";	
      	llvm::raw_string_ostream stream(Op1);
      	ASE_lhsItoD->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
      	stream.flush();
				lhs = getTempDest();
				rhs = " = convertDoubleToP32(" + Op1+");";
				ReplaceBOLiteralWithPosit(BO, lhs, rhs);
        Op1 = lhs;
      	llvm::errs()<<"opName:"<<Op1<<"\n"; 
			}*/
			else if(DeclRefExpr *DEL = dyn_cast<DeclRefExpr>(Op)){
				llvm::errs()<<"DEL_lhs\n";	
				Op1 = DEL->getDecl()->getName();
			}
			else{
				llvm::errs()<<"Op Not found.......\n\n";
      	llvm::raw_string_ostream stream(Op1);
      	Op->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
      	stream.flush();
      	llvm::errs()<<"opName:"<<Op1<<"\n"; 
				Op->dump();
			}
		return Op1;
	}

  FloatVarDeclHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
		if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vd_literal")){
			llvm::errs()<<"vd_literal\n";
			const FloatingLiteral *FL = Result.Nodes.getNodeAs<clang::FloatingLiteral>("floatliteral");
			if(FL != NULL){
				std::string positLiteral = VD->getNameAsString()+convertFloatToPosit(FL);
				ReplaceVDWithPosit(VD->getSourceRange().getBegin(), positLiteral);
			}
			const IntegerLiteral *IL = Result.Nodes.getNodeAs<clang::IntegerLiteral>("intliteral");
			if(IL != NULL){
				std::string positLiteral = convertIntToPosit(IL);
				ReplaceVDWithPosit(VD->getSourceRange().getBegin(), positLiteral);
			}
		}
		if (const UnaryExprOrTypeTraitExpr *UE = Result.Nodes.getNodeAs<clang::UnaryExprOrTypeTraitExpr>("unary")){
			llvm::errs()<<"unary:\n";
			UE->getTypeOfArgument()->dump();
			QualType QT = UE->getTypeOfArgument();
      std::string TypeStr = QT.getAsString();
      SourceManager &SM = Rewrite.getSourceMgr();
      if(TypeStr.find("double") == 0){
        const char *Buf = SM.getCharacterData(UE->getSourceRange().getBegin());
        int StartOffset = 0;
        while (*Buf != '(') {
           Buf++;
          StartOffset++;
        }
        StartOffset++;
				int Offset = 0;
        while (*Buf != ')') {
           Buf++;
					if(*Buf == ' ')
						break; 
					if(*Buf == '*')
						break; 
          Offset++;
        }

      //  int RangeSize = TheRewriter.getRangeSize(SourceRange(c->getLocStart(), c->getExprLoc()));
        Rewrite.ReplaceText(UE->getSourceRange().getBegin().getLocWithOffset(StartOffset), Offset, "posit32_t");
      }
				//->getName();
//			ReplaceVDWithPosit(UE->getLocStart(), UE->getNameAsString()+";");
		}
		if (const FieldDecl *FD = Result.Nodes.getNodeAs<clang::FieldDecl>("struct")){
			llvm::errs()<<"struct\n"<<FD->getDeclName();
			ReplaceVDWithPosit(FD->getSourceRange().getBegin(), FD->getNameAsString()+";");
		}
		if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclnoinit")){
			llvm::errs()<<"vardeclnoinit\n";
			VD->dump();
			llvm::errs()<<"\n";
			const ParmVarDecl *PD = dyn_cast<ParmVarDecl>(VD);
  		if (PD) {
				ReplaceParmVDWithPosit(VD, ' ');
  		}
			else{
				ReplaceVDWithPosit(VD->getSourceRange().getBegin(), VD->getNameAsString()+";");
			}
		}
		if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclarray")){
			llvm::errs()<<"vardeclarray\n";
			VD->dump();
			llvm::errs()<<"\n";
			const Type *Ty = VD->getType().getTypePtr();
			while (Ty->isArrayType()) {                                                             
    		const ArrayType *AT = dyn_cast<ArrayType>(Ty);
    		Ty = AT->getElementType().getTypePtr();
  		}
			if(!Ty->isFloatingType())
				return;

			std::string arrayDim = getArrayDim(VD);
			llvm::errs()<<"arrayDim:"<<arrayDim<<"\n";
			ReplaceVDWithPosit(VD->getSourceRange().getBegin(), VD->getNameAsString()+arrayDim);
		}
		if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclpointer")){
			llvm::errs()<<"vardeclpointer\n";
			VD->dump();
			llvm::errs()<<"\n\n";
			const Type *Ty = VD->getType().getTypePtr();
			QualType QT = Ty->getPointeeType();
			while (!QT.isNull()) {
				Ty = QT.getTypePtr();
				QT = Ty->getPointeeType();
			}
			if(!Ty->isFloatingType())
				return;
			const ParmVarDecl *PD = dyn_cast<ParmVarDecl>(VD);
  		if (PD) {
				//for function parameter end string will be either ',' or ')'
				//we want to replace double with posit, instead of creating a new variable
				ReplaceParmVDWithPosit(VD, '*');
  		}
			else{
        Rewrite.ReplaceText(VD->getSourceRange().getBegin(), 6, "posit32_t");
//				ReplaceVDWithPosit(VD->getLocStart(), VD->getNameAsString()+";");
			}
		}
		if(const CStyleCastExpr *DEL = Result.Nodes.getNodeAs<clang::CStyleCastExpr>("cast")){
			llvm::errs()<<"cast...\n";
		}
		if (const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("callbo")){
			llvm::errs()<<"vardeclbo\n";
			const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("op");
			if(BinOp_Temp.count(BO) != 0){
				std::string Op1 = BinOp_Temp.at(BO);
				Rewrite.InsertTextAfterToken(CE->getSourceRange().getBegin(), 
					"\n"+Op1);
			}
		}
		if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclbo")){
			llvm::errs()<<"vardeclbo\n";
			const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("op");
			if(BinOp_Temp.count(BO) != 0){
				std::string Op1 = BinOp_Temp.at(BO);
				ReplaceVDWithPosit(VD->getSourceRange().getBegin(), VD->getNameAsString()+" = "+Op1+";");
			}
		}
		if (const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("fadd_be")){
			llvm::errs()<<"fadd_be\n\n\n";
			BO->dump();	
			//check if this binaryoperator is processed
			//check if LHS is processed, get its temp variable
			//check if RHS is processed, get its temp variable
			//create new varible, if this BO doesnt have a VD as its LHS
			//push this in map
			SmallVector<const BinaryOperator*, 8>::iterator it;
			it = std::find(ProcessedBO.begin(), ProcessedBO.end(), BO);		
			if(it != ProcessedBO.end())
				return;
			
			Expr *Op1 = removeParen(BO->getLHS());
			Expr *Op2 = removeParen(BO->getRHS());

//			std::string Op1Str = handleOperand(BO, Op1);
//			std::string Op2Str = handleOperand(BO, Op2);

//			llvm::errs()<<"Op1:"<<Op1Str<<"\n";
//			llvm::errs()<<"Op2:"<<Op2Str<<"\n";
			ProcessedBO.push_back(BO);
/*
			if (BO->getOpcode() == BO_Assign){
				ReplaceBOEqWithPosit(BO, Op1Str, Op2Str);
			}
*/
			BOStack.push(BO);
		}
  }

private:
  Rewriter &Rewrite;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser. It registers a couple of matchers and runs them on
// the AST.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R) :  HandlerFloatVarDecl(R){

		//matcher for  double x = 3.4, y = 5.6;
		Matcher.addMatcher(
				varDecl(hasType(realFloatingPointType()), anyOf(hasInitializer(ignoringParenImpCasts(
					integerLiteral().bind("intliteral"))), hasInitializer(ignoringParenImpCasts(
          floatLiteral().bind("floatliteral")))))
						.bind("vd_literal"), &HandlerFloatVarDecl);
		//matcher for  double x, y;
		//ignores double x = x + y;
		Matcher.addMatcher(
			varDecl(hasType(realFloatingPointType()), unless( hasInitializer(floatLiteral())), unless(hasType(arrayType())), 
				unless( hasInitializer(ignoringParenImpCasts(integerLiteral()))), unless(hasDescendant(binaryOperator()))).
					bind("vardeclnoinit"), &HandlerFloatVarDecl);


		//pointer
		Matcher.addMatcher(
			varDecl(hasType(pointerType())). 
					bind("vardeclpointer"), &HandlerFloatVarDecl);

		Matcher.addMatcher(cStyleCastExpr().bind("cast")
					, &HandlerFloatVarDecl);
	//match structs with floating point field elements
		Matcher.addMatcher(
			fieldDecl(hasType(realFloatingPointType())).bind("struct"), &HandlerFloatVarDecl);

		Matcher.addMatcher(
			varDecl(hasType(arrayType()), unless(	hasInitializer(initListExpr()	))).
					bind("vardeclarray"), &HandlerFloatVarDecl);
		//function parameters
		Matcher.addMatcher(
			cxxMethodDecl(hasAnyParameter(hasType(realFloatingPointType()))).
				bind("vardeclnoinit"), &HandlerFloatVarDecl);

  const auto FloatPtrType = pointerType(pointee(realFloatingPointType()));
	 const auto PointerToFloat = 
      hasType(qualType(hasCanonicalType(pointerType(pointee(realFloatingPointType())))));


		Matcher.addMatcher(
			unaryExprOrTypeTraitExpr(ofKind(UETT_SizeOf)).
				bind("unary"), &HandlerFloatVarDecl);


		const auto Op1 =  anyOf(ignoringParenImpCasts(declRefExpr(
					to(varDecl(hasType(realFloatingPointType()))))), 
            implicitCastExpr(unless(hasImplicitDestinationType(realFloatingPointType()))),
             	implicitCastExpr(hasImplicitDestinationType(realFloatingPointType())),
            		ignoringParenImpCasts(ignoringParens(floatLiteral())));
			
		
		//all binary operators, except '=' and binary operators which have operand as binary operator
		const auto Basic = binaryOperator(unless(hasOperatorName("=")), hasType(realFloatingPointType()), 
							hasLHS(Op1),
							hasRHS(Op1)).bind("fadd_be");

		
		const auto BinOp1 = binaryOperator(hasType(realFloatingPointType()), hasEitherOperand(anyOf(ignoringParens(Basic), Op1))).bind("fadd_be");
		Matcher1.addMatcher(BinOp1, &HandlerFloatVarDecl);

		//double t = 0.5 + x + z * y ;   
		Matcher4.addMatcher(
			varDecl(hasType(realFloatingPointType()), unless( hasInitializer(floatLiteral())), 
				unless( hasInitializer(ignoringParenImpCasts(integerLiteral()))), hasDescendant(binaryOperator().bind("op"))).
					bind("vardeclbo"), &HandlerFloatVarDecl);

		Matcher4.addMatcher(
			callExpr(hasDescendant(binaryOperator(hasType(realFloatingPointType())).bind("op"))).
					bind("callbo"), &HandlerFloatVarDecl);
//TODO: How to generalize any array with builting type as floaitng point
	//typedef
	//struct type
	//pointer type
	//binaryoperators

  }

  void HandleTranslationUnit(ASTContext &Context) override {
    // Run the matchers when we have the whole TU parsed.
    Matcher.matchAST(Context);
    Matcher1.matchAST(Context);
    Matcher2.matchAST(Context);
    Matcher3.matchAST(Context);
		HandlerFloatVarDecl.handleBinOp();
    Matcher4.matchAST(Context);
  }

	bool HandleTopLevelDecl(DeclGroupRef DR) override {
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
      // Traverse the declaration using our AST visitor.
      (*b)->dump();
    }
    return true;
  }
private:
  FloatVarDeclHandler HandlerFloatVarDecl;
  MatchFinder Matcher;
  MatchFinder Matcher1;
  MatchFinder Matcher2;
  MatchFinder Matcher3;
  MatchFinder Matcher4;
  MatchFinder Matcher5;
};

// For each source file provided to the tool, a new FrontendAction is created.
class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() {}
  void EndSourceFileAction() override {
    TheRewriter.getEditBuffer(TheRewriter.getSourceMgr().getMainFileID())
        .write(llvm::outs());
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return std::make_unique<MyASTConsumer>(TheRewriter);
  }

private:
  Rewriter TheRewriter;
};

int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, MatcherSampleCategory);
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());

  return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
