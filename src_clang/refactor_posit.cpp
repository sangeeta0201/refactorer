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
SmallVector<unsigned, 8> ProcessedVD;

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
		SourceLocation StartLoc =  VD->getLocStart();
		SourceManager &SM = Rewrite.getSourceMgr();
		const char *OrigBuf = SM.getCharacterData(StartLoc);
		const char *StartBuf = SM.getCharacterData(StartLoc);
		
    while (*StartBuf != '[' ){
    	StartBuf++;
			StartOffset++;
		}
  	while (*StartBuf != ';') {
    	StartBuf++;
			Offset++;
		}
		Offset++;
		
		char *result = (char *)malloc(Offset+1);
		
		strncpy(result, OrigBuf+StartOffset, Offset);
		result[Offset] = '\0'; 
		return result;
	}

	void removeLine(SourceLocation StartLoc){
    int Offset = 0;
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
    	const char *StartBuf1 = SM.getCharacterData(StartLoc.getLocWithOffset(-1));
			llvm::errs()<<"removeLine StartBuf:"<<StartBuf1<<"\n";
			Rewrite.RemoveText(SourceRange(StartLoc.getLocWithOffset(-1), StartLoc.getLocWithOffset(Offset)), Opts); 
			ProcessedVD.push_back(lineNo);
		}
	}

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

	std::string convertFloatToPosit(const FloatingLiteral *FL){
		llvm::APFloat floatval = FL->getValue();
		llvm::SmallVector<char, 32> string;
		floatval.toString(string, 32, 0);
		std::stringstream SSBefore;
		for(int i = 0;i<string.size();i++)
			SSBefore <<string[i];
		std::string convert = " convertDoubleToP32(" + SSBefore.str()+");";
		return convert ;
	}

	std::string getTempDest(){
		tmpCount++;
		return "tmp"+std::to_string(tmpCount);
	}

	std::string getPositFuncName(unsigned Opcode){
		string funcName;
		llvm::errs()<<"getPositFuncName Opcode:"<<Opcode<<"\n";
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

	void ReplaceParmVDWithPosit(const VarDecl *VD, char positLiteral){
		SourceLocation StartLoc =  VD->getLocStart();
		SourceManager &SM = Rewrite.getSourceMgr();
		const char *StartBuf = SM.getCharacterData(StartLoc);
		int Offset = getOffsetUntil(StartBuf, positLiteral);
		Rewrite.ReplaceText(SourceRange(VD->getLocStart(), VD->getLocStart().getLocWithOffset(Offset-1)), 
													PositTY);	
	}

	void ReplaceVDWithPosit(const VarDecl *VD, std::string positLiteral){
		llvm::errs()<<"ReplaceVDWithPosit:\n";
		SourceLocation StartLoc =  VD->getLocStart();
		SourceManager &SM = Rewrite.getSourceMgr();
		const char *StartBuf = SM.getCharacterData(StartLoc);
		llvm::errs()<<"StartBuf:"<<StartBuf<<"\n";
		int Offset = getOffsetUntil(StartBuf, ';');
		SourceLocation StartLoc1 = VD->getSourceRange().getBegin();
		std::string IndentStr = getStmtIndentString(StartLoc1);
		Rewrite.InsertTextAfterToken(VD->getLocStart().getLocWithOffset(Offset), 
					"\n"+IndentStr+PositTY+VD->getNameAsString()+ positLiteral);
		
		removeLine(VD->getLocStart());
	}

	void ReplaceBOLiteralWithPosit(const BinaryOperator *BO, std::string lhs, std::string rhs){
		SourceLocation StartLoc =  BO->getLocStart();
		SourceManager &SM = Rewrite.getSourceMgr();
		const char *StartBuf = SM.getCharacterData(StartLoc);
		llvm::errs()<<"StartBuf:"<<StartBuf<<"\n";
		int Offset = getOffsetUntil(StartBuf, ';');
		SourceLocation StartLoc1 = BO->getSourceRange().getBegin();
		std::string IndentStr = getStmtIndentString(StartLoc1);
		llvm::errs()<<"Offset:"<<Offset<<"\n";
		Rewrite.InsertTextAfterToken(BO->getLocStart().getLocWithOffset(Offset), 
					"\n"+IndentStr+PositTY+lhs+"=" +rhs);		
	}

	void ReplaceBOEqWithPosit(const BinaryOperator *BO, std::string Op1, std::string Op2){
			llvm::errs()<<"ReplaceBOEqWithPosit\n";
			BO->dump();	
			std::string func = getPositFuncName(BO->getOpcode());
			SourceLocation StartLoc =  BO->getLocStart();
			SourceManager &SM = Rewrite.getSourceMgr();
			const char *StartBuf = SM.getCharacterData(StartLoc);
			int Offset = getOffsetUntil(StartBuf, ';');
			SourceLocation StartLoc1 = BO->getSourceRange().getBegin();
			std::string IndentStr = getStmtIndentString(StartLoc1);
	
			Rewrite.InsertTextAfter(BO->getLocStart().getLocWithOffset(Offset), 
					"\n"+IndentStr+PositTY+Op1+" = "+Op2+";\n");
			BinOp_Temp.insert(std::pair<const BinaryOperator*, std::string>(BO, Op1));	
			removeLine(BO->getLocStart());
	}

	void ReplaceBOWithPosit(const BinaryOperator *BO, std::string Op1, std::string Op2){
		
			std::string func = getPositFuncName(BO->getOpcode());
			SourceLocation StartLoc =  BO->getLocStart();
			SourceManager &SM = Rewrite.getSourceMgr();
			const char *StartBuf = SM.getCharacterData(StartLoc);
			llvm::errs()<<"StartBuf:"<<StartBuf<<"\n";
			int Offset = getOffsetUntil(StartBuf, ';');
			SourceLocation StartLoc1 = BO->getSourceRange().getBegin();
			std::string IndentStr = getStmtIndentString(StartLoc1);
			const char *StartBuf1 = SM.getCharacterData(BO->getLocStart().getLocWithOffset(Offset));
			llvm::errs()<<"StartBuf1:"<<StartBuf1<<"\n";
			std::string temp, newline;
			temp = getTempDest();
			newline = "\0";
			Rewrite.InsertTextAfterToken(BO->getLocStart().getLocWithOffset(Offset), 
					"\n"+IndentStr+PositTY+" "+temp+" = "+func+"("+Op1+","+Op2+");"+newline);
			BinOp_Temp.insert(std::pair<const BinaryOperator*, std::string>(BO, temp));	
	}

  FloatVarDeclHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
		if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vd_literal")){
			VD->dump();
			const FloatingLiteral *FL = Result.Nodes.getNodeAs<clang::FloatingLiteral>("floatliteral");
			if(FL != NULL){
				std::string positLiteral = convertFloatToPosit(FL);
				ReplaceVDWithPosit(VD, positLiteral);
			}
			const IntegerLiteral *IL = Result.Nodes.getNodeAs<clang::IntegerLiteral>("intliteral");
			if(IL != NULL){
				std::string positLiteral = convertIntToPosit(IL);
				ReplaceVDWithPosit(VD, positLiteral);
			}
		}
		if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclnoinit")){
			const ParmVarDecl *PD = dyn_cast<ParmVarDecl>(VD);
  		if (PD) {
				ReplaceParmVDWithPosit(VD, ' ');
  		}
			else{
				ReplaceVDWithPosit(VD, ";");
			}
		}
		if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclarray")){
			VD->dump();
			const Type *Ty = VD->getType().getTypePtr();
			while (Ty->isArrayType()) {                                                             
    		const ArrayType *AT = dyn_cast<ArrayType>(Ty);
    		Ty = AT->getElementType().getTypePtr();
  		}
			Ty->dump();
			if(!Ty->isFloatingType())
				return;

			std::string arrayDim = getArrayDim(VD);
			ReplaceVDWithPosit(VD, arrayDim);
		}
		if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclpointer")){
			VD->dump();
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
				ReplaceVDWithPosit(VD, ";");
			}
		}
		if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclbo")){
			const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("op");
			std::string Op1 = BinOp_Temp.at(BO);
			ReplaceVDWithPosit(VD, " = "+Op1+";");
		}
		if (const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("fadd_ee")){

			//check if this binaryoperator is processed
			//check if LHS is processed, get its temp variable
			//check if RHS is processed, get its temp variable
			//create new varible, if this BO doesnt have a VD as its LHS
			//push this in map
			const DeclRefExpr *DEL = Result.Nodes.getNodeAs<clang::DeclRefExpr>("lhs_ee");
			const DeclRefExpr *DER = Result.Nodes.getNodeAs<clang::DeclRefExpr>("rhs_ee");
			const FloatingLiteral *FL_lhs = Result.Nodes.getNodeAs<clang::FloatingLiteral>("lhs_literal");
			const FloatingLiteral *FL_rhs = Result.Nodes.getNodeAs<clang::FloatingLiteral>("rhs_literal");
			const BinaryOperator *BO_lhs = Result.Nodes.getNodeAs<clang::BinaryOperator>("lhs_bo");
			const BinaryOperator *BO_rhs = Result.Nodes.getNodeAs<clang::BinaryOperator>("rhs_bo");

			BO->dump();
			std::string Op1, Op2, lhs, rhs;
			llvm::errs()<<"BO_lhs:\n";
			BO_lhs->dump();
			llvm::errs()<<"BO_rhs:\n";
			BO_rhs->dump();
			if(FL_lhs){
				lhs = getTempDest();
				rhs  = convertFloatToPosit(FL_lhs);
				ReplaceBOLiteralWithPosit(BO, lhs, rhs);
				Op1 = lhs;
				llvm::errs()<<"FL_lhs Op1:"<<Op1<<"\n";
			}
			else if(BO_lhs){
				Op1 = BinOp_Temp.at(BO_lhs);
				llvm::errs()<<"BO_lhs Op1:"<<Op1<<"\n";
			}
			else if(DEL){
				Op1 = DEL->getDecl()->getName();
				llvm::errs()<<"DEL Op1:"<<Op1<<"\n";
			}
			else
				assert("fadd_ee: operand not handled!!!");

			if(FL_rhs){
				lhs = getTempDest();
				rhs  = convertFloatToPosit(FL_rhs);
				ReplaceBOLiteralWithPosit(BO, lhs, rhs);
				Op2 = lhs;
				llvm::errs()<<"FL_lhs Op2:"<<Op2<<"\n";
			}
			else if(BO_rhs){
				Op2 = BinOp_Temp.at(BO_rhs);
				llvm::errs()<<"BO_lhs Op2:"<<Op2<<"\n";
			}
			else if(DER){
				Op2 = DER->getDecl()->getName();
				llvm::errs()<<"DER Op2:"<<Op2<<"\n";
			}
			else{
				llvm::errs()<<"error\n";
				assert("fadd_ee: operand not handled!!!");
			}

			llvm::errs()<<"BO->getOpcode():"<<BO->getOpcode()<<"\n";
			if (BO->getOpcode() == BO_Assign){
				llvm::errs()<<"Op1:"<<Op1<<"\n";
				llvm::errs()<<"Op2:"<<Op2<<"\n";
				ReplaceBOEqWithPosit(BO, Op1, Op2);
			}
			else{
				ReplaceBOWithPosit(BO, Op1, Op2);
			}
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
			varDecl(hasType(realFloatingPointType()), unless( hasInitializer(floatLiteral())), 
				unless( hasInitializer(ignoringParenImpCasts(integerLiteral()))), unless(hasDescendant(binaryOperator()))).
					bind("vardeclnoinit"), &HandlerFloatVarDecl);


		//pointer
		Matcher.addMatcher(
			varDecl(hasType(pointerType())). 
					bind("vardeclpointer"), &HandlerFloatVarDecl);

		Matcher.addMatcher(
			typedefDecl().
					bind("vardeclstruct"), &HandlerFloatVarDecl);
		Matcher.addMatcher(
			varDecl(hasType(arrayType()), unless(	hasInitializer(initListExpr()	))).
					bind("vardeclarray"), &HandlerFloatVarDecl);
		//function parameters
		Matcher.addMatcher(
			cxxMethodDecl(hasAnyParameter(hasType(realFloatingPointType()))).
				bind("vardeclnoinit"), &HandlerFloatVarDecl);

		//all binary operators, except '=' and binary operators which have operand as binary operator
		Matcher.addMatcher(
			binaryOperator(unless(hasOperatorName("=")),
  				hasLHS(anyOf(ignoringParenImpCasts(declRefExpr(
                    to(varDecl(hasType(realFloatingPointType())))).bind("lhs_ee")), 
						ignoringParenImpCasts(floatLiteral().bind("lhs_literal")))),
  						hasRHS(anyOf(ignoringParenImpCasts(declRefExpr(
                    to(varDecl(hasType(realFloatingPointType())))).bind("rhs_ee")), 
											ignoringParenImpCasts(floatLiteral().bind("rhs_literal"))))).bind("fadd_ee"), &HandlerFloatVarDecl);

		Matcher1.addMatcher(
			binaryOperator(unless(hasOperatorName("=")),
  				hasLHS(binaryOperator(                                                                                   
            hasType(realFloatingPointType())).bind("lhs_bo")),
  						hasRHS(binaryOperator(hasType(realFloatingPointType())).bind("rhs_bo"))).bind("fadd_ee"), &HandlerFloatVarDecl);

		//all binary operators
		//should be executed after above matcher
		Matcher2.addMatcher(
			binaryOperator(hasOperatorName("="),
  				hasLHS(anyOf(binaryOperator(
						hasType(realFloatingPointType())).bind("lhs_bo"),ignoringParenImpCasts(declRefExpr(
                    to(varDecl(hasType(realFloatingPointType())))).bind("lhs_ee")), 
						ignoringParenImpCasts(floatLiteral().bind("lhs_literal")))),
  						hasRHS(anyOf(binaryOperator(hasType(realFloatingPointType())).bind("rhs_bo"), 
									ignoringParenImpCasts(declRefExpr(
                    to(varDecl(hasType(realFloatingPointType())))).bind("rhs_ee")), 
											ignoringParenImpCasts(floatLiteral().bind("rhs_literal"))))).bind("fadd_ee"), &HandlerFloatVarDecl);

		//double t = 0.5 + x + z * y ;   
		Matcher3.addMatcher(
			varDecl(hasType(realFloatingPointType()), unless( hasInitializer(floatLiteral())), 
				unless( hasInitializer(ignoringParenImpCasts(integerLiteral()))), hasDescendant(binaryOperator().bind("op"))).
					bind("vardeclbo"), &HandlerFloatVarDecl);
		//binary operator which has lhs as binaryoperator and rhs as expr



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
    return llvm::make_unique<MyASTConsumer>(TheRewriter);
  }

private:
  Rewriter TheRewriter;
};

int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, MatcherSampleCategory);
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());

  return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
