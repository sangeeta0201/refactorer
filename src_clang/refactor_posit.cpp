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

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

#define DoubleSize 6
std::string PositTY = "posit32_t ";
std::stringstream SSBefore;

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

class FloatVarDeclHandler : public MatchFinder::MatchCallback {
public:
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
		
		char *result = (char *)malloc(Offset+1);
		
		strncpy(result, OrigBuf+StartOffset, Offset);
		result[Offset] = '\0'; 
		return result;
	}

	void remove(const VarDecl *VD){
		bool flag = true;
  	int Offset = 0;
		SourceLocation StartLoc =  VD->getLocStart();
		SourceManager &SM = Rewrite.getSourceMgr();
		const char *StartBuf = SM.getCharacterData(StartLoc);
		
		//is this the last vardecl in stmt? if yes, then remove the statement
  	while (*StartBuf != ';') {
    	StartBuf++;
    	if (*StartBuf == '\0' ){
				flag = false;
      	break;
			}
			Offset++;
		}
		if(flag){
			Rewriter::RewriteOptions Opts;
			Opts.RemoveLineIfEmpty = true;
			Rewrite.RemoveText(SourceRange(VD->getLocStart(), VD->getLocStart().getLocWithOffset(Offset)), Opts); 
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
		std::string convert = " = convertDoubleToP32(" + SSBefore.str()+");";
		return convert ;
	}

	void ReplaceVDWithPosit(const VarDecl *VD, std::string positLiteral){
		SourceLocation StartLoc =  VD->getLocStart();
		SourceManager &SM = Rewrite.getSourceMgr();
		const char *StartBuf = SM.getCharacterData(StartLoc);
		int Offset = getOffsetUntil(StartBuf, ';');
		SourceLocation StartLoc1 = VD->getSourceRange().getBegin();
		std::string IndentStr = getStmtIndentString(StartLoc1);
		Rewrite.InsertTextAfterToken(VD->getLocStart().getLocWithOffset(Offset), 
					"\n"+IndentStr+PositTY+VD->getNameAsString()+ positLiteral);
		
		remove(VD);
	}

  FloatVarDeclHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
		if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vd_literal")){
			llvm::errs()<<"literal\n";
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
			llvm::errs()<<"no literal\n";
			VD->dump();
			const ParmVarDecl *PD = dyn_cast<ParmVarDecl>(VD);
  		if (PD) {
				llvm::errs()<<"paramval\n";
  		}

			ReplaceVDWithPosit(VD, ";");
		}
		if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclarray")){
			llvm::errs()<<"array\n";
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
			llvm::errs()<<"pointer\n";
			VD->dump();
			const Type *Ty = VD->getType().getTypePtr();
			QualType QT = Ty->getPointeeType();
			while (!QT.isNull()) {
				Ty = QT.getTypePtr();
				QT = Ty->getPointeeType();
			}
			if(!Ty->isFloatingType())
				return;
			ReplaceVDWithPosit(VD, ";");
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

		Matcher.addMatcher(
			varDecl(hasType(realFloatingPointType()), unless( hasInitializer(floatLiteral())), 
				unless( hasInitializer(ignoringParenImpCasts(integerLiteral())))).
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
/*
		//function parameters
		Matcher.addMatcher(
			cxxMethodDecl(hasAnyParameter(hasType(realFloatingPointType()))).
				bind("vardeclnoinit"), &HandlerFloatVarDecl);
		//matches floating type array with no initializer
		Matcher.addMatcher(
			varDecl(hasType(arrayType()), unless(	hasInitializer(initListExpr()	))).
					bind("vardeclarray"), &HandlerFloatVarDecl);
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
