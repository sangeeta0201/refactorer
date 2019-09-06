#include <sstream>
#include <string>
#include <iostream>
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"

#include "clang/Driver/Options.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/ADT/SmallVector.h"

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
clang::SourceManager *SrcManager;

std::map<BinaryOperator*, bool> BinOp;
stack <std::string> TempStack; 
stack <std::string> TempStack1; 
stack <BinaryOperator*> BinOpStack; 
stack <Stmt*> StmtStack; 
stack <std::string> VarName;
bool flag = false;
size_t countBinOP = 0;
size_t tmpCount = 0;
static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");


size_t oldStmtSize, newStmtSize ;
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
public:
  MyASTVisitor(Rewriter &R) : TheRewriter(R) {}

		std::string name;
		std::string BOR;
		std::string SubLOR;

		void traverseVarNameAST(ImplicitCastExpr *Cast){
		Expr *RHS = Cast->getSubExpr();
		VarDecl *LVar;
		std::string tmp;
		llvm::APInt val;
		if (ArraySubscriptExpr *ASE = llvm::dyn_cast<ArraySubscriptExpr>(RHS)) {
				if(IntegerLiteral *IL = llvm::dyn_cast<IntegerLiteral>(ASE->getRHS())){
					val = IL->getValue();

					llvm::SmallVector<char, 32> Str;
					val.toString(Str,10, true);
    			std::stringstream SSBefore;
					for(int i = 0;i<Str.size();i++)
    				SSBefore <<Str[i];
					tmp = "["+SSBefore.str()+"]";
					name = tmp + name;
				}
			if (ImplicitCastExpr *CastL = llvm::dyn_cast<ImplicitCastExpr>(ASE->getLHS())){ 
				traverseVarNameAST(CastL);
			}
			if (ImplicitCastExpr *CastR = llvm::dyn_cast<ImplicitCastExpr>(ASE->getRHS())){
				traverseVarNameAST(CastR);
			}
		}
		else if (DeclRefExpr *DeclRef = llvm::dyn_cast<DeclRefExpr>(RHS)) {
			if (LVar = llvm::dyn_cast<VarDecl>(DeclRef->getDecl())) {
				BOR = LVar->getName();
				BOR = BOR+name;
				name.clear();
			}
		}
		else {
				return ;
		}
	}

unsigned getLocationOffsetAndFileID(SourceLocation Loc,                               
                                                  FileID &FID,
                                                  SourceManager *SrcManager)
{
  assert(Loc.isValid() && "Invalid location");
  std::pair<FileID,unsigned> V = SrcManager->getDecomposedLoc(Loc);
  FID = V.first;
  return V.second;
}

std::string getStmtIndentString(SourceLocation StmtStartLoc)
{
	SourceManager *SrcManager = &TheRewriter.getSourceMgr();
  if (StmtStartLoc.isMacroID()) {
    StmtStartLoc = SrcManager->getFileLoc(StmtStartLoc);
  }
  
  FileID FID;
  unsigned StartOffset = 
    getLocationOffsetAndFileID(StmtStartLoc, FID, SrcManager);
  
  StringRef MB = SrcManager->getBufferData(FID);
  
  unsigned lineNo = SrcManager->getLineNumber(FID, StartOffset) - 1;
  const SrcMgr::ContentCache *
      Content = SrcManager->getSLocEntry(FID).getFile().getContentCache();
  unsigned lineOffs = Content->SourceLineCache[lineNo];
  
  // Find the whitespace at the start of the line.
  StringRef indentSpace;
  
  unsigned I = lineOffs;
  while (isspace(MB[I]))
    ++I;
  indentSpace = MB.substr(lineOffs, I-lineOffs);
  
  return indentSpace;
}

SourceLocation getVarDeclTypeLocBegin(const VarDecl *VD)
{
  TypeLoc VarTypeLoc = VD->getTypeSourceInfo()->getTypeLoc();

  TypeLoc NextTL = VarTypeLoc.getNextTypeLoc();
  while (!NextTL.isNull()) {
    VarTypeLoc = NextTL;
    NextTL = NextTL.getNextTypeLoc();
  }

  return VarTypeLoc.getBeginLoc();
}


	void traverseBinOPAST(Stmt *s){
		if(BinaryOperator *op = dyn_cast<BinaryOperator>(s)){
			BinOpStack.push(op);	
			llvm::errs()<<"BinaryOperator pushed;"<<op<<"\n";
			BinOp.insert(std::pair<BinaryOperator*, bool>(op, false)); 
			traverseBinOPAST(op->getLHS());
			traverseBinOPAST(op->getRHS());
		}
		else if(ParenExpr *PE = dyn_cast<ParenExpr>(s)){
			if(BinaryOperator *op = dyn_cast<BinaryOperator>(PE->getSubExpr())){
				BinOpStack.push(op);	
				llvm::errs()<<"BinaryOperator pushed;"<<op<<"\n";
				BinOp.insert(std::pair<BinaryOperator*, bool>(op, false)); 
				traverseBinOPAST(op->getLHS());
				traverseBinOPAST(op->getRHS());
			}
		}
		else if(ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(s)){
			if(BinaryOperator *op = dyn_cast<BinaryOperator>(ICE->getSubExpr())){
				QualType QT = op->getType();
    		std::string TypeStr = QT.getAsString();
				if(TypeStr == "int"){
					flag = false;
					llvm::errs()<<"bo type:"<<TypeStr<<"\n";
					BinOpStack.push(op);	
					llvm::errs()<<"BinaryOperator pushed;"<<op<<"\n";
					BinOp.insert(std::pair<BinaryOperator*, bool>(op, false)); 
					return;
				}
				BinOpStack.push(op);	
				llvm::errs()<<"BinaryOperator pushed;"<<op<<"\n";
				BinOp.insert(std::pair<BinaryOperator*, bool>(op, false)); 
				traverseBinOPAST(op->getLHS());
				traverseBinOPAST(op->getRHS());
			}
		}
		else 
			return;
	}

std::string getOperandName(BinaryOperator *BO, Expr *LHS, SourceLocation ST, std::string IndentStr){
	llvm::errs()<<"getOperandName:\n";
//	LHS->dump();
	BOR.clear();
	std::string boRhs;
	llvm::raw_string_ostream stream(boRhs);
	LHS->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
	stream.flush();
	bool flag = false;
	bool processed = false;
	if(BinOp.count(dyn_cast<BinaryOperator>(LHS)) != 0){ 
    processed = BinOp.at(dyn_cast<BinaryOperator>(LHS));
  }
	else if(ParenExpr *PE = dyn_cast<ParenExpr>(LHS)){
		if(BinaryOperator *op = dyn_cast<BinaryOperator>(PE->getSubExpr())){
			if(BinOp.count(op) != 0){ 
    		processed = BinOp.at(op);
			}
		}
	}
	else if(ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(LHS)){
		if(BinaryOperator *op = dyn_cast<BinaryOperator>(ICE->getSubExpr())){
			if(BinOp.count(op) != 0){ 
    		processed = BinOp.at(op);
			}
		}
	}
	else
		llvm::errs()<<"getOperandName not found Binop\n";
	if(processed)
		return BOR;
	QualType QT = BO->getType();
  std::string TypeStr = QT.getAsString();
	if(TypeStr == "int"){
		return BOR;
	}
	if(ParenExpr *PE = dyn_cast<ParenExpr>(LHS)){
		if (FloatingLiteral *FL = llvm::dyn_cast<FloatingLiteral>(PE->getSubExpr())) { 
			llvm::APFloat floatval = FL->getValue();
			llvm::SmallVector<char, 32> string;
			floatval.toString(string, 32, 0);
    	std::stringstream SSBefore;
			for(int i = 0;i<string.size();i++)
    		SSBefore <<string[i];
			std::string temp = "tmp"+std::to_string(tmpCount);
			tmpCount++;
			emitCodeFL(IndentStr + "posit32_t "+temp+"=convertDoubleToP32("+SSBefore.str()+");\n", temp, ST);
			flag = true;
		}
	}
	if(UnaryOperator *UO = llvm::dyn_cast<UnaryOperator>(LHS)){
		if (FloatingLiteral *FL = llvm::dyn_cast<FloatingLiteral>(UO->getSubExpr())) { 
			llvm::APFloat floatval = FL->getValue();
			llvm::SmallVector<char, 32> string;
			floatval.toString(string, 32, 0);
    	std::stringstream SSBefore;
			for(int i = 0;i<string.size();i++)
    		SSBefore <<string[i];
			std::string temp = "tmp"+std::to_string(tmpCount);
			tmpCount++;
			emitCodeFL(IndentStr + "posit32_t "+temp+"=convertDoubleToP32("+UO->getOpcodeStr(UO->getOpcode()).str()+SSBefore.str()+");\n", temp, ST);
			flag = true;
		}
		else{
			llvm::errs()<<"********need to handle unary operator:\n";
			std::string opName;
			llvm::raw_string_ostream stream(opName);
			UO->getSubExpr()->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
			stream.flush();
			llvm::errs()<<"opName:"<<opName<<"\n";
			TempStack.push(opName);
			std::string temp = "tmp"+std::to_string(tmpCount);
			tmpCount++;
			emitCodeFL(IndentStr + "posit32_t "+temp+"=convertDoubleToP32(0);\n", temp, ST);
			size_t op = UO->getOpcode();
			buildAST(BOR, op);
			if(op == 6 ) //25 is for +=
    		TempStack.push("p32_add");
			if(op == 7 ) //25 is for +=
    		TempStack.push("p32_sub");
			TempStack.push("=");
			std::string tmp = "tmp"+std::to_string(tmpCount);
			tmpCount++;
			TempStack.push(tmp);
			emitCode(tmp, ST, IndentStr);
			return BOR;
		}
	}
	if (FloatingLiteral *FL = llvm::dyn_cast<FloatingLiteral>(LHS)) { 
		llvm::APFloat floatval = FL->getValue();
		llvm::SmallVector<char, 32> string;
		floatval.toString(string, 32, 0);
    std::stringstream SSBefore;
		for(int i = 0;i<string.size();i++)
    	SSBefore <<string[i];
		std::string temp = "tmp"+std::to_string(tmpCount);
		tmpCount++;
		emitCodeFL(IndentStr + "posit32_t "+temp+"=convertDoubleToP32("+SSBefore.str()+");\n", temp, ST);
		flag = true;
	}
	if(ParenExpr *PE = llvm::dyn_cast<ParenExpr>(LHS)){
		if(CStyleCastExpr *CCE = llvm::dyn_cast<CStyleCastExpr>(PE->getSubExpr())){
			llvm::errs()<<"CStyleCastExpr****\n";
			std::string subExpr;
			CCE->getSubExpr();
			llvm::raw_string_ostream stream(subExpr);
			CCE->getSubExpr()->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
			stream.flush();
			std::cout<<"subExpr:"<<subExpr<<"\n";
			std::string temp = "tmp"+std::to_string(tmpCount);
			tmpCount++;
			emitCodeFL(IndentStr + "posit32_t "+temp+"=convertDoubleToP32("+subExpr+");\n", temp, ST);
			flag = true;
		}
	}
		if(CStyleCastExpr *CCE = llvm::dyn_cast<CStyleCastExpr>(LHS)){
			llvm::errs()<<"CStyleCastExpr****\n";
			std::string subExpr;
			CCE->getSubExpr();
			llvm::raw_string_ostream stream(subExpr);
			CCE->getSubExpr()->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
			stream.flush();
			std::cout<<"subExpr:"<<subExpr<<"\n";
			std::string temp = "tmp"+std::to_string(tmpCount);
			tmpCount++;
			emitCodeFL(IndentStr + "posit32_t "+temp+"=convertDoubleToP32("+subExpr+");\n", temp, ST);
			flag = true;
		}
	else if(ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(LHS)){
		if(UnaryOperator *UO = llvm::dyn_cast<UnaryOperator>(ICE->getSubExpr())){
			if (IntegerLiteral *IL = llvm::dyn_cast<IntegerLiteral>(UO->getSubExpr())) { 
				std::string valStr;
				llvm::APInt val = IL->getValue();
				llvm::raw_string_ostream stream(valStr);
				IL->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
				stream.flush();
				std::string temp = "tmp"+std::to_string(tmpCount);
				tmpCount++;
				emitCodeFL(IndentStr + "posit32_t "+temp+"=convertDoubleToP32("+valStr+");\n", temp, ST);
				flag = true;
			}
		}
			if(BinaryOperator *BO = llvm::dyn_cast<BinaryOperator>(ICE->getSubExpr())){
				QualType QT = BO->getType();
  			std::string TypeStr = QT.getAsString();
				if(TypeStr == "int"){
					std::string opName;
					llvm::raw_string_ostream stream(opName);
					BO->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
					stream.flush();
					std::string temp = "tmp"+std::to_string(tmpCount);
					tmpCount++;
					emitCodeFL(IndentStr + "posit32_t "+temp+"=convertDoubleToP32("+opName+");\n", temp, ST);
					flag = true;
				}
			}
		if(ParenExpr *PE = llvm::dyn_cast<ParenExpr>(ICE->getSubExpr())){
			if(BinaryOperator *BO = llvm::dyn_cast<BinaryOperator>(PE->getSubExpr())){
				QualType QT = BO->getType();
  			std::string TypeStr = QT.getAsString();
				if(TypeStr == "int"){
					std::string opName;
					llvm::raw_string_ostream stream(opName);
					BO->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
					stream.flush();
					std::string temp = "tmp"+std::to_string(tmpCount);
					tmpCount++;
					emitCodeFL(IndentStr + "posit32_t "+temp+"=convertDoubleToP32("+opName+");\n", temp, ST);
					flag = true;
				}
			}
		}
		if(BinaryOperator *BO = llvm::dyn_cast<BinaryOperator>(ICE->getSubExpr())){
			std::string opName;
			llvm::raw_string_ostream stream(opName);
			BO->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
			stream.flush();
			std::string temp = "tmp"+std::to_string(tmpCount);
			tmpCount++;
			emitCodeFL(IndentStr + "posit32_t "+temp+"=convertDoubleToP32("+opName+");\n", temp, ST);
			flag = true;
			return BOR;
		}
		if (IntegerLiteral *IL = llvm::dyn_cast<IntegerLiteral>(ICE->getSubExpr())) { 
			std::string valStr;
			llvm::APInt val = IL->getValue();
			llvm::raw_string_ostream stream(valStr);
			IL->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
			stream.flush();
			std::string temp = "tmp"+std::to_string(tmpCount);
			tmpCount++;
			emitCodeFL(IndentStr + "posit32_t "+temp+"=convertDoubleToP32("+valStr+");\n", temp, ST);
			flag = true;
		}
		else if (FloatingLiteral *FL = llvm::dyn_cast<FloatingLiteral>(ICE->getSubExpr())) { 
			llvm::APFloat floatval = FL->getValue();
			llvm::SmallVector<char, 32> string;
			floatval.toString(string, 32, 0);
    	std::stringstream SSBefore;
			for(int i = 0;i<string.size();i++)
    		SSBefore <<string[i];
			std::string temp = "tmp"+std::to_string(tmpCount);
			tmpCount++;
			emitCodeFL(IndentStr + "posit32_t "+temp+"=convertDoubleToP32("+SSBefore.str()+");\n", temp, ST);
			flag = true;
		}
		else if(ImplicitCastExpr *Cast = dyn_cast<ImplicitCastExpr>(ICE->getSubExpr())){
      if (const DeclRefExpr *DeclRef = llvm::dyn_cast<DeclRefExpr>(Cast->getSubExpr())) {
        if (const VarDecl *Var = llvm::dyn_cast<VarDecl>(DeclRef->getDecl())) {
          if (Var->getType()->isIntegerType()) {
						std::string temp = "tmp"+std::to_string(tmpCount);
						tmpCount++;
						emitCodeFL(IndentStr + "posit32_t "+temp+"=convertDoubleToP32("+Var->getName().str()+");\n", temp, ST);
						flag = true;
          }
        }
      }
		}
	}
	if(!flag){
		llvm::raw_string_ostream stream(BOR);
		LHS->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
		stream.flush();
		llvm::errs()<<"pretty print BOR:"<<BOR<<"\n";
	}

	return BOR;
}

void buildAST(std::string LOR, size_t op){
	if(LOR.length() != 0){
		TempStack.push(LOR);
		std::cout<<"pushing TempStack1:"<<LOR<<"\n";
	}
	else if(TempStack1.size() > 0){
		std::string temp = TempStack1.top();
		TempStack1.pop();
		TempStack.push(temp);
		std::cout<<"pushing TempStack2:"<<temp<<"\n";
	}
	else{
		llvm::errs()<<"buildAST Error!!!\n";
	}
}

void emitCodeFL(std::string stmt, std::string temp, SourceLocation ST){
	TheRewriter.InsertText(ST, stmt, true);	
	if(temp.size() > 0)
		TempStack1.push(temp);
}

void emitCode(std::string temp, SourceLocation ST, std::string IndentStr){
	std::string old, cur, stmt;
	size_t count = 0;
	bool flag = false;
	while(TempStack.size()>0){
		count++;
		old = cur;	
		cur = TempStack.top();
		std::cout<<"cur:"<<cur<<"\n";
		TempStack.pop();
		stmt = stmt + cur;
		if(cur.find("p32") == 0){
			stmt = stmt+"(";
			flag = true;
		}
		if(cur == "="){
			if(old.find("tmp") == 0){
				stmt = "posit32_t "+stmt;
			}
		}
		if(flag && count == 4){
			stmt = stmt+",";
		}
		if(flag && count == 5){
			stmt = stmt+")";
			int pos = stmt.find("sqrt");
			if(pos >= 0){
				stmt.insert(pos,"p32_");  
			}
			TheRewriter.InsertText(ST, IndentStr + stmt+";\n", true);	
		std::cout<<"stmt:::"<<stmt<<"\n";
			stmt.clear();
			cur.clear();
		}
		else if(TempStack.size() == 0){
			int pos = stmt.find("sqrt");
			if(pos >= 0){
				stmt.insert(pos,"p32_");  
			}
				TheRewriter.InsertText(ST, IndentStr + stmt+";\n", true);	
			std::cout<<"stmt:::"<<stmt<<"\n";
			stmt.clear();
		}	
	}
	
	if(temp.size() > 0){
		TempStack1.push(temp);
	}
}

void processBinOPCall(Stmt *s, std::string lhs, SourceLocation ST, SourceLocation TempLoc, std::string IndentStr){
	while(BinOpStack.size() > 0){
		llvm::errs()<<"processBinOPVD binop stack size:"<<BinOpStack.size()<<"\n\n\n\n";
		BinaryOperator *bo = BinOpStack.top();
		BinOpStack.pop();
			llvm::errs()<<"BinaryOperator popped;"<<bo<<"\n";
		llvm::errs()<<"processBinOPVD bo:\n";
//		bo->dump();
	
		std::map<BinaryOperator*, bool>::iterator it = BinOp.find(bo); 
  	if (it != BinOp.end()){
    	it->second = true; 
  	} 

		string stmt;
		llvm::raw_string_ostream stream(stmt);

		bo->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
		stream.flush();
		std::cout<<stmt<<"\n";
		std::cout<<"*********\n";

		QualType QT = bo->getType();
    std::string TypeStr = QT.getAsString();
		llvm::errs()<<"bo: opcode:"<<bo->getOpcode()<<"\n";	
		size_t op =	bo->getOpcode();

		if(TypeStr == "double" || TypeStr == "float"){
			const FloatingLiteral *FL;
			std::string nameR, nameL;
			std::string LOR;
			std::string SubLOR;
			std::string BOR;
			const VarDecl *LVar;
			LOR = getOperandName(bo, bo->getLHS(), ST, IndentStr);
			BOR = getOperandName(bo, bo->getRHS(), ST, IndentStr);
	
			llvm::errs()<<"LOR:"<<LOR<<"\n";
			llvm::errs()<<"BOR:"<<BOR<<"\n";
			TempStack.push(LOR);
			TempStack.push(BOR);
			buildAST(BOR, op);
			buildAST(LOR, op);

			if(op == 5 || op == 25) //25 is for +=
				TempStack.push("p32_add");
			else if(op == 2 || op == 22) //22 is for *=
				TempStack.push("p32_mul");
			else if(op == 3 || op == 23)
				TempStack.push("p32_div");
			else if(op == 6 || op == 26)
				TempStack.push("p32_sub");	

			TempStack.push("=");
			std::string tmp = "tmp"+std::to_string(tmpCount);
			tmpCount++;
			TempStack.push(tmp);
			emitCode(tmp, ST, IndentStr);
			if(BinOpStack.size() == 0){
				buildAST("", op);
				buildAST(lhs, op);
				emitCode("", TempLoc, IndentStr);
			}
		}
	}
}
void processBinOPVD(Stmt *s, std::string lhs, SourceLocation ST, std::string IndentStr){
	while(BinOpStack.size() > 0){
		llvm::errs()<<"processBinOPVD binop stack size:"<<BinOpStack.size()<<"\n\n\n\n";
		BinaryOperator *bo = BinOpStack.top();
		BinOpStack.pop();
			llvm::errs()<<"BinaryOperator popped;"<<bo<<"\n";
		llvm::errs()<<"processBinOPVD bo:\n";
//		bo->dump();
	
		std::map<BinaryOperator*, bool>::iterator it = BinOp.find(bo); 
  	if (it != BinOp.end()){
    	it->second = true; 
  	} 

		string stmt;
		llvm::raw_string_ostream stream(stmt);

		bo->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
		stream.flush();
		std::cout<<stmt<<"\n";
		std::cout<<"*********\n";

		QualType QT = bo->getType();
    std::string TypeStr = QT.getAsString();
		llvm::errs()<<"bo: opcode:"<<bo->getOpcode()<<"\n";	
		size_t op =	bo->getOpcode();

		if(TypeStr == "double" || TypeStr == "float"){
			const FloatingLiteral *FL;
			std::string nameR, nameL;
			std::string LOR;
			std::string SubLOR;
			std::string BOR;
			const VarDecl *LVar;
			LOR = getOperandName(bo, bo->getLHS(), ST, IndentStr);
			BOR = getOperandName(bo, bo->getRHS(), ST, IndentStr);
	
			llvm::errs()<<"LOR:"<<LOR<<"\n";
			llvm::errs()<<"BOR:"<<BOR<<"\n";
		std::cout<<"pushing TempStack6:"<<LOR<<"\n";
		std::cout<<"pushing TempStack7:"<<BOR<<"\n";
			//TempStack.push(LOR);
			//TempStack.push(BOR);
			buildAST(BOR, op);
			buildAST(LOR, op);

			if(op == 5 || op == 25) //25 is for +=
				TempStack.push("p32_add");
			else if(op == 2 || op == 22) //22 is for *=
				TempStack.push("p32_mul");
			else if(op == 3 || op == 23)
				TempStack.push("p32_div");
			else if(op == 6 || op == 26)
				TempStack.push("p32_sub");	

			TempStack.push("=");
			std::string tmp = "tmp"+std::to_string(tmpCount);
			tmpCount++;
			TempStack.push(tmp);
		std::cout<<"pushing TempStack8:"<<tmp<<"\n";
			std::cout<<"tmp:"<<tmp<<"\n";
			emitCode(tmp, ST, IndentStr);
			if(BinOpStack.size() == 0){
				buildAST("", op);
				TempStack.push("=");
				buildAST(lhs, op);
				emitCode("", ST, IndentStr);
			}
		}
	}
}

void processBinOP(Stmt *s, SourceLocation ST, std::string IndentStr){
	while(BinOpStack.size() > 0){
		llvm::errs()<<"processBinOP binop stack size:"<<BinOpStack.size()<<"\n\n\n\n";
		BinaryOperator *bo = BinOpStack.top();
		BinOpStack.pop();
		llvm::errs()<<"BinaryOperator popped;"<<bo<<"\n";
		llvm::errs()<<"processBinOP bo:\n";
//		bo->dump();
		std::map<BinaryOperator*, bool>::iterator it = BinOp.find(bo); 
  	if (it != BinOp.end()){
    	it->second = true; 
  	} 

		string stmt;
		llvm::raw_string_ostream stream(stmt);

		bo->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
		stream.flush();
		std::cout<<stmt<<"\n";

		QualType QT = bo->getType();
    std::string TypeStr = QT.getAsString();
		size_t op =	bo->getOpcode();

		std::cout<<"processBinOP op:"<<op<<"*********\n";
		std::string LOR;
		std::string BOR;
		if(TypeStr == "int"){
			LOR = getOperandName(bo, bo->getLHS(), ST, IndentStr);
			llvm::errs()<<"LOR:"<<LOR<<"\n";
			buildAST(LOR, op);
		}
		if(TypeStr == "double" || TypeStr == "float"){
			const FloatingLiteral *FL;
			std::string nameR, nameL;
			const VarDecl *LVar;
			LOR = getOperandName(bo, bo->getLHS(), ST, IndentStr);
			BOR = getOperandName(bo, bo->getRHS(), ST, IndentStr);
	
			llvm::errs()<<"LOR:"<<LOR<<"\n";
			llvm::errs()<<"BOR:"<<BOR<<"\n";
			if(op != 21){ //it means this is intermediate binop
				buildAST(BOR, op);
				buildAST(LOR, op);
				if(op == 5 || op == 25) //25 is for +=
          TempStack.push("p32_add");
        else if(op == 2 || op == 22) //22 is for *=
          TempStack.push("p32_mul");
        else if(op == 3 || op == 23)
          TempStack.push("p32_div");
        else if(op == 6 || op == 26)
          TempStack.push("p32_sub");	
				TempStack.push("=");
				std::string tmp = "tmp"+std::to_string(tmpCount);
				tmpCount++;
				TempStack.push(tmp);
				emitCode(tmp, ST, IndentStr);
				if(op == 25 || op == 22 || op == 23 || op == 26){ //composite operator
					buildAST("", op);
					TempStack.push("=");
					buildAST(LOR, op);
					emitCode("", ST, IndentStr);
				}
			}
			else{
				buildAST(BOR, op);
				TempStack.push("=");
				buildAST(LOR, op);
				emitCode("", ST, IndentStr);
			}
		}
	}
}
/*	
	bool VisitDeclRefExpr(DeclRefExpr *DeclRef){
		const VarDecl *LVar;
		std::string LOR;
		if (LVar = llvm::dyn_cast<VarDecl>(DeclRef->getDecl())) {
			llvm::errs()<<"VisitDeclRefExpr:"<<LVar->getName()<<"\n";  // YAY found it!!
			LOR = LVar->getName();
		}
		return true;
	}
	bool VisitIntegerLiteral(IntegerLiteral *E){
		llvm::APInt val = E->getValue();
		llvm::errs()<<"IL val:"<<val<<"\n";
		return true;
	}
*/
	bool VisitStmt(Stmt *s) {
		if (DeclStmt *d = dyn_cast<DeclStmt>(s)) {
        
		}
		if (CStyleCastExpr *c = dyn_cast<CStyleCastExpr>(s)) {
			QualType QT = c->getTypeAsWritten();
			std::string TypeStr = QT.getAsString();
			//if(TypeStr.find("double") == 0){
			//	TheRewriter.ReplaceText(c->getLocStart(), 7, "(posit32_t ");
			//}
		}
		if(UnaryExprOrTypeTraitExpr *c = dyn_cast<UnaryExprOrTypeTraitExpr>(s)) {
			QualType QT = c->getTypeOfArgument();
			std::string TypeStr = QT.getAsString();
    	SourceManager &SM = TheRewriter.getSourceMgr();
			if(TypeStr.find("double") == 0){
				const char *Buf = SM.getCharacterData(c->getLocStart());
				int Offset = 0;                                                                            
  			while (*Buf != ')') {
   				 Buf++;
    			if (*Buf == '\0')
      			break;
    			Offset++;
  			}

			//	int RangeSize = TheRewriter.getRangeSize(SourceRange(c->getLocStart(), c->getExprLoc()));
				TheRewriter.ReplaceText(c->getLocStart(), Offset, "sizeof(posit32_t");
			}
		}

		if (CallExpr *call = dyn_cast<CallExpr>(s)) {
			FunctionDecl *func = call->getDirectCallee();
			const std::string funcName = func->getNameInfo().getAsString();
			if (funcName == "printf") {
				for(int i=0, j=call->getNumArgs(); i<j; i++){
					QualType QT = call->getArg(i)->getType();
      		std::string TypeStr = QT.getAsString();
					std::string Arg;
					llvm::raw_string_ostream stream(Arg);
					llvm::errs()<<"cal arg:\n";
					call->getArg(i)->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
					stream.flush();
					if(TypeStr == "double" || TypeStr == "float"){
      			if (const ImplicitCastExpr *Cast = llvm::dyn_cast<ImplicitCastExpr>(call->getArg(i))) {
      				if (const DeclRefExpr *DeclRef = llvm::dyn_cast<DeclRefExpr>(Cast->getSubExpr())) {
								std::string temp = "tmp"+std::to_string(tmpCount);
								tmpCount++;
								TheRewriter.InsertText(s->getLocStart().getLocWithOffset(-1), "double "+temp+"=convertP32ToDouble("+Arg+");" ,true, true);
								TheRewriter.ReplaceText(call->getArg(i)->getLocStart(), Arg.size(), temp);
							}
      			}
					} 	


				}

			for(int i=0, j=call->getNumArgs(); i<j; i++){
/*
					if (isa<CallExpr>(call->getArg(i))) {
						CallExpr *call = dyn_cast<CallExpr>(call->getArg(i)); 
						if(call != NULL){
						FunctionDecl *func = call->getDirectCallee();
						if(func != NULL){
						const std::string funcName = func->getNameInfo().getAsString();
						llvm::errs()<<"VisitCallExpr:"<<funcName<<"\n";
						if(funcName == "sqrt"){
							TheRewriter.InsertText(call->getDirectCallee()->getLocStart(), "p32_" ,true, true);
						}
						}
					}
			}*/
				if(BinaryOperator *bo = dyn_cast<BinaryOperator>(call->getArg(i))){
					llvm::errs()<<"call arg is bo:\n";
					QualType QT = bo->getType();
      		std::stringstream SSBefore;
      		std::string TypeStr = QT.getAsString();
					size_t op =	bo->getOpcode();
					if(TypeStr == "double" || TypeStr == "float"){
						std::string st;
						llvm::raw_string_ostream stream(st);
						call->getArg(i)->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
						stream.flush();
						std::cout<<"bo in fn arg********************************:"<<st<<":"<<st.length()<<"\n";
						SourceRange VarRange = call->getArg(i)->getSourceRange();
				
						SourceLocation StartLoc =  call->getArg(i)->getLocStart();
    				SourceManager &SM = TheRewriter.getSourceMgr();
  					const char *StartBuf = SM.getCharacterData(StartLoc);
  					int Offset = 0;
  					while (*StartBuf != '=') {
    					StartBuf--;
    					Offset--;
  					}

						StartLoc =  call->getArg(i)->getLocEnd();
  					const char *End = SM.getCharacterData(StartLoc);
  					const char *EndBuf = SM.getCharacterData(StartLoc);
  					int EndOffset = 0;

  					while (*EndBuf != '\n') {
    					EndBuf++;
    					EndOffset++;
  					}
	
						SourceLocation EndLoc = getEndLocationFromBegin( call->getArg(i)->getSourceRange());
  					if (!EndLoc.isInvalid()){
  						const char *Buf = SM.getCharacterData(EndLoc);
							int Offset = 0;
  						while (*Buf != ';') {
    						Buf++;
    						if (*Buf == '\0')
      						break;
   	 						Offset++;
  						}
  						EndLoc = EndLoc.getLocWithOffset(Offset);
						}

    				if (EndLoc.isMacroID())
      				EndLoc = SM.getSpellingLoc(EndLoc);

						TheRewriter.RemoveText(SourceRange( call->getArg(i)->getLocStart(), call->getArg(i)->getLocEnd()));
//						SourceLocation SrcLoc =  call->getArg(i)->getLocStart().getLocWithOffset(Offset);
	//					TheRewriter.InsertText(SrcLoc, ";" ,true, true);
	//					TheRewriter.InsertText(s->getLocStart().getLocWithOffset(-1), "test;\n" ,true, true);
				//		traverseBinOPAST(dyn_cast<Stmt>( call->getArg(i)));
				//		processBinOPCall(dyn_cast<Stmt>( call->getArg(i)), "",  s->getLocStart().getLocWithOffset(-1));
					}
				}
			}
			}
		}

		if(CompoundAssignOperator *CAO = dyn_cast<CompoundAssignOperator>(s)){
			QualType QT = CAO->getType();
      std::stringstream SSBefore;
      std::string TypeStr = QT.getAsString();
			size_t op =	CAO->getOpcode();
			if(TypeStr == "double" || TypeStr == "float"){
					llvm::errs()<<"CompoundAssignOperator float\n";
					std::string st;
					llvm::raw_string_ostream stream(st);
					s->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
					stream.flush();

					SourceLocation StartLoc = s->getLocEnd();
    			SourceManager &SM = TheRewriter.getSourceMgr();

  				const char *EndBuf = SM.getCharacterData(StartLoc);
  				int EndOffset = 0;

  				while (*EndBuf != ';') {
    				EndBuf++;
    				EndOffset++;
  				}
  				SourceLocation StartLoc1 = s->getSourceRange().getBegin();
					SourceLocation EndLoc = getEndLocationFromBegin(s->getSourceRange());
  				if (!EndLoc.isInvalid()){
  					const char *Buf = SM.getCharacterData(EndLoc);
						int Offset = 0;
  					while (*Buf != ';') {
    					Buf++;
    					if (*Buf == '\0')
      					break;
   	 					Offset++;
  					}

  					EndLoc = EndLoc.getLocWithOffset(Offset);
					}

    			if (EndLoc.isMacroID())
      			EndLoc = SM.getSpellingLoc(EndLoc);
    

					std::string IndentStr = getStmtIndentString(StartLoc1);
					TheRewriter.RemoveText(getLocationSR(SourceRange(StartLoc1, EndLoc)));
					traverseBinOPAST(s);
					processBinOP(s, EndLoc, IndentStr);
	
			}
		}
		if(BinaryOperator *bo = dyn_cast<BinaryOperator>(s)){
			llvm::errs()<<"found bo:\n";
//			bo->dump();
			QualType QT = bo->getType();
      std::stringstream SSBefore;
      std::string TypeStr = QT.getAsString();
			size_t op =	bo->getOpcode();
			if(TypeStr == "double" || TypeStr == "float"){
				if(op==21){
					std::string st;
					llvm::raw_string_ostream stream(st);
					s->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
					stream.flush();
					SourceLocation StartLoc = s->getLocEnd();

    			SourceManager &SM = TheRewriter.getSourceMgr();
  				const char *EndBuf = SM.getCharacterData(StartLoc);

					llvm::errs()<<"st:"<<st<<"\n";	

  				SourceLocation StartLoc1 = s->getSourceRange().getBegin();
					SourceLocation EndLoc = getEndLocationFromBegin(s->getSourceRange());
  				if (!EndLoc.isInvalid()){
  					const char *Buf = SM.getCharacterData(EndLoc);
						int Offset = 0;
  					while (*Buf != ';') {
    					Buf++;
    					if (*Buf == '\0')
      					break;
   	 					Offset++;
  					}

  					EndLoc = EndLoc.getLocWithOffset(Offset);
					}

    			if (EndLoc.isMacroID())
      			EndLoc = SM.getSpellingLoc(EndLoc);
			
					std::string IndentStr = getStmtIndentString(StartLoc1);
			//		TheRewriter.RemoveText(SourceRange(s->getLocStart(), EndLoc));
					TheRewriter.RemoveText(getLocationSR(SourceRange(StartLoc1, EndLoc)));
					traverseBinOPAST(s);
					processBinOP(s, EndLoc, IndentStr);
				}
			}
		}
		return true;
	}
	bool VisitCallExpr(CallExpr *CE){
		FunctionDecl *func = CE->getDirectCallee();
		const std::string funcName = func->getNameInfo().getAsString();
		llvm::errs()<<"VisitCallExpr:"<<funcName<<"\n";
		if(funcName == "sqrt"){
//			TheRewriter.InsertText(CE->getDirectCallee()->getLocStart(), "p32_" ,true, true);
		}
		return true;
	}

	bool VisitIfStmt(IfStmt *s) {
		llvm::errs()<<"VisitIfStmt\n:";
		if (const BinaryOperator *BinOP = llvm::dyn_cast<BinaryOperator>(s->getCond())) {
			llvm::errs()<<"VisitIfStmt1\n:";
			std::string stmt;
    	if (BinOP->getOpcode() == BO_EQ) {
				stmt = "p32_eq";
			}
    	if (BinOP->getOpcode() == BO_LE) {
				stmt = "p32_le";
			}
    	if (BinOP->getOpcode() == BO_LT) {
				stmt = "p32_lt";
			}
    	if (BinOP->getOpcode() == BO_GE) {
				stmt = "!p32_lt";
			}
    	if (BinOP->getOpcode() == BO_GT) {
				stmt = "!p32_le";
			}
      	const Expr *LHS = BinOP->getLHS();
      	const Expr *RHS = BinOP->getRHS();
				QualType QT = LHS->getType();
    		std::string TypeStr = QT.getAsString();
				llvm::errs()<<"VisitIfStmt2 TypeStr:"<<TypeStr<<"\n";
			
				if(TypeStr == "double" || TypeStr == "float"){
					std::string strLHS;
					llvm::raw_string_ostream stream(strLHS);
					LHS->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
					stream.flush();
					llvm::errs()<<"VisitIfStmt LHS:"<<strLHS<<"\n";

					std::string strRHS;
					llvm::raw_string_ostream stream1(strRHS);
					RHS->printPretty(stream1, NULL, PrintingPolicy(LangOptions()));
					stream1.flush();
					llvm::errs()<<"VisitIfStmt RHS:"<<strRHS<<"\n";
    			SourceManager &SM = TheRewriter.getSourceMgr();
					const char *Buf = SM.getCharacterData(LHS->getLocStart());
					int Offset = 0;                                                                            
  				while (*Buf != ')') {
   					Buf++;
    				Offset++;
					}
					TheRewriter.ReplaceText(LHS->getLocStart(), Offset, stmt+"("+strLHS+","+strRHS+")");

      		if (const ImplicitCastExpr *Cast = llvm::dyn_cast<ImplicitCastExpr>(LHS)) {
        		LHS = Cast->getSubExpr();
      		}

      		if (const DeclRefExpr *DeclRef = llvm::dyn_cast<DeclRefExpr>(LHS)) {
        		if (const VarDecl *Var = llvm::dyn_cast<VarDecl>(DeclRef->getDecl())) {
          		if (Var->getType()->isPointerType()) {
//            	Var->dump();  // YAY found it!!
          		}
        		}
      		}
    		}
  	}
  	return true;
	}
	bool VisitConditionalOperator(ConditionalOperator *CO){
		if (const BinaryOperator *BinOP = llvm::dyn_cast<BinaryOperator>(CO->getCond())) {
			llvm::errs()<<"VisitConditionalOperator\n:";
			std::string stmt;
    	if (BinOP->getOpcode() == BO_EQ) {
				stmt = "p32_eq";
			}
    	if (BinOP->getOpcode() == BO_LE) {
				stmt = "p32_le";
			}
    	if (BinOP->getOpcode() == BO_LT) {
				stmt = "p32_lt";
			}
    	if (BinOP->getOpcode() == BO_GE) {
				stmt = "!p32_lt";
			}
    	if (BinOP->getOpcode() == BO_GT) {
				stmt = "!p32_le";
			}
      const Expr *LHS = BinOP->getLHS();
      const Expr *RHS = BinOP->getRHS();
			QualType QT = LHS->getType();
    	std::string TypeStr = QT.getAsString();
			llvm::errs()<<"VisitConditionalOperator TypeStr:"<<TypeStr<<"\n";
			
				if(TypeStr == "double" || TypeStr == "float"){
					std::string strLHS;
					llvm::raw_string_ostream stream(strLHS);
					LHS->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
					stream.flush();
					llvm::errs()<<"VisitConditionalOperator LHS:"<<strLHS<<"\n";

					std::string strRHS;
					llvm::raw_string_ostream stream1(strRHS);
					RHS->printPretty(stream1, NULL, PrintingPolicy(LangOptions()));
					stream1.flush();
					llvm::errs()<<"VisitConditionalOperator RHS:"<<strRHS<<"\n";
    			SourceManager &SM = TheRewriter.getSourceMgr();
					const char *Buf = SM.getCharacterData(LHS->getLocStart());
					int Offset = 0;                                                                            
  				while (*Buf != ')') {
   					Buf++;
    				Offset++;
					}
					TheRewriter.ReplaceText(LHS->getLocStart(), Offset, stmt+"("+strLHS+","+strRHS+")");

      		if (const ImplicitCastExpr *Cast = llvm::dyn_cast<ImplicitCastExpr>(LHS)) {
        		LHS = Cast->getSubExpr();
      		}

      		if (const DeclRefExpr *DeclRef = llvm::dyn_cast<DeclRefExpr>(LHS)) {
        		if (const VarDecl *Var = llvm::dyn_cast<VarDecl>(DeclRef->getDecl())) {
          		if (Var->getType()->isPointerType()) {
//            	Var->dump();  // YAY found it!!
          		}
        		}
      		}
    		}
  	}
		
		return true;
	}

	bool VisitRecordDecl(RecordDecl *RD) {
		RecordDecl::field_iterator Iter(RD->field_begin());
  	RecordDecl::field_iterator End(RD->field_end());
  	const FieldDecl *Last = nullptr;
  	for (; Iter != End; ++Iter){
			QualType QT = (*Iter)->getType();
    	std::stringstream SSBefore;
    	std::string TypeStr = QT.getAsString();
			
			if(TypeStr == "float")
				TheRewriter.ReplaceText((*Iter)->getLocStart(), 6, "posit16_t ");
			if(TypeStr == "float *")
    		TheRewriter.ReplaceText((*Iter)->getLocStart(), 6, "posit16_t ");
			if(TypeStr.find("double") || TypeStr.find("volatile double") == 0){
				TheRewriter.ReplaceText((*Iter)->getLocStart(),  6, "posit32_t");
			}
		}
		return true;
	}

SourceRange getLocationSR(SourceRange Range)
{
	SourceManager &SrcManager = TheRewriter.getSourceMgr();
  SourceLocation StartLoc = Range.getBegin();
  SourceLocation EndLoc = Range.getEnd();
  if (StartLoc.isInvalid())
    return StartLoc;
  if (EndLoc.isInvalid())
    return EndLoc;

  if (StartLoc.isMacroID())
    StartLoc = SrcManager.getFileLoc(StartLoc);
  if (EndLoc.isMacroID())
    EndLoc = SrcManager.getFileLoc(EndLoc);

  SourceRange NewRange(StartLoc, EndLoc);
  int LocRangeSize = TheRewriter.getRangeSize(NewRange);
  if (LocRangeSize == -1)
    return NewRange;
	return NewRange;
//  return StartLoc.getLocWithOffset(LocRangeSize);
}

SourceLocation getEndLocationFromBegin(SourceRange Range)
{
  SourceManager &SrcManager = TheRewriter.getSourceMgr();
  SourceLocation StartLoc = Range.getBegin();
  SourceLocation EndLoc = Range.getEnd();
  if (StartLoc.isInvalid())
    return StartLoc;
  if (EndLoc.isInvalid())
    return EndLoc;

  if (StartLoc.isMacroID())
    StartLoc = SrcManager.getFileLoc(StartLoc);
  if (EndLoc.isMacroID())
    EndLoc = SrcManager.getFileLoc(EndLoc);

  SourceRange NewRange(StartLoc, EndLoc);
  int LocRangeSize = TheRewriter.getRangeSize(NewRange);
  if (LocRangeSize == -1)
    return NewRange.getEnd();

  return StartLoc.getLocWithOffset(LocRangeSize);
}

	bool VisitDeclStmt(DeclStmt *DS)                                      
	{
		Decl *PrevDecl = NULL;
		VarDecl *VD = NULL;
		int NumDecls = 0;
		int VarPos = 0;

		for (DeclStmt::const_decl_iterator I = DS->decl_begin(),
       E = DS->decl_end(); I != E; ++I) {
			NumDecls++;

			if (VD)
				continue;

			Decl *CurrDecl = (*I);
   
			VD = dyn_cast<VarDecl>(CurrDecl);
			VarPos = NumDecls - 1;
		} 
		if (!VD)
			return true;

		bool IsFirstDecl = (!VarPos);
		SourceManager &SrcManager = TheRewriter.getSourceMgr();
		TypeLoc VarTypeLoc = VD->getTypeSourceInfo()->getTypeLoc();
		const IdentifierInfo *Id = VD->getType().getBaseTypeIdentifier();
		SourceLocation EndLoc;
		if (!Id) {
			EndLoc = VD->getLocation();
			const char *Buf = SrcManager.getCharacterData(EndLoc);
			int Offset = -1;
			SourceLocation NewEndLoc = EndLoc.getLocWithOffset(Offset);
			if (NewEndLoc.isValid()){
				Buf--;
				while (isspace(*Buf) || (*Buf == '*') || (*Buf == '(')) {
					Offset--;
					NewEndLoc = EndLoc.getLocWithOffset(Offset);
					if (!NewEndLoc.isValid()){
						EndLoc = EndLoc.getLocWithOffset(Offset+1);
						break;
					}
      		Buf--;
   	 		}
    		EndLoc = EndLoc.getLocWithOffset(Offset+1);
  		}
		}
		TypeLoc NextTL = VarTypeLoc.getNextTypeLoc();
 		while (!NextTL.isNull()) {
			VarTypeLoc = NextTL;
			NextTL = NextTL.getNextTypeLoc();
		}

		SourceRange TypeLocRange = VarTypeLoc.getSourceRange();
		EndLoc = getEndLocationFromBegin(TypeLocRange);

		const Type *Ty = VarTypeLoc.getTypePtr();
		QualType QT = VarTypeLoc.getType();
		std::string TypeStr = QT.getAsString();
  	
		SourceLocation StmtLoc = DS->getLocStart();
		std::string VarStmt = "posit32_t ";
		if(TypeStr == "double")
			TheRewriter.ReplaceText(VarTypeLoc.getLocStart(), 6, "posit32_t");
		
  	return true;
	}

	bool VisitVarDecl(VarDecl *VD) {
		QualType QT = VD->getType();
		std::stringstream SSBefore;
		std::string TypeStr = QT.getAsString();
		Expr *e = VD->getInit();
		SourceLocation StartLoc1 = VD->getSourceRange().getBegin();
		if(e != NULL){
			if(ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(e)){
				if(FloatingLiteral *FL = dyn_cast<FloatingLiteral>(ICE->getSubExpr())){
						TheRewriter.InsertText(e->getExprLoc(), "convertDoubleToP32(", true);
    				TheRewriter.InsertTextAfterToken(e->getLocEnd(), ")");
				}
				if (IntegerLiteral *IL = llvm::dyn_cast<IntegerLiteral>(ICE->getSubExpr())) { 
						TheRewriter.InsertText(e->getExprLoc(), "convertDoubleToP32(", true);
    				TheRewriter.InsertTextAfterToken(e->getLocEnd(), ")");
				}
				if(UnaryOperator *UO = dyn_cast<UnaryOperator>(ICE->getSubExpr())){
					if(FloatingLiteral *FL = dyn_cast<FloatingLiteral>(UO->getSubExpr())){
						TheRewriter.InsertText(e->getExprLoc(), "convertDoubleToP32(", true);
    				TheRewriter.InsertTextAfterToken(e->getLocEnd(), ")");
					}
					if (IntegerLiteral *IL = llvm::dyn_cast<IntegerLiteral>(UO->getSubExpr())) { 
						TheRewriter.InsertText(e->getExprLoc(), "convertDoubleToP32(", true);
    				TheRewriter.InsertTextAfterToken(e->getLocEnd(), ")");
					}
				}
			}
			if(FloatingLiteral *FL = dyn_cast<FloatingLiteral>(e)){
				TheRewriter.InsertText(e->getExprLoc(), "convertDoubleToP32(", true);
    		TheRewriter.InsertTextAfterToken(e->getLocEnd(), ")");
			}
			if(UnaryOperator *UO = dyn_cast<UnaryOperator>(e)){
				if(FloatingLiteral *FL = dyn_cast<FloatingLiteral>(UO->getSubExpr())){
					TheRewriter.InsertText(e->getExprLoc(), "convertDoubleToP32(", true);
    			TheRewriter.InsertTextAfterToken(e->getLocEnd(), ")");
				}
				if (IntegerLiteral *IL = llvm::dyn_cast<IntegerLiteral>(UO->getSubExpr())) { 
					TheRewriter.InsertText(e->getExprLoc(), "convertDoubleToP32(", true);
    			TheRewriter.InsertTextAfterToken(e->getLocEnd(), ")");
				}
			}
		if(BinaryOperator *bo = dyn_cast<BinaryOperator>(e)){
				QualType QT = bo->getType();
      	std::stringstream SSBefore;
      	std::string TypeStr = QT.getAsString();
				size_t op =	bo->getOpcode();
				if(TypeStr == "double" || TypeStr == "float"){
					std::string st;
					llvm::raw_string_ostream stream(st);
					e->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
					stream.flush();
					std::cout<<"to be removed********************************:"<<st<<":"<<st.length()<<"\n";
					SourceRange VarRange = VD->getSourceRange();
				
					SourceLocation StartLoc = e->getLocStart();
    			SourceManager &SM = TheRewriter.getSourceMgr();
  				const char *StartBuf = SM.getCharacterData(StartLoc);
  				int Offset = 0;
  				while (*StartBuf != '=') {
    				StartBuf--;
    				Offset--;
  				}

					StartLoc = e->getLocEnd();
  				const char *End = SM.getCharacterData(StartLoc);
  				const char *EndBuf = SM.getCharacterData(StartLoc);
  				int EndOffset = 0;

  				while (*EndBuf != '\n') {
    				EndBuf++;
    				EndOffset++;
  				}

          SourceLocation EndLoc = getEndLocationFromBegin(StartLoc1);
          if (!EndLoc.isInvalid()){
            const char *Buf = SM.getCharacterData(EndLoc);
            int Offset = 0;
            while (*Buf != ';') {
              Buf++;
              if (*Buf == '\0')
                break;
              Offset++;
            }

            EndLoc = EndLoc.getLocWithOffset(Offset);
          }

          if (EndLoc.isMacroID())
            EndLoc = SM.getSpellingLoc(EndLoc);
			
					std::string IndentStr = getStmtIndentString(StartLoc1);

					SourceLocation LocStart = getVarDeclTypeLocBegin(VD);  
					TheRewriter.RemoveText(SourceRange(LocStart, EndLoc));
					SourceLocation SrcLoc = e->getLocStart().getLocWithOffset(Offset);
					traverseBinOPAST(dyn_cast<Stmt>(e));
					processBinOPVD(dyn_cast<Stmt>(e), dyn_cast<NamedDecl>(VD)->getNameAsString(), EndLoc, IndentStr);
				}
			}
		}
		return true;
	}
	bool VisitParmVarDecl(ParmVarDecl *PV){
	//	std::string BOR;
	//	llvm::raw_string_ostream stream(BOR);
//		PV->getExpr()->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
//		stream.flush();
//		llvm::errs()<<"pretty print PV:"<<BOR<<"\n";
		QualType QT = PV->getType();
		std::string TypeStr = QT.getAsString();
		
		if(TypeStr.find("double") == 0){
			SourceLocation ST = PV->getSourceRange().getBegin();
    	SourceManager &SM = TheRewriter.getSourceMgr();
			const char *Buf = SM.getCharacterData(ST);
			const char *Buf1 = SM.getCharacterData(ST);
			const char *TypeBuf = TypeStr.c_str();	
			std::string stmt = "posit32_t ";
  		while (*TypeBuf != ' ' || *TypeBuf != '*') {
				TypeBuf++;
				if(*TypeBuf == ' ' || *TypeBuf == '*'){
					stmt = stmt + TypeBuf;
					break;
				}
			}
			stmt = stmt + " "; 
			int star = 0;
			int Offset = 0;                  
			int lastSpace = 0;
			int oldSpace = 0;
  		while (*Buf != ')' || *Buf != ',') {
   			Buf++;
				if(*Buf != ' '){
					lastSpace++;
				}
				else{
					oldSpace = lastSpace;
					lastSpace = 0;
				}
				Offset++;
				if(*Buf == ',')
					break;	
				if(*Buf == ')')
					break;	
			}
			lastSpace = lastSpace -1;
			if(lastSpace == 0)
				lastSpace = oldSpace+1;
			Buf1 = Buf1 + (Offset-lastSpace);
			while (*Buf1 != ')' || *Buf1 != ','){
				if(*Buf1 == ',')
					break;	
				if(*Buf1 == '*')
					star++;
				Buf1++;
				if(*Buf1 == ',')
					break;	
				if(*Buf1 == ')')
					break;	
			}
			std::cout<<"VisitParmVarDecl offset:"<<Offset<<" lastSpace:"<<lastSpace<<" star:"<<star<<"\n";
			TheRewriter.ReplaceText(ST, Offset-lastSpace+star, stmt);
		}
		return true;
	}	
	bool VisitFunctionDecl(FunctionDecl *f) {
    // Only function definitions (with bodies), not declarations.
		QualType QT = f->getReturnType();
		std::string TypeStr = QT.getAsString();
		if(TypeStr.find("float") == 0){
			SourceLocation ST = f->getReturnTypeSourceRange().getBegin();
			TheRewriter.ReplaceText(ST, 6,"posit16_t ");
		}
		if(TypeStr.find("double") == 0){
			SourceLocation ST = f->getReturnTypeSourceRange().getBegin();
			TheRewriter.ReplaceText(ST, 6,"posit32_t ");
		}
    if (f->hasBody()) {
			Stmt *FuncBody = f->getBody();
			// Type name as string
			QualType QT = f->getReturnType();
      std::string TypeStr = QT.getAsString();

      // Function name
      DeclarationName DeclName = f->getNameInfo().getName();
      std::string FuncName = DeclName.getAsString();
    }
    return true;
  }

	private:
  	Rewriter &TheRewriter;
};

class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R) : Visitor(R) {}
  // Override the method that gets called for each parsed top-level
  // declaration.
  bool HandleTopLevelDecl(DeclGroupRef DR) override {
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
      // Traverse the declaration using our AST visitor.
      Visitor.TraverseDecl(*b);
      (*b)->dump();
    }
    return true;
  }

private:
  MyASTVisitor Visitor;
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
		std::error_code OutErrorInfo;
		std::error_code ok;
		llvm::raw_fd_ostream outFile(llvm::StringRef(outName), OutErrorInfo, llvm::sys::fs::F_None);

    TheRewriter.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
    TheRewriter.getEditBuffer(SM.getMainFileID()).write(outFile);
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    llvm::errs() << "** Creating AST consumer for: " << file << "\n";
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return std::make_unique<MyASTConsumer>(TheRewriter);
  }

private:
  Rewriter TheRewriter;
};

int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, ToolingSampleCategory);
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());

  // ClangTool::run accepts a FrontendActionFactory, which is then used to
  // create new objects implementing the FrontendAction interface. Here we use
  // the helper newFrontendActionFactory to create a default factory that will
  // return a new MyFrontendAction object every time.
  // To further customize this, we could create our own factory class.
  return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
