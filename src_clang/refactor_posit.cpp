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
#include "llvm/Support/CommandLine.h"
#include "clang/Lex/Lexer.h" 
#include <unistd.h>
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>

#define GetCurrentDir getcwd
using namespace std;
using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

static llvm::cl::OptionCategory MyToolCategory("My tool options");
static llvm::cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp MoreHelp("\nMore help text...");
static llvm::cl::opt<string> YourOwnOption("abc", llvm::cl::cat(MyToolCategory));
#define DoubleSize 6
std::string PositMathFunc = "p32_";
std::string PositTY = "posit32_t ";
std::string PositDtoP = "convertDoubleToP32 ";
std::string PositPtoD = "convertP32ToDouble ";
std::stringstream SSBefore;
//track temp variables
unsigned tmpCount = 0;

std::set<const Stmt *> ForceBracesStmts;
std::map<const IfStmt*, SourceLocation> BinIfLoc; 
std::map<const BinaryOperator*, std::string> BinOp_Temp; 
std::map<const BinaryOperator*, SourceLocation> BinLoc_Temp; 
std::map<const BinaryOperator*, const CallExpr*> BinParentCE; 
std::map<const BinaryOperator*, const Stmt*> BinParentST; 
std::map<const BinaryOperator*, const BinaryOperator*> BinParentBO; 
std::map<const BinaryOperator*, const VarDecl*> BinParentVD; 
std::stack<const BinaryOperator*> BOStack; 
SmallVector<unsigned, 8> ProcessedLine;
SmallVector<const BinaryOperator*, 8> ProcessedBO;
SmallVector<const FloatingLiteral*, 8> ProcessedFL;
SmallVector<const InitListExpr*, 8> ProcessedILE;
SmallVector<const IntegerLiteral*, 8> ProcessedIL;
SmallVector<const VarDecl*, 8> ProcessedVD;
SmallVector<const CallExpr*, 8> ProcessedCE;

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
	unsigned insertHeader(SourceLocation  StmtStartLoc){
		SourceManager &SM = Rewrite.getSourceMgr();
		
		if (StmtStartLoc.isMacroID()) {
   		StmtStartLoc = SM.getFileLoc(StmtStartLoc);
 		}
 
  	FileID FID;
  	unsigned StartOffset = 
    	getLocationOffsetAndFileID(StmtStartLoc, FID, &SM);
  
  	StringRef MB = SM.getBufferData(FID);
		Rewrite.InsertText(StmtStartLoc, 
					"#include \"softposit.h\"\n", true, true);
	}

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
  		if(*StartBuf == '=') 
				break;
    	StartBuf++;
			Offset++;
		}
		
		char *result = (char *)malloc(Offset+2);
		
		strncpy(result, OrigBuf+StartOffset, Offset);
		result[Offset] = ';'; 
		result[Offset+1] = '\0'; 
		return result;
	}

	void removeLineBO(SourceLocation StartLoc){
		int StartOffset = 0;
		SourceManager &SM = Rewrite.getSourceMgr();
		const char *StartBuf = SM.getCharacterData(StartLoc);
		unsigned lineNo = getLineNo(StartLoc);
    //is this the last vardecl in stmt? if yes, then removeLine the statement
		int Offset = getOffsetUntil(StartBuf, ';');
		Rewriter::RewriteOptions Opts;
		Opts.RemoveLineIfEmpty = true;
		Rewrite.RemoveText(StartLoc, Offset+1, Opts); 
	}

	void removeLineVD(SourceLocation StartLoc){
		int StartOffset = 0;
		SourceManager &SM = Rewrite.getSourceMgr();
		const char *StartBuf = SM.getCharacterData(StartLoc);
		unsigned lineNo = getLineNo(StartLoc);
    //is this the last vardecl in stmt? if yes, then removeLine the statement
		int Offset = getOffsetUntil(StartBuf, ';');
		SmallVector<unsigned, 8>::iterator it;
		it = std::find(ProcessedLine.begin(), ProcessedLine.end(), lineNo);		
		if(it == ProcessedLine.end()){
			Rewriter::RewriteOptions Opts;
			Opts.RemoveLineIfEmpty = true;
			Rewrite.RemoveText(StartLoc, Offset+1, Opts); 
			//Rewrite.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
			ProcessedLine.push_back(lineNo);
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
		std::string convert =  PositDtoP+"(" + SSBefore.str()+");";
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
		std::string convert = PositDtoP+"(" + SSBefore.str()+");";
		return convert ;
	}

	//This function returns temp variables
	std::string getTempDest(){
		tmpCount++;
		return "tmp"+std::to_string(tmpCount);
	}

	std::string getPositBinOp(unsigned Opcode){
		string funcName;
		switch(Opcode){
			case BO_EQ:
				funcName = "p32_eq";
				break;
			case BO_LE:
				funcName = "p32_le";
				break;
			case BO_LT:
				funcName = "p32_lt";
				break;
			case BO_GE:
				funcName = "!p32_lt";
				break;
			case BO_GT:
				funcName = "!p32_le";
				break;
			default:
				assert("This opcode is not handled!!!");
		}
		return funcName;
	}

	std::string getPositFuncName(unsigned Opcode){
		string funcName;
		switch(Opcode){
			case BO_Mul:
			case BO_MulAssign:
				funcName = "p32_mul";
				break;
			case BO_Div:
			case BO_DivAssign:
				funcName = "p32_div";
				break;
			case UO_PostInc:
			case BO_Add:
			case BO_AddAssign:
				funcName = "p32_add";
				break;
			case UO_PostDec:
			case BO_Sub:
			case BO_SubAssign:
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
	void ReplaceParmVDWithPosit(SourceLocation StartLoc, char positLiteral){
		if(!StartLoc.isValid())
			return;
		SourceManager &SM = Rewrite.getSourceMgr();
		const char *StartBuf = SM.getCharacterData(StartLoc);
		int Offset = getOffsetUntil(StartBuf, positLiteral);
		Rewrite.ReplaceText(SourceRange(StartLoc, StartLoc.getLocWithOffset(Offset-1)), 
													PositTY);	
	}

	//double x => posit32_t x
	void ReplaceVDWithPosit(SourceLocation StartLoc, SourceLocation EndLoc, std::string positLiteral){
		if(!StartLoc.isValid())
			return;
		SourceManager &SM = Rewrite.getSourceMgr();
		Rewrite.InsertText(StartLoc, 
					PositTY+positLiteral, true, true);
		removeLineVD(StartLoc);
	}

	std::string handleMathFunc(const std::string funcName){
#if 1
		if(funcName == "sqrt" || funcName == "cos" ||
			funcName == "sin" || funcName == "tan" ||
			funcName == "cosec")
				return PositMathFunc+funcName; 
		else
#endif
				return funcName; 
	}
//	x = y * 0.3 => t1 = convertdoubletoposit(0.3)
	void ReplaceBOLiteralWithPosit(const BinaryOperator *BO, SourceLocation StartLoc, std::string lhs, std::string rhs){
		if(!StartLoc.isValid())
			return;
		SourceManager &SM = Rewrite.getSourceMgr();
		Rewrite.InsertText(StartLoc, 
					PositTY+lhs +rhs+"\n", true, true);		
	}

	std::string ReplaceUOWithPosit(const UnaryOperator *UO, SourceLocation BOStartLoc, 
														std::string Op1, std::string Op2){
		
		unsigned Opcode = UO->getOpcode();
		if(!BOStartLoc.isValid())
			return nullptr;
		int Offset = -1;
		SourceManager &SM = Rewrite.getSourceMgr();
		const char *StartBuf1 = SM.getCharacterData(BOStartLoc.getLocWithOffset(Offset));
		std::string temp;
		temp = getTempDest();
		switch(Opcode){
			case::UO_PreInc:
			case::UO_PostInc:{
				std::string func = getPositFuncName(BO_Add);
				Rewrite.InsertText(BOStartLoc, 
					PositTY+temp+" = "+func+"("+Op1+","+Op2+");\n", true, true);
				break;
			}
			case::UO_PreDec:
			case::UO_PostDec:{
				std::string func = getPositFuncName(BO_Sub);
				Rewrite.InsertText(BOStartLoc, 
					PositTY+Op1+" = "+func+"("+Op1+","+Op2+");\n", true, true);
				//removeLine(BO->getSourceRange().getBegin(), BO->getSourceRange().getEnd());
				break;
			}
			default:
				llvm::errs()<<"Error!!! Operand is unknown\n\n";
		}
		return temp;
	}
	//handle all binary operators except assign
	// x = *0.4*y*z => t1 = posit_mul(y,z);
	void ReplaceBOWithPosit(const BinaryOperator *BO, SourceLocation BOStartLoc, 
														std::string Op1, std::string Op2){
		llvm::errs()<<"*******\n";	
		BO->dump();
		llvm::errs()<<"*******\n";	
		unsigned Opcode = BO->getOpcode();
		std::string func = getPositFuncName(Opcode);
		SourceManager &SM = Rewrite.getSourceMgr();
		if(!BOStartLoc.isValid()){
			llvm::errs()<<"Error!!! ReplaceBOWithPosit loc is invalid\n";
			return;
		}
		int Offset = -1;
		std::string temp;
		temp = getTempDest();
		switch(Opcode){
			case::BO_Add:
			case::BO_Mul:
			case::BO_Div:
			case::BO_Sub:{	
				llvm::errs()<<"Bo Add\n";
				Rewrite.InsertText(BOStartLoc, 
					PositTY+temp+" = "+func+"("+Op1+","+Op2+");\n", true, true);
				BinOp_Temp.insert(std::pair<const BinaryOperator*, std::string>(BO, temp));	
				break;
			}
			case::BO_Assign:{
				llvm::errs()<<"Bo Assign\n";
				Rewrite.InsertText(BOStartLoc,
					Op1+" = "+Op2+";\n", true, true);
				removeLineBO(BO->getSourceRange().getBegin());
				BinOp_Temp.insert(std::pair<const BinaryOperator*, std::string>(BO, Op1));	
				break;
			}
			case::BO_DivAssign:
			case::BO_MulAssign:
			case::BO_AddAssign:
			case::BO_SubAssign:{
				llvm::errs()<<"Bo AddAssign\n";
				Rewrite.InsertText(BOStartLoc, 
					Op1+" = "+func+"("+Op1+","+Op2+");\n", true, true);
				removeLineBO(BO->getSourceRange().getBegin());
				BinOp_Temp.insert(std::pair<const BinaryOperator*, std::string>(BO, Op1));	
				llvm::errs()<<"BO_AddAssign Op1:"<<Op1<<" Op2:"<<Op2<<"\n";
				break;
			}
			default:
				llvm::errs()<<"Error!!! Operand is unknown\n\n";
		}
			//update parent

		SourceLocation StartLoc = BO->getSourceRange().getBegin();
		if (StartLoc.isMacroID()) {
    	StartLoc = SM.getFileLoc(StartLoc); 
		}
		SourceLocation EndLoc = BO->getSourceRange().getEnd();
		if (EndLoc.isMacroID()) {
    	EndLoc = SM.getFileLoc(StartLoc); 
		}
		if(BinParentCE.count(BO) != 0){
			llvm::errs()<<"ReplaceBOWithPosit: parent is CE\n";
			const CallExpr *CE = BinParentCE.at(BO);
			const FunctionDecl *Func = CE->getDirectCallee();
      const std::string funcName = Func->getNameInfo().getAsString();
			if(funcName == "printf") {
				llvm::errs()<<"ReplaceBOWithPosit: func name is printf\n";
				std::string convert;
				convert = PositPtoD+"(" + temp +");";
				std::string tmp;
				tmp = getTempDest();
				Rewrite.InsertText(CE->getSourceRange().getBegin(), 
									"double "+tmp +" = "+convert+"\n", true, true);		
				Rewrite.ReplaceText(SourceRange(StartLoc, EndLoc), tmp);
			}
			else
				Rewrite.ReplaceText(SourceRange(StartLoc, EndLoc), temp);
		}
		else if(BinParentBO.count(BO) != 0){
			llvm::errs()<<"ReplaceBOWithPosit: parent is BO\n";
			Rewrite.ReplaceText(SourceRange(StartLoc, EndLoc), temp);
		}
		else if(BinParentVD.count(BO) != 0){
			llvm::errs()<<"ReplaceBOWithPosit: parent is VD\n";
			const VarDecl* VD = BinParentVD.at(BO);
			ReplaceVDWithPosit(VD->getSourceRange().getBegin(), VD->getSourceRange().getEnd(), VD->getNameAsString()+" = "+temp+";");
		}
		else{
			llvm::errs()<<"ReplaceBOWithPosit: Error!!! no parent\n";
		}	
	}

	void handleBinOp(ASTContext &Context){
		llvm::errs()<<"foooooooooooo********\n"<<"BOStack.size():"<<BOStack.size()<<"\n";
		const BinaryOperator *BO;
		while(BOStack.size() > 0){
			BO = BOStack.top();

			BOStack.pop();
			//check if this binaryoperator is processed
			//check if LHS is processed, get its temp variable
			//check if RHS is processed, get its temp variable
			//create new varible, if this BO doesnt have a VD as its LHS
			//push this in map
			SmallVector<const BinaryOperator*, 8>::iterator it;
			
			Expr *Op1 = removeParen(BO->getLHS());
			Expr *Op2 = removeParen(BO->getRHS());

			SourceLocation StartLoc = BinLoc_Temp.at(BO);
			
			if(BinParentST.count(BO) != 0){
				auto ST = BinParentST.at(BO);
		 		if(const IfStmt *IfSt = dyn_cast<IfStmt>(ST)) {
					StartLoc = BinIfLoc.at(IfSt); 
				}
			}
			unsigned lineNo = getLineNo(StartLoc);
			llvm::errs()<<"handleBinOp lineNo:"<<lineNo<<"\n";
			std::string Op1Str = handleOperand(BO, StartLoc, Op1);
			std::string Op2Str = handleOperand(BO, StartLoc, Op2);

			llvm::errs()<<"Op1:"<<Op1Str<<"\n";
			llvm::errs()<<"Op2:"<<Op2Str<<"\n";

			ReplaceBOWithPosit(BO, StartLoc, Op1Str, Op2Str);
		}		
	}
	bool isPointerToFloatingType(const Type *Ty){
		QualType QT = Ty->getPointeeType();
		while (!QT.isNull()) {
			Ty = QT.getTypePtr();
			QT = Ty->getPointeeType();
		}
		if(Ty->isFloatingType())
			return true;
		return false;
	}
	bool isArithmetic(unsigned opCode){
		switch(opCode){
			case UO_PreInc:
			case UO_PostInc:
			case UO_PostDec:
			case UO_PreDec:
				return true;
			default:
				return false;
		}
	}

	Expr* removeParen(Expr *Op){
		while (isa<ParenExpr>(Op)) {
			ParenExpr *PE = llvm::dyn_cast<ParenExpr>(Op);
			Op = PE->getSubExpr();
  	}
		while (isa<ImplicitCastExpr>(Op)) {
			ImplicitCastExpr *PE = llvm::dyn_cast<ImplicitCastExpr>(Op);
			Op = PE->getSubExpr();
		}
		while (isa<UnaryOperator>(Op)) {
			UnaryOperator *UE = llvm::dyn_cast<UnaryOperator>(Op);
			if(!isArithmetic(UE->getOpcode()))
				Op = UE->getSubExpr();
			else
				break;
		}
		return Op;
	}

	SourceLocation getEndLocation(SourceRange Range)
	{
      	SourceManager &SM = Rewrite.getSourceMgr();
  SourceLocation StartLoc = Range.getBegin();
  SourceLocation EndLoc = Range.getEnd();
  if (StartLoc.isInvalid())
    return StartLoc;
  if (EndLoc.isInvalid())
    return EndLoc;

  if (StartLoc.isMacroID())
    StartLoc = SM.getFileLoc(StartLoc);
  if (EndLoc.isMacroID())
    EndLoc = SM.getFileLoc(EndLoc);

  SourceRange NewRange(StartLoc, EndLoc);
  int LocRangeSize = Rewrite.getRangeSize(NewRange);
  if (LocRangeSize == -1)
    return NewRange.getEnd();

  	return StartLoc.getLocWithOffset(LocRangeSize);
	}

	std::string handleOperand(const BinaryOperator *BO, SourceLocation StartLoc, Expr *Op){
			std::string Op1, lhs, rhs;
			if(FloatingLiteral *FL_lhs = dyn_cast<FloatingLiteral>(Op)){
				llvm::errs()<<"Op is floatliteral\n";
				lhs = getTempDest();
				rhs = convertFloatToPosit(FL_lhs);
				ReplaceBOLiteralWithPosit(BO, StartLoc, lhs, " = "+rhs);
				Op1 = lhs;
			}
			else if(UnaryOperator *U_lhs = dyn_cast<UnaryOperator>(Op)){
				llvm::errs()<<"Op is unaryOperator\n";
				lhs = getTempDest();
				rhs = " = "+PositDtoP+"(1);";
				ReplaceBOLiteralWithPosit(BO, StartLoc, lhs, rhs);
				std::string func = getPositFuncName(BO->getOpcode());
				const Type *Ty = U_lhs->getType().getTypePtr();
				QualType QT = Ty->getPointeeType();
				std::string indirect="";
				while (!QT.isNull()) {
					Ty = QT.getTypePtr();
					QT = Ty->getPointeeType();
					indirect += "*";
				}
				std::string opName;
      	llvm::raw_string_ostream stream(opName);
      	U_lhs->getSubExpr()->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
      	stream.flush();
				std::string tmp = ReplaceUOWithPosit(U_lhs, BinLoc_Temp.at(BO), lhs, indirect+opName);
				Op1 = tmp;
			}
			else if(BinaryOperator *BO_lhs = dyn_cast<BinaryOperator>(Op)){
				llvm::errs()<<"Op is binaryOperator\n";
				if(BinOp_Temp.count(BO_lhs) != 0){
					Op1 = BinOp_Temp.at(BO_lhs);
					llvm::errs()<<"Op is found:op:"<<Op1<<"\n";
				}
			}
			else if(ImplicitCastExpr *ASE_lhs = dyn_cast<ImplicitCastExpr>(Op)){
				llvm::errs()<<"Op is implicitcast\n";
      	llvm::raw_string_ostream stream(Op1);
      	ASE_lhs->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
      	stream.flush();
				
				//handle inttoD
				const Expr *SubExpr = ASE_lhs->getSubExpr();
				const Type *SubTy =  SubExpr->getType().getTypePtr();
				if(SubTy->isIntegerType()){
					lhs = getTempDest();
					rhs = " = "+PositDtoP+"(" + Op1+");";
					ReplaceBOLiteralWithPosit(BO, StartLoc, lhs, rhs);
        	Op1 = lhs;
				}
			}
			else if(DeclRefExpr *DEL = dyn_cast<DeclRefExpr>(Op)){
				llvm::errs()<<"Op is decl\n";
				Op1 = DEL->getDecl()->getName();
			}
			else if(const CStyleCastExpr *CCE = dyn_cast<CStyleCastExpr>(Op)){
				llvm::errs()<<"Op is cast\n";
				std::string subExpr;
      	CCE->getSubExpr();
      	llvm::raw_string_ostream stream(subExpr);
      	CCE->getSubExpr()->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
      	stream.flush();
      	std::cout<<"subExpr:"<<subExpr<<"\n";
      	std::string temp = getTempDest();
				std::string rhs = " = "+PositDtoP+"(" + subExpr+");";
				ReplaceBOLiteralWithPosit(BO, StartLoc, temp, rhs);
				Op1 = temp;
			}
			else if(const CallExpr *CE = dyn_cast<CallExpr>(Op)){
				llvm::errs()<<"Op is call expr";
				const FunctionDecl *Func = CE->getDirectCallee();
				std::string funcName = handleMathFunc(Func->getNameInfo().getAsString());
				Op1 = funcName+"(";
				for(int i=0, j=CE->getNumArgs(); i<j; i++){
					auto Arg = dyn_cast<BinaryOperator>(CE->getArg(i));
					if(Arg && isPointerToFloatingType(Arg->getType().getTypePtr())){
						std::string temp;
						temp = BinOp_Temp.at(Arg);
						Op1 += temp;
					}
					else{
        		std::string ArgName;
        		llvm::raw_string_ostream s(ArgName);
        		CE->getArg(i)->printPretty(s, NULL, PrintingPolicy(LangOptions()));
						llvm::errs()<<"ArgName:"<<s.str()<<"\n";
						Op1 += s.str();
					}
    		}
				Op1 += ")";
			}
			else{
				llvm::errs()<<"Op Not found, returning op as string.......\n\n";
      	llvm::raw_string_ostream stream(Op1);
      	Op->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
      	stream.flush();
			}
		return Op1;
	}
tok::TokenKind getTokenKind(SourceLocation Loc, const SourceManager &SM,                                                                                                    
                            const ASTContext *Context) {
  Token Tok;
  SourceLocation Beginning =
      Lexer::GetBeginningOfToken(Loc, SM, Context->getLangOpts());
  const bool Invalid =
      Lexer::getRawToken(Beginning, Tok, SM, Context->getLangOpts());
  assert(!Invalid && "Expected a valid token.");

  if (Invalid)
    return tok::NUM_TOKENS;

  return Tok.getKind();
}
SourceLocation forwardSkipWhitespaceAndComments(SourceLocation Loc,
                                                const SourceManager &SM,
                                                const ASTContext *Context) {
  assert(Loc.isValid());
  for (;;) {
    while (isWhitespace(*SM.getCharacterData(Loc)))
      Loc = Loc.getLocWithOffset(1);

    tok::TokenKind TokKind = getTokenKind(Loc, SM, Context);
    if (TokKind == tok::NUM_TOKENS || TokKind != tok::comment)
      return Loc;

    // Fast-forward current token.
    Loc = Lexer::getLocForEndOfToken(Loc, 0, SM, Context->getLangOpts());
  }
}

SourceLocation findEndLocation(SourceLocation LastTokenLoc,
                               const SourceManager &SM,
                               const ASTContext *Context) {                                                                                                                 
  SourceLocation Loc =
      Lexer::GetBeginningOfToken(LastTokenLoc, SM, Context->getLangOpts());
  // Loc points to the beginning of the last (non-comment non-ws) token
  // before end or ';'.
  assert(Loc.isValid());
  bool SkipEndWhitespaceAndComments = true;
  tok::TokenKind TokKind = getTokenKind(Loc, SM, Context);
  if (TokKind == tok::NUM_TOKENS || TokKind == tok::semi ||
      TokKind == tok::r_brace) {
    // If we are at ";" or "}", we found the last token. We could use as well
    // `if (isa<NullStmt>(S))`, but it wouldn't work for nested statements.
    SkipEndWhitespaceAndComments = false;
  }
	 Loc = Lexer::getLocForEndOfToken(Loc, 0, SM, Context->getLangOpts());
  // Loc points past the last token before end or after ';'.
  if (SkipEndWhitespaceAndComments) {
    Loc = forwardSkipWhitespaceAndComments(Loc, SM, Context);
    tok::TokenKind TokKind = getTokenKind(Loc, SM, Context);
    if (TokKind == tok::semi)
      Loc = Lexer::getLocForEndOfToken(Loc, 0, SM, Context->getLangOpts());
  }

  for (;;) {
    assert(Loc.isValid());
    while (isHorizontalWhitespace(*SM.getCharacterData(Loc))) {
      Loc = Loc.getLocWithOffset(1);
    }

    if (isVerticalWhitespace(*SM.getCharacterData(Loc))) {
      // EOL, insert brace before.
      break;
    }
    tok::TokenKind TokKind = getTokenKind(Loc, SM, Context);
    if (TokKind != tok::comment) {
      // Non-comment token, insert brace before.
      break;
    }

    SourceLocation TokEndLoc =
        Lexer::getLocForEndOfToken(Loc, 0, SM, Context->getLangOpts());
    SourceRange TokRange(Loc, TokEndLoc);
    StringRef Comment = Lexer::getSourceText(
        CharSourceRange::getTokenRange(TokRange), SM, Context->getLangOpts());
    if (Comment.startswith("/*") && Comment.find('\n') != StringRef::npos) {
      // Multi-line block comment, insert brace before.
      break;
    }
	 // else: Trailing comment, insert brace after the newline.

    // Fast-forward current token.
    Loc = TokEndLoc;
  }
  return Loc;
}
template <typename IfOrWhileStmt>
SourceLocation findRParenLoc(const IfOrWhileStmt *S,
                                           const SourceManager &SM,
                                           const ASTContext *Context) {
  // Skip macros.
  if (S->getBeginLoc().isMacroID())
    return SourceLocation();

  SourceLocation CondEndLoc = S->getCond()->getEndLoc();
  if (const DeclStmt *CondVar = S->getConditionVariableDeclStmt())
    CondEndLoc = CondVar->getEndLoc();

  if (!CondEndLoc.isValid()) {
    return SourceLocation();
  }

  SourceLocation PastCondEndLoc =
      Lexer::getLocForEndOfToken(CondEndLoc, 0, SM, Context->getLangOpts());
  if (PastCondEndLoc.isInvalid())
    return SourceLocation();
  SourceLocation RParenLoc =
      forwardSkipWhitespaceAndComments(PastCondEndLoc, SM, Context);
  if (RParenLoc.isInvalid())
    return SourceLocation();
  tok::TokenKind TokKind = getTokenKind(RParenLoc, SM, Context);
  if (TokKind != tok::r_paren)
    return SourceLocation();
  return RParenLoc;
}   
bool checkStmt(
    const MatchFinder::MatchResult &Result, const Stmt *S,
    SourceLocation InitialLoc) {
	SourceLocation EndLocHint = SourceLocation();
	checkStmtWithLoc(Result, S, InitialLoc, EndLocHint);
}
/// Determine if the statement needs braces around it, and add them if it does.
/// Returns true if braces where added.
bool checkStmtWithLoc(
    const MatchFinder::MatchResult &Result, const Stmt *S,
    SourceLocation InitialLoc, SourceLocation EndLocHint) {
  // 1) If there's a corresponding "else" or "while", the check inserts "} "
  // right before that token.
  // 2) If there's a multi-line block comment starting on the same line after
  // the location we're inserting the closing brace at, or there's a non-comment
  // token, the check inserts "\n}" right before that token.
  // 3) Otherwise the check finds the end of line (possibly after some block or
  // line comments) and inserts "\n}" right before that EOL.
  if (!S || isa<CompoundStmt>(S)) {
    // Already inside braces.
    return false;
  }

  if (!InitialLoc.isValid())
    return false;
  const SourceManager &SM = *Result.SourceManager;
  const ASTContext *Context = Result.Context;

  // Treat macros.
  CharSourceRange FileRange = Lexer::makeFileCharRange(
      CharSourceRange::getTokenRange(S->getSourceRange()), SM,
      Context->getLangOpts());
  if (FileRange.isInvalid())
    return false;

  // Convert InitialLoc to file location, if it's on the same macro expansion
  // level as the start of the statement. We also need file locations for
  // Lexer::getLocForEndOfToken working properly.
  InitialLoc = Lexer::makeFileCharRange(
                   CharSourceRange::getCharRange(InitialLoc, S->getBeginLoc()),
                   SM, Context->getLangOpts())
                   .getBegin();
if (InitialLoc.isInvalid())
    return false;
  SourceLocation StartLoc =
      Lexer::getLocForEndOfToken(InitialLoc, 0, SM, Context->getLangOpts());

  // StartLoc points at the location of the opening brace to be inserted.
  SourceLocation EndLoc;
  std::string ClosingInsertion;
  if (EndLocHint.isValid()) {
    EndLoc = EndLocHint;
    ClosingInsertion = "} ";
  } else {
    const auto FREnd = FileRange.getEnd().getLocWithOffset(-1);
    EndLoc = findEndLocation(FREnd, SM, Context);
    ClosingInsertion = "\n}";
  }

  assert(StartLoc.isValid());
  assert(EndLoc.isValid());
	Rewrite.InsertText(StartLoc, " {", true, true); 
	Rewrite.InsertText(EndLoc, ClosingInsertion, true, true); 
  return true;
}

  FloatVarDeclHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
		const SourceManager &SM = *Result.SourceManager;
  	const ASTContext *Context = Result.Context;	
		if (auto S = Result.Nodes.getNodeAs<ForStmt>("for")) {
			checkStmt(Result, S->getBody(), S->getRParenLoc());
		} else if (auto S = Result.Nodes.getNodeAs<CXXForRangeStmt>("for-range")) {
			checkStmt(Result, S->getBody(), S->getRParenLoc());
		} else if (auto S = Result.Nodes.getNodeAs<DoStmt>("do")) {
			checkStmtWithLoc(Result, S->getBody(), S->getDoLoc(), S->getWhileLoc());
		} else if (auto S = Result.Nodes.getNodeAs<WhileStmt>("while")) {
			SourceLocation StartLoc = findRParenLoc(S, SM, Context);
			if (StartLoc.isInvalid())
				return;
			checkStmt(Result, S->getBody(), StartLoc);	
		} else if (auto S = Result.Nodes.getNodeAs<IfStmt>("if")) {
			SourceLocation StartLoc = findRParenLoc(S, SM, Context);
			if (StartLoc.isInvalid())
				return;
			llvm::errs()<<"found if***\n";
			BinIfLoc.insert(std::pair<const IfStmt*, SourceLocation>(S, S->getSourceRange().getBegin()));	
			if (ForceBracesStmts.erase(S))
				ForceBracesStmts.insert(S->getThen());
			bool BracedIf = checkStmtWithLoc(Result, S->getThen(), StartLoc, S->getElseLoc());
			const Stmt *Else = S->getElse();
			if (Else && BracedIf)
				ForceBracesStmts.insert(Else);
			if (Else && !isa<IfStmt>(Else)) {
			// Omit 'else if' statements here, they will be handled directly.	
				checkStmt(Result, Else, S->getElseLoc());
			}
		}
		if (auto S = Result.Nodes.getNodeAs<ConditionalOperator>("ifcond")) {
			if (auto BinOp = Result.Nodes.getNodeAs<BinaryOperator>("cond")) {
				llvm::errs()<<"condition......\n";
				std::string func = getPositBinOp(BinOp->getOpcode());
				std::string Op1 = handleOperand(BinOp, S->getSourceRange().getBegin(), BinOp->getLHS());
				std::string Op2 = handleOperand(BinOp, S->getSourceRange().getBegin(), BinOp->getRHS());
				llvm::errs()<<"bin if Op1:"<<Op1<<" Op2:"<<Op2<<"\n";
				Rewrite.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
				Rewrite.ReplaceText(BinOp->getSourceRange(), func+"("+Op1+","+Op2+")");
				Rewrite.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
			}
		}
		if (auto S = Result.Nodes.getNodeAs<IfStmt>("ifcond")) {
			if (auto BinOp = Result.Nodes.getNodeAs<BinaryOperator>("cond")) {
				std::string func = getPositBinOp(BinOp->getOpcode());
				std::string Op1 = handleOperand(BinOp, S->getSourceRange().getBegin(), BinOp->getLHS());
				std::string Op2 = handleOperand(BinOp, S->getSourceRange().getBegin(), BinOp->getRHS());
				llvm::errs()<<"bin if Op1:"<<Op1<<" Op2:"<<Op2<<"\n";
				Rewrite.ReplaceText(BinOp->getSourceRange(), func+"("+Op1+","+Op2+")");
			}
		}
		if (auto S = Result.Nodes.getNodeAs<IfStmt>("curif")) {
			llvm::errs()<<"matched if\n";
			llvm::errs()<<"****\n";
			auto topIf = Result.Nodes.getNodeAs<IfStmt>("topif");
			auto itr = BinIfLoc.find(S); 
     	if (itr != BinIfLoc.end()){
          (*itr).second = topIf->getSourceRange().getBegin();
     	}
			else
				BinIfLoc.insert(std::pair<const IfStmt*, SourceLocation>(S, topIf->getSourceRange().getBegin()));	
		}
		if (const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("boassignvd")){
			llvm::errs()<<"boassignvd\n";
			std::string opName;
      llvm::raw_string_ostream stream(opName);
      BO->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
			llvm::errs()<<opName<<"\n";
			const FloatingLiteral *FL = Result.Nodes.getNodeAs<clang::FloatingLiteral>("floatliteral");
			const IntegerLiteral *IL = Result.Nodes.getNodeAs<clang::IntegerLiteral>("intliteral");
			if(FL){
				std::string positLiteral = convertFloatToPosit(FL);
				Rewrite.ReplaceText(SourceRange(FL->getSourceRange().getBegin(), FL->getSourceRange().getEnd()), positLiteral);
			}
			if(IL){
				std::string positLiteral = convertIntToPosit(IL);
				Rewrite.ReplaceText(SourceRange(IL->getSourceRange().getBegin(), IL->getSourceRange().getEnd()), positLiteral);
			}
		}
		if (const IntegerLiteral *IL = Result.Nodes.getNodeAs<clang::IntegerLiteral>("initintegerliteral")){
			llvm::errs()<<"initintegerliteral\n";
			const InitListExpr *ILE = Result.Nodes.getNodeAs<clang::InitListExpr>("init");
			const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vd_literal");

			//initListExpr is visited twice, need to keep a set of visited nodes
			SmallVector<const IntegerLiteral*, 8>::iterator it;
			it = std::find(ProcessedIL.begin(), ProcessedIL.end(), IL);		
			if(it != ProcessedIL.end())
				return;
			
			ProcessedIL.push_back(IL);
			if(ILE){
				std::string temp;
    		temp = getTempDest();
				std::string positLiteral = " = "+convertIntToPosit(IL);
				Rewrite.InsertText(VD->getSourceRange().getBegin(), 
					PositTY+temp+ positLiteral+"\n", true, true);
      	SourceManager &SM = Rewrite.getSourceMgr();
        const char *Buf = SM.getCharacterData(IL->getSourceRange().getBegin());
        int Offset = 0;
        while (*Buf != ';') {
					if(*Buf == ',')
						break;
					if(*Buf == '}')
						break;
					Buf++;
          Offset++;
        }
        const char *Buf1 = SM.getCharacterData(IL->getSourceRange().getBegin().getLocWithOffset(Offset));
				Rewrite.ReplaceText(IL->getSourceRange().getBegin(), Offset, temp);
			}
		}
		if(const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("callfunc2")){
			const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vd");
			SourceLocation StartLoc, EndLoc;
			std::string VName = "";

			SmallVector<const CallExpr*, 8>::iterator it;
			it = std::find(ProcessedCE.begin(), ProcessedCE.end(), CE);		
			if(it != ProcessedCE.end())
				return;
			
			llvm::errs()<<"callfloatliteral2\n";
			ProcessedCE.push_back(CE);

			for(int i=0, j=CE->getNumArgs(); i<j; i++){
				if(isPointerToFloatingType(CE->getArg(i)->getType().getTypePtr())){
					VName = VD->getNameAsString();
					StartLoc = VD->getSourceRange().getBegin();
					EndLoc = VD->getSourceRange().getEnd();
					std::string temp;
					temp = getTempDest();
					llvm::errs()<<"callfunc...BO\n";
					Rewrite.ReplaceText(StartLoc, 6, PositTY);
				}
    	}
		}
		if(const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("callfunc3")){
			const FloatingLiteral *FL = Result.Nodes.getNodeAs<clang::FloatingLiteral>("callfloatliteral");
			const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("call_binop");
			const CompoundStmt *CS = Result.Nodes.getNodeAs<clang::CompoundStmt>("call_stmt");
			SourceLocation StartLoc, EndLoc;

			SmallVector<const CallExpr*, 8>::iterator it;
			it = std::find(ProcessedCE.begin(), ProcessedCE.end(), CE);		
			if(it != ProcessedCE.end())
				return;
			
			llvm::errs()<<"callfloatliteral3\n";
			ProcessedCE.push_back(CE);

			for(int i=0, j=CE->getNumArgs(); i<j; i++){
				if(isPointerToFloatingType(CE->getArg(i)->getType().getTypePtr())){
					if(BO){
						StartLoc = BO->getLHS()->getSourceRange().getBegin();
						EndLoc = BO->getLHS()->getSourceRange().getEnd();
					}
					else if(CS){
						StartLoc = CE->getSourceRange().getBegin();
						EndLoc = CE->getSourceRange().getEnd();
					}
					else{
						StartLoc = CE->getArg(i)->getSourceRange().getBegin();
						EndLoc = CE->getArg(i)->getSourceRange().getEnd();
					}
					llvm::errs()<<"callfunc...else\n";
					std::string temp;
					temp = getTempDest();
					std::string op  = convertFloatToPosit(FL);
					Rewrite.InsertText(StartLoc,
                  PositTY+temp +" = "+op+"\n", true, true);                                                                                                                 
					Rewrite.ReplaceText(SourceRange(CE->getArg(i)->getSourceRange().getBegin(), 
																					CE->getArg(i)->getSourceRange().getEnd()), temp);
				}
			}	
		}
		if(const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("callfunc1")){
			const FloatingLiteral *FL = Result.Nodes.getNodeAs<clang::FloatingLiteral>("callfloatliteral");
			const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vd");
			SourceLocation StartLoc, EndLoc;
			std::string VName = "";

			SmallVector<const CallExpr*, 8>::iterator it;
			it = std::find(ProcessedCE.begin(), ProcessedCE.end(), CE);		
			if(it != ProcessedCE.end())
				return;
			
			llvm::errs()<<"callfloatliteral1\n";
			ProcessedCE.push_back(CE);

			for(int i=0, j=CE->getNumArgs(); i<j; i++){
				if(isPointerToFloatingType(CE->getArg(i)->getType().getTypePtr())){
					VName = VD->getNameAsString();
					StartLoc = VD->getSourceRange().getBegin();
					EndLoc = VD->getSourceRange().getEnd();
					std::string temp;
					temp = getTempDest();
					llvm::errs()<<"callfunc...VD\n";
					std::string op  = convertFloatToPosit(FL);
					Rewrite.InsertText(StartLoc, 
									PositTY+temp +" = "+op+"\n", true, true);		
          Rewrite.ReplaceText(SourceRange(CE->getArg(i)->getSourceRange().getBegin(), 
													CE->getArg(i)->getSourceRange().getEnd()), temp);
					Rewrite.ReplaceText(StartLoc, 6, PositTY);
				}
    	}
		}
		if(const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("callfunc")){
			const FloatingLiteral *FL = Result.Nodes.getNodeAs<clang::FloatingLiteral>("callfloatliteral");
			const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("call_binop");
			SourceLocation StartLoc, EndLoc;

			SmallVector<const CallExpr*, 8>::iterator it;
			it = std::find(ProcessedCE.begin(), ProcessedCE.end(), CE);		
			if(it != ProcessedCE.end())
				return;
			
			llvm::errs()<<"callfloatliteral0\n";
			ProcessedCE.push_back(CE);

			for(int i=0, j=CE->getNumArgs(); i<j; i++){
				if(isPointerToFloatingType(CE->getArg(i)->getType().getTypePtr())){
					StartLoc = BO->getSourceRange().getBegin();
					EndLoc = BO->getSourceRange().getEnd();
					std::string temp;
					temp = getTempDest();
					std::string op  = convertFloatToPosit(FL);
					Rewrite.InsertText(StartLoc, 
									PositTY+temp +" = "+op+"\n", true, true);		
					llvm::errs()<<"callfunc...BO\n";
          Rewrite.ReplaceText(SourceRange(CE->getArg(i)->getSourceRange().getBegin(), 
													CE->getArg(i)->getSourceRange().getEnd()), temp);
				}
    	}
		}
		if (const FloatingLiteral *FL = Result.Nodes.getNodeAs<clang::FloatingLiteral>("finit")){
			llvm::errs()<<"finit\n";
		}
		if (const FloatingLiteral *FL = Result.Nodes.getNodeAs<clang::FloatingLiteral>("initfloatliteral_vd")){
			llvm::errs()<<"initfloatliteral\n";
			const InitListExpr *ILE = Result.Nodes.getNodeAs<clang::InitListExpr>("init");
			const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("init_literal");
			const IntegerLiteral *IL = Result.Nodes.getNodeAs<clang::IntegerLiteral>("intliteral");

			//initListExpr is visited twice, need to keep a set of visited nodes
			SmallVector<const FloatingLiteral*, 8>::iterator it;
			it = std::find(ProcessedFL.begin(), ProcessedFL.end(), FL);		
			if(it != ProcessedFL.end())
				return;
			
			ProcessedFL.push_back(FL);
			if(ILE){
				std::string temp;
    		temp = getTempDest();
				std::string positLiteral = " = "+convertFloatToPosit(FL);
				Rewrite.InsertText(VD->getSourceRange().getBegin(), 
					PositTY+temp+ positLiteral+"\n", true, true);
      	SourceManager &SM = Rewrite.getSourceMgr();
        const char *Buf = SM.getCharacterData(FL->getSourceRange().getBegin());
        int Offset = 0;
        while (*Buf != ';') {
					if(*Buf == ',')
						break;
					if(*Buf == '}')
						break;
					Buf++;
          Offset++;
        }
        const char *Buf1 = SM.getCharacterData(FL->getSourceRange().getBegin().getLocWithOffset(Offset));
				Rewrite.ReplaceText(FL->getSourceRange().getBegin(), Offset, temp);

        const char *BufVD = SM.getCharacterData(VD->getSourceRange().getBegin());
        int VDOffset = 0;
        while (*BufVD != ';') {
					if(*BufVD == ' ')
						break;
					if(*BufVD == '*')
						break;
					BufVD++;
          VDOffset++;
        }
				SmallVector<const VarDecl*, 8>::iterator it;
				it = std::find(ProcessedVD.begin(), ProcessedVD.end(), VD);		
				if(it == ProcessedVD.end()){
					Rewrite.ReplaceText(VD->getSourceRange().getBegin(), VDOffset, PositTY);
					ProcessedVD.push_back(VD);
				}
			}
		}
		if (const FloatingLiteral *FL = Result.Nodes.getNodeAs<clang::FloatingLiteral>("initfloatliteral")){
			const InitListExpr *ILE = Result.Nodes.getNodeAs<clang::InitListExpr>("init");
			const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("init_literal");
			const IntegerLiteral *IL = Result.Nodes.getNodeAs<clang::IntegerLiteral>("intliteral");

			SmallVector<const InitListExpr*, 8>::iterator ite;
			ite = std::find(ProcessedILE.begin(), ProcessedILE.end(), ILE);
			if(ite == ProcessedILE.end()){
				Rewrite.InsertText(VD->getSourceRange().getBegin(), 
					"__attribute__ ((constructor)) void __init_"+VD->getNameAsString()+"(void){\n", true, true);
			}
			//initListExpr is visited twice, need to keep a set of visited nodes
			SmallVector<const FloatingLiteral*, 8>::iterator it;
			it = std::find(ProcessedFL.begin(), ProcessedFL.end(), FL);		
			if(it != ProcessedFL.end())
				return;
			
			ProcessedFL.push_back(FL);

			std::string temp;
    	temp = getTempDest();
			std::string positLiteral = " = "+convertFloatToPosit(FL);
			Rewrite.InsertText(VD->getSourceRange().getBegin(), 
					"        "+PositTY+temp+ positLiteral+"\n", true, true);
			SourceManager &SM = Rewrite.getSourceMgr();
			const char *Buf = SM.getCharacterData(FL->getSourceRange().getBegin());
			int Offset = 0;
			while (*Buf != ';') {
				if(*Buf == ',')
					break;
				if(*Buf == '}')
					break;
				Buf++;
				Offset++;
			}
			const char *Buf1 = SM.getCharacterData(FL->getSourceRange().getBegin().getLocWithOffset(Offset));
			Rewrite.ReplaceText(FL->getSourceRange().getBegin(), Offset, temp);

			const char *BufVD = SM.getCharacterData(VD->getSourceRange().getBegin());
			int VDOffset = 0;
			while (*BufVD != ';') {
				if(*BufVD == ' ')
					break;
				if(*BufVD == '*')	
					break;
				BufVD++;
				VDOffset++;
			}
			SmallVector<const VarDecl*, 8>::iterator itvd;
			itvd = std::find(ProcessedVD.begin(), ProcessedVD.end(), VD);		
			if(itvd == ProcessedVD.end()){
				Rewrite.ReplaceText(VD->getSourceRange().getBegin(), VDOffset, "       ");
				ProcessedVD.push_back(VD);
			}
			if(ite == ProcessedILE.end()){
				Rewrite.InsertTextAfterToken(VD->getSourceRange().getEnd().getLocWithOffset(1), 
					"\n}\n");
				
				std::string arrayDim = getArrayDim(VD);
				Rewrite.InsertTextAfterToken(VD->getSourceRange().getEnd().getLocWithOffset(1), 
					 PositTY+" "+VD->getNameAsString()+arrayDim);
				ProcessedILE.push_back(ILE);
			}
		}
		if (const UnaryExprOrTypeTraitExpr *UE = Result.Nodes.getNodeAs<clang::UnaryExprOrTypeTraitExpr>("unary")){
			llvm::errs()<<"unary\n";
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

        Rewrite.ReplaceText(UE->getSourceRange().getBegin().getLocWithOffset(StartOffset), Offset, "posit32_t");
      }
		}
		if (const FieldDecl *FD = Result.Nodes.getNodeAs<clang::FieldDecl>("struct")){
			llvm::errs()<<"struct\n";
			ReplaceVDWithPosit(FD->getSourceRange().getBegin(), FD->getSourceRange().getEnd(), FD->getNameAsString()+";");
		}
		if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vd_literal")){
			llvm::errs()<<"vd_literal\n";
			const FloatingLiteral *FL = Result.Nodes.getNodeAs<clang::FloatingLiteral>("floatliteral");
			const IntegerLiteral *IL = Result.Nodes.getNodeAs<clang::IntegerLiteral>("intliteral");
			if(FL != NULL){
				std::string positLiteral = VD->getNameAsString()+" = "+convertFloatToPosit(FL);
				ReplaceVDWithPosit(VD->getSourceRange().getBegin(), VD->getSourceRange().getEnd(), positLiteral);
			}
			else if(IL != NULL){
				std::string positLiteral = VD->getNameAsString()+" = "+convertIntToPosit(IL);
				ReplaceVDWithPosit(VD->getSourceRange().getBegin(), VD->getSourceRange().getEnd(), positLiteral);
			}
		}
		if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclnoinit")){
			llvm::errs()<<"vardeclnoinit\n";
			const ParmVarDecl *PD = dyn_cast<ParmVarDecl>(VD);
  		if (PD) {
				ReplaceParmVDWithPosit(VD->getSourceRange().getBegin(), ' ');
  		}
			else{
				ReplaceVDWithPosit(VD->getSourceRange().getBegin(), VD->getSourceRange().getEnd(), VD->getNameAsString()+";");
			}
		}
		if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclarray")){
			llvm::errs()<<"vardeclarray\n";
			const Type *Ty = VD->getType().getTypePtr();
			while (Ty->isArrayType()) {                                                             
    		const ArrayType *AT = dyn_cast<ArrayType>(Ty);
    		Ty = AT->getElementType().getTypePtr();
  		}
			if(!Ty->isFloatingType())
				return;

			std::string arrayDim = getArrayDim(VD);
				Rewrite.ReplaceText(VD->getSourceRange().getBegin(), 6, PositTY);
			//ReplaceVDWithPosit(VD->getSourceRange().getBegin(), VD->getSourceRange().getEnd(), VD->getNameAsString()+arrayDim+"\n");
		}
		if (const FunctionDecl *FD = Result.Nodes.getNodeAs<clang::FunctionDecl>("returnfp")){
			if(!isPointerToFloatingType(FD->getReturnType().getTypePtr()))
				return;

			DeclarationNameInfo NameInfo = FD->getNameInfo();
			SourceLocation NameInfoStartLoc = NameInfo.getBeginLoc();

			SourceRange FuncDefRange = FD->getSourceRange();
			SourceLocation FuncStartLoc = FuncDefRange.getBegin();
  
      SourceManager &SM = Rewrite.getSourceMgr();
			const char *FuncStartBuf = SM.getCharacterData(FuncStartLoc);

			const char *NameInfoStartBuf = SM.getCharacterData(NameInfoStartLoc);

			if (FuncStartBuf == NameInfoStartBuf)
				return ;

			int Offset = NameInfoStartBuf - FuncStartBuf;

			NameInfoStartBuf--;
			while ((*NameInfoStartBuf == '(') || (*NameInfoStartBuf == ' ') ||
         (*NameInfoStartBuf == '*') || (*NameInfoStartBuf == '\n') || (*NameInfoStartBuf == '\n')) {
				Offset--;
				NameInfoStartBuf--;
			}

			if(FD->getReturnType().getTypePtr()->isPointerType())
				ReplaceParmVDWithPosit(FuncStartLoc, '*');
			else
				ReplaceParmVDWithPosit(FuncStartLoc, ' ');
		}
		if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclpointer")){
			llvm::errs()<<"vardeclpointer\n";
			if(!isPointerToFloatingType(VD->getType().getTypePtr()))
				return;
			const ParmVarDecl *PD = dyn_cast<ParmVarDecl>(VD);
  		if (PD) {
				//for function parameter end string will be either ',' or ')'
				//we want to replace double with posit, instead of creating a new variable
				ReplaceParmVDWithPosit(VD->getSourceRange().getBegin(), '*');
  		}
			else{
        Rewrite.ReplaceText(VD->getSourceRange().getBegin(), 6, "posit32_t");
			}
		}
		if(const FunctionDecl *FD = Result.Nodes.getNodeAs<clang::FunctionDecl>("addheader")){
			llvm::errs()<<"addheader\n";
		//	if(SrcManager->getFileID(Loc) != SrcManager->getMainFileID())	
			insertHeader(FD->getSourceRange().getBegin());	
			
		}
		if(const IfStmt *IFStmt = Result.Nodes.getNodeAs<clang::IfStmt>("ifelse")){
			const Stmt *IF = Result.Nodes.getNodeAs<clang::Stmt>("else");
			if(IF){
		//		SourceLocation StartLoc = IF->getSourceRange().getBegin();
		//		Rewrite.InsertText(StartLoc, 
			//						"{\n", true, true);	//close if	
				llvm::errs()<<"else start....\n";
				SourceLocation StartLoc = IF->getSourceRange().getBegin();
			SourceManager &SM = Rewrite.getSourceMgr();
			const char *StartBuf = SM.getCharacterData(StartLoc);
			unsigned StartOffset = 0;	
			llvm::errs()<<"StartBuf:"<<StartBuf<<"\n";
				llvm::errs()<<"else end....\n";
				SourceLocation EndLoc = IF->getSourceRange().getEnd();
			const char *EndBuf = SM.getCharacterData(EndLoc.getLocWithOffset(-1));
			llvm::errs()<<"EndBuf:"<<EndBuf<<"\n";
			while (*StartBuf != 'e' ){
    		StartBuf--;
				StartOffset--;
			}
			llvm::errs()<<"StartBuf:"<<*StartBuf<<*(StartBuf-1)<<*(StartBuf-2)<<"*************\n";
			std::string IndentStr = getStmtIndentString(IFStmt->getSourceRange().getBegin());
			Rewrite.InsertTextAfterToken(StartLoc.getLocWithOffset(StartOffset), 
									"\n"+IndentStr+"{");		
			Rewrite.InsertTextAfterToken(EndLoc.getLocWithOffset(1), 
									"\n"+IndentStr+"}");		
			}
		}
		if(const IfStmt *IFStmt = Result.Nodes.getNodeAs<clang::IfStmt>("ifthen")){
			const Stmt *IF = Result.Nodes.getNodeAs<clang::Stmt>("then");
			if(IF){
				llvm::errs()<<"then start....\n";
				SourceLocation StartLoc = IF->getSourceRange().getBegin();
			SourceManager &SM = Rewrite.getSourceMgr();
			const char *StartBuf = SM.getCharacterData(StartLoc);
			unsigned StartOffset = 0;	
			llvm::errs()<<"StartBuf:"<<StartBuf<<"\n";
				llvm::errs()<<"then end....\n";
				SourceLocation EndLoc = IF->getSourceRange().getEnd();
			const char *EndBuf = SM.getCharacterData(EndLoc.getLocWithOffset(-1));
			llvm::errs()<<"EndBuf:"<<EndBuf<<"\n";
			while (*StartBuf != ')' ){
    		StartBuf--;
				StartOffset--;
			}
			std::string IndentStr = getStmtIndentString(IFStmt->getSourceRange().getBegin());
			Rewrite.InsertTextAfterToken(StartLoc.getLocWithOffset(StartOffset), 
									"\n"+IndentStr+"{");		
			Rewrite.InsertTextAfterToken(EndLoc.getLocWithOffset(1), 
									"\n"+IndentStr+"}");		
			}
		}
		if(const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("printfsqrt")){
			llvm::errs()<<"printfsqrt......\n";
      SourceManager &SM = Rewrite.getSourceMgr();
			for(int i=0, j=CE->getNumArgs(); i<j; i++)
    	{
        std::string TypeS;
        llvm::raw_string_ostream s(TypeS);
        CE->getArg(i)->printPretty(s, 0, PrintingPolicy(LangOptions()));
				if(isPointerToFloatingType(CE->getArg(i)->getType().getTypePtr())){
					const BinaryOperator *BO = dyn_cast<clang::BinaryOperator>(CE->getArg(i));
					std::string Op, convert;
					if(!BO){
						convert = PositPtoD+"(" + s.str()+");";
						std::string temp;
						temp = getTempDest();
						Rewrite.InsertText(CE->getSourceRange().getBegin(), 
									"double "+temp +" = "+convert+"\n", true, true);		
        		const char *Buf = SM.getCharacterData(CE->getArg(i)->getSourceRange().getBegin());
        		int StartOffset = 0, openB = 0, closeB = 0;
        		while (*Buf != ';') {
							llvm::errs()<<*Buf;
							if(*Buf == '(')
								openB++;
							if(*Buf == ')')
								closeB++;
							if(*Buf == ',' && (openB == closeB))
								break;
							if(*Buf == ')' && *(Buf+1) == ';')
								break;
          		Buf++;
          		StartOffset++;
        		}
						Rewrite.ReplaceText(CE->getArg(i)->getSourceRange().getBegin(), StartOffset, temp);
					}
				}
    	}
		}
		if(const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("callsqrt")){
			llvm::errs()<<"callsqrt\n";
			const FunctionDecl *Func = CE->getDirectCallee();
			const std::string oldFuncName = Func->getNameInfo().getAsString();
			std::string funcName = handleMathFunc(oldFuncName);
			Rewrite.ReplaceText(CE->getSourceRange().getBegin(), oldFuncName.size(), funcName);
		}
		if(const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclparent")){
			llvm::errs()<<"vardeclparent\n";
			const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("op");
			BinParentVD.insert(std::pair<const BinaryOperator*, const VarDecl*>(BO, VD));	
		}
		if(const Stmt *ST = Result.Nodes.getNodeAs<clang::Stmt>("returnparent")){
			llvm::errs()<<"returnparent\n";
			const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("op");
			BinParentST.insert(std::pair<const BinaryOperator*, const Stmt*>(BO, ST));	
		}
		if(const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("callexprparent")){
			llvm::errs()<<"callexprparent\n";
			const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("op");
			BinParentCE.insert(std::pair<const BinaryOperator*, const CallExpr*>(BO, CE));	
		}
		if (const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("boassign")){
			const FloatingLiteral *FL = Result.Nodes.getNodeAs<clang::FloatingLiteral>("binfloat");
			const IntegerLiteral *IL = Result.Nodes.getNodeAs<clang::IntegerLiteral>("binint");
			if(FL != NULL){
				std::string positLiteral = convertFloatToPosit(FL);
				Rewrite.ReplaceText(SourceRange(BO->getRHS()->getSourceRange().getBegin(), BO->getRHS()->getSourceRange().getEnd()), positLiteral);
			}
			if(IL != NULL){
				std::string positLiteral = convertIntToPosit(IL);
				Rewrite.ReplaceText(SourceRange(BO->getRHS()->getSourceRange().getBegin(), BO->getRHS()->getSourceRange().getEnd()), positLiteral);
			}
		}
		if (const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("fadd_be")){
			llvm::errs()<<"fadd_be\n";
			const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("callbo");
			const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclbo");
			const BinaryOperator *BA = Result.Nodes.getNodeAs<clang::BinaryOperator>("bobo");
			const Stmt *ST = Result.Nodes.getNodeAs<clang::ReturnStmt>("returnbo");
			auto IfST = Result.Nodes.getNodeAs<IfStmt>("ifstmtbo");
			//We need to handle deepest node first in AST, but there is no way to traverse AST from down to up.
			//We store all binaryoperator in stack.
			//Handle all binop in stack in handleBinOp
			//But there is a glitch, we need to know the location of parent node of binaryoperator
			//It would be better to store a object of binop and location of its parent node in stack, 
			//for now we are storing it in a seperate map
			//We also need to know source location range of parent node to remove it
			if(CE){
				llvm::errs()<<"CE loc\n";
				SourceManager &SM = Rewrite.getSourceMgr();
        SourceLocation StartLoc = CE->getSourceRange().getBegin();
        if (StartLoc.isMacroID()) {
          StartLoc = SM.getFileLoc(StartLoc);
        }
				BinLoc_Temp.insert(std::pair<const BinaryOperator*, SourceLocation>(BO, StartLoc));	
			}
			if(VD){
				llvm::errs()<<"VD loc\n";
				SourceManager &SM = Rewrite.getSourceMgr();
        SourceLocation StartLoc = VD->getSourceRange().getBegin();
        if (StartLoc.isMacroID()) {
          StartLoc = SM.getFileLoc(StartLoc);
        }

				BinLoc_Temp.insert(std::pair<const BinaryOperator*, SourceLocation>(BO, StartLoc));	
			}
			if(BA){
				llvm::errs()<<"BA loc\n";
				SourceManager &SM = Rewrite.getSourceMgr();
				SourceLocation StartLoc = BA->getSourceRange().getBegin();
				if (StartLoc.isMacroID()) {
    			StartLoc = SM.getFileLoc(StartLoc); 
				}
				BinLoc_Temp.insert(std::pair<const BinaryOperator*, SourceLocation>(BO, StartLoc));	
				BinParentBO.insert(std::pair<const BinaryOperator*, const BinaryOperator*>(BO, BA));	
			}
			if(IfST){
				llvm::errs()<<"If ST loc\n";
				llvm::errs()<<"found if***\n";
				SourceLocation StartLoc = findRParenLoc(IfST, SM, Context);
				BinParentST.insert(std::pair<const BinaryOperator*, const Stmt*>(BO, IfST));	
				BinLoc_Temp.insert(std::pair<const BinaryOperator*, SourceLocation>(BO, StartLoc));	
			}
			if(ST){
				llvm::errs()<<"ST loc\n";
				BinLoc_Temp.insert(std::pair<const BinaryOperator*, SourceLocation>(BO, ST->getSourceRange().getBegin()));	
			}
			else
				BinLoc_Temp.insert(std::pair<const BinaryOperator*, SourceLocation>(BO, BO->getSourceRange().getBegin()));	
			//check if this binaryoperator is processed
			//check if LHS is processed, get its temp variable
			//check if RHS is processed, get its temp variable
			//create new varible, if this BO doesnt have a VD as its LHS
			//push this in map
			SmallVector<const BinaryOperator*, 8>::iterator it;
			it = std::find(ProcessedBO.begin(), ProcessedBO.end(), BO);		
			if(it != ProcessedBO.end())
				return;
			
			ProcessedBO.push_back(BO);
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

	//check if there are open braces around statement, if not insert them
	//rewritten using clang-tidy BracesAroundStatementsCheck
	Matcher.addMatcher(ifStmt().bind("if"), &HandlerFloatVarDecl);
	Matcher.addMatcher(ifStmt(hasAncestor(ifStmt(unless(hasAncestor(ifStmt()))).bind("topif"))).bind("curif"), &HandlerFloatVarDecl);
	Matcher4.addMatcher(ifStmt(hasCondition(ignoringParenImpCasts(ignoringParens(
		binaryOperator(hasEitherOperand(hasType(realFloatingPointType()))).bind("cond"))))).bind("ifcond"), &HandlerFloatVarDecl);
	Matcher4.addMatcher(conditionalOperator(
			hasCondition(ignoringParenImpCasts(ignoringParens(
				binaryOperator(hasEitherOperand(
					hasType(realFloatingPointType()))).bind("cond"))))).bind("ifcond"), &HandlerFloatVarDecl);
	//Matcher4.addMatcher(conditionalOperator(hasCondition()).bind("ifcond"), &HandlerFloatVarDecl);
  Matcher.addMatcher(whileStmt().bind("while"), &HandlerFloatVarDecl);
  Matcher.addMatcher(doStmt().bind("do"), &HandlerFloatVarDecl);
  Matcher.addMatcher(forStmt().bind("for"), &HandlerFloatVarDecl);
  Matcher.addMatcher(cxxForRangeStmt().bind("for-range"), &HandlerFloatVarDecl);
	
	//matcher for  double x = 3.4, y = 5.6;
	//double sum = z;
	Matcher.addMatcher(
		varDecl(hasType(realFloatingPointType()), 
			unless(hasDescendant(binaryOperator(hasOperatorName("*")))), 
				anyOf(hasInitializer(ignoringParenImpCasts(
					integerLiteral().bind("intliteral"))), 
						hasInitializer(ignoringParenImpCasts(
          		floatLiteral().bind("floatliteral"))), 
							hasDescendant(binaryOperator(hasOperatorName("="))))).bind("vd_literal"), &HandlerFloatVarDecl);
/*
	//double x[2] = {2.3, 3.4}
	Matcher.addMatcher(
			floatLiteral(hasAncestor(initListExpr(
					unless(hasAncestor(initListExpr().bind("topinit"))), hasAncestor(varDecl().
						bind("init_literal"))).bind("init"))).
							bind("initfloatliteral"), &HandlerFloatVarDecl);
*/
	Matcher.addMatcher(
			floatLiteral(hasAncestor(initListExpr(unless(hasAncestor(initListExpr().bind("init")))).bind("init")), 
				hasAncestor(varDecl(hasLocalStorage()).                                                                                       
            bind("init_literal"))).
				bind("initfloatliteral_vd"), &HandlerFloatVarDecl);
	Matcher.addMatcher(
			floatLiteral(hasAncestor(initListExpr(unless(hasAncestor(initListExpr().bind("init")))).bind("init")), 
				hasAncestor(varDecl(hasGlobalStorage()).                                                                                       
            bind("init_literal"))).
				bind("initfloatliteral"), &HandlerFloatVarDecl);

	//double x;
	//x = foo(-2.0)
		Matcher4.addMatcher(
			callExpr(hasDescendant(floatLiteral().bind("callfloatliteral")),
				hasAncestor(binaryOperator(hasOperatorName("=")).bind("call_binop")), 
				unless(hasAncestor(varDecl(hasType(realFloatingPointType())))), 
					unless(hasDescendant(binaryOperator()))).bind("callfunc3"), &HandlerFloatVarDecl);
	//foo(-2.0)
		Matcher4.addMatcher(
			callExpr(hasDescendant(floatLiteral().bind("callfloatliteral")),
				hasAncestor(compoundStmt().bind("call_stmt")), 
				unless(hasAncestor(binaryOperator(hasOperatorName("=")))), 
				unless(hasAncestor(varDecl(hasType(realFloatingPointType())))), 
					unless(hasDescendant(binaryOperator()))).bind("callfunc3"), &HandlerFloatVarDecl);
	//double x = foo(-2.0/3.4)
		Matcher4.addMatcher(
			callExpr(hasAncestor(varDecl(hasType(realFloatingPointType())).bind("vd")), 
					unless(hasAncestor(binaryOperator())),
					hasDescendant(binaryOperator().bind("call_bo"))).bind("callfunc2"), &HandlerFloatVarDecl);
	//double x = foo(-2.0)
		Matcher4.addMatcher(
			callExpr(hasAncestor(varDecl(hasType(realFloatingPointType())).bind("vd")), 
				hasDescendant(floatLiteral().bind("callfloatliteral")), 
					unless(hasDescendant(binaryOperator()))).bind("callfunc1"), &HandlerFloatVarDecl);

	//double x;
	//x = foo(-2.0)
/*
		Matcher4.addMatcher(
			callExpr(hasDescendant(floatLiteral().bind("callfloatliteral")), 
				unless(hasAncestor(varDecl(hasType(realFloatingPointType())))), 
				hasAncestor(binaryOperator(hasOperatorName("=")).bind("call_binop"))).
						bind("callfunc"), &HandlerFloatVarDecl);
*/
		Matcher.addMatcher(
			integerLiteral(hasAncestor(initListExpr(hasAncestor(varDecl(hasType(realFloatingPointType())).
					bind("vd_literal"))).bind("init"))).
						bind("initintegerliteral"), &HandlerFloatVarDecl);
		
		//matcher for  double x, y;
		//ignores double x = x + y;
		Matcher.addMatcher(
			varDecl(hasType(realFloatingPointType()), unless( hasInitializer(floatLiteral())), 		
				unless(hasType(arrayType())), 
					unless( hasInitializer(ignoringParenImpCasts(integerLiteral()))), 
						unless(hasDescendant(callExpr())),
							unless(hasDescendant(binaryOperator()))).
								bind("vardeclnoinit"), &HandlerFloatVarDecl);


		//pointer
		Matcher.addMatcher(
			varDecl(hasType(pointerType())). 
					bind("vardeclpointer"), &HandlerFloatVarDecl);

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

		//return f;
		Matcher.addMatcher(
			functionDecl(returns(anyOf(realFloatingPointType(), pointerType()))). 
					bind("returnfp"), &HandlerFloatVarDecl);

		//sizeof(double)
		Matcher.addMatcher(
			unaryExprOrTypeTraitExpr(ofKind(UETT_SizeOf)).
				bind("unary"), &HandlerFloatVarDecl);

		//add softposit.h
		Matcher.addMatcher(
			functionDecl(functionDecl(hasName("main"))).bind("addheader")
					, &HandlerFloatVarDecl);

		//sqrt => p32_sqrt
		Matcher.addMatcher(
			callExpr(callee(functionDecl(anyOf(hasName("sqrt"), hasName("cos"), 
				hasName("sin"), hasName("tan"))))).
					bind("callsqrt"), &HandlerFloatVarDecl);

		//foo(x*y)
		Matcher.addMatcher(
			callExpr(unless(hasParent(binaryOperator(hasType(realFloatingPointType())))), 
						unless(hasAncestor(binaryOperator(hasOperatorName("=")))), 
							forEachDescendant(binaryOperator(hasType(realFloatingPointType())).bind("op"))).
								bind("callexprparent"), &HandlerFloatVarDecl);

		//return sum*3.4;
		Matcher.addMatcher(
			returnStmt(forEachDescendant(binaryOperator(hasType(realFloatingPointType())).bind("op"))).
					bind("returnparent"), &HandlerFloatVarDecl);

		Matcher.addMatcher(
			varDecl(hasType(realFloatingPointType()), unless( hasInitializer(floatLiteral())), 
				unless( hasInitializer(ignoringParenImpCasts(integerLiteral()))), 
						hasDescendant(binaryOperator().bind("op"))).
							bind("vardeclparent"), &HandlerFloatVarDecl);

		const auto Op1 =  anyOf(ignoringParenImpCasts(declRefExpr(
					to(varDecl(hasType(realFloatingPointType()))))), 
            implicitCastExpr(unless(hasImplicitDestinationType(realFloatingPointType()))),
             	implicitCastExpr(hasImplicitDestinationType(realFloatingPointType())),
            		ignoringParenImpCasts(ignoringParens(floatLiteral())));
			
		
		//all binary operators, except '=' and binary operators which have operand as binary operator
		const auto Basic = binaryOperator(unless(hasOperatorName("=")), hasType(realFloatingPointType()), 
							hasLHS(Op1),
							hasRHS(Op1)).bind("fadd_be");
		//mass = z = sum = 2.3
		Matcher.addMatcher(
      binaryOperator(hasOperatorName("="), hasType(realFloatingPointType()), 
				hasRHS(floatLiteral().bind("binfloat"))).
          bind("boassign"), &HandlerFloatVarDecl);
	
		const auto BinOp1 = binaryOperator(hasType(realFloatingPointType()),
                                        hasEitherOperand(anyOf(ignoringParens(Basic), Op1)), 
																						unless(hasOperatorName("=")),
																							anyOf(hasAncestor(varDecl().bind("vardeclbo")), 
																									hasAncestor(callExpr(unless(hasAncestor(binaryOperator(hasOperatorName("="))))).bind("callbo")),
																										hasAncestor(returnStmt().bind("returnbo")),
																											hasAncestor(ifStmt().bind("ifstmtbo")),
																												hasAncestor(binaryOperator(hasOperatorName("=")).bind("bobo")),
																													hasAncestor(binaryOperator(hasOperatorName("/=")).bind("bobo")),
																														hasAncestor(binaryOperator(hasOperatorName("*=")).bind("bobo")),
																															hasAncestor(binaryOperator(hasOperatorName("-=")).bind("bobo")),
																																hasAncestor(binaryOperator(hasOperatorName("+=")).bind("bobo")))).bind("fadd_be");
	
		Matcher1.addMatcher(BinOp1, &HandlerFloatVarDecl);
	
		//y = x++;
		//y = x++ + z;
		//y = ++x;
		const auto BinOp2 = binaryOperator(hasType(realFloatingPointType()), hasOperatorName("="),
                                        hasEitherOperand(ignoringParenImpCasts(unaryOperator())
																										)).bind("fadd_be");
	
		Matcher1.addMatcher(BinOp2, &HandlerFloatVarDecl);
		//x /= y;
		const auto BinOp3 = binaryOperator(hasType(realFloatingPointType()), anyOf(hasOperatorName("/="), 
																hasOperatorName("+="), hasOperatorName("-="), hasOperatorName("*=")),
                                        hasEitherOperand(anyOf(ignoringParens(Basic), Op1))).bind("fadd_be");
	
		Matcher1.addMatcher(BinOp3, &HandlerFloatVarDecl);

	
//		Matcher1.addMatcher(
//			ifStmt(hasCondition(binaryOperator(hasEitherOperand(hasType(realFloatingPointType()))).bind("cond"))).
//				bind("ifstmt"), &HandlerFloatVarDecl);
		//printf("%e", x); => t1 = convertP32toDouble; printf("%e", t1);
		Matcher2.addMatcher(
			callExpr(callee(functionDecl(hasName("printf")))).bind("printfsqrt")
					, &HandlerFloatVarDecl);
//		Matcher2.addMatcher(cStyleCastExpr().bind("cast")
	//				, &HandlerFloatVarDecl);
		//double t = 0.5 + x + z * y ;   
/*
		Matcher4.addMatcher(
			varDecl(hasType(realFloatingPointType()), unless( hasInitializer(floatLiteral())), 
				unless( hasInitializer(ignoringParenImpCasts(integerLiteral()))), hasDescendant(binaryOperator().bind("op"))).
					bind("vardeclbo"), &HandlerFloatVarDecl);

		Matcher4.addMatcher(
			callExpr(hasDescendant(binaryOperator(hasType(realFloatingPointType())).bind("op"))).
					bind("callbo"), &HandlerFloatVarDecl);
*/
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
    Matcher3.matchAST(Context);
		HandlerFloatVarDecl.handleBinOp(Context);
    Matcher4.matchAST(Context);
    Matcher2.matchAST(Context);
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
		SourceManager &SM = TheRewriter.getSourceMgr();
    llvm::errs() << "** EndSourceFileAction for: "
                 << SM.getFileEntryForID(SM.getMainFileID())->getName() << "\n";

    // Now emit the rewritten buffer.
    std::string outName (SM.getFileEntryForID(SM.getMainFileID())->getName());

    size_t ext = outName.rfind(".");
    if (ext == std::string::npos)
      ext = outName.length();
    outName.insert(ext, "_pos");

    llvm::errs() << "Output to: " << outName << "\n";
    llvm::errs() << "YourOwnOption: " << YourOwnOption << "\n";
    std::error_code OutErrorInfo;
    std::error_code ok;
    llvm::raw_fd_ostream outFile(llvm::StringRef(outName), OutErrorInfo, llvm::sys::fs::F_None);

    TheRewriter.getEditBuffer(SM.getMainFileID()).write(outFile);
    TheRewriter.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
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

  llvm::outs() << YourOwnOption.getValue();

  return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
