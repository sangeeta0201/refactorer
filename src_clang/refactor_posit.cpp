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

size_t *arrSize;
size_t *countD;
size_t Dim;
typedef llvm::SmallVector<const clang::ArrayType *, 10> ArraySubTypeVector;
SourceLocation IfStartLoc;
static llvm::cl::OptionCategory MyToolCategory("My tool options");
static llvm::cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp MoreHelp("\nMore help text...");
static llvm::cl::opt<string> YourOwnOption("abc", llvm::cl::cat(MyToolCategory));
#define P32 true
#define DoubleSize 6
#ifdef P32
std::string PositMathFunc = "rapl_p32_";
std::string OtherMathFunc = "p32_";
std::string PositLibFunc = "rapl_p32_";
std::string PositTY = "posit_t ";
std::string PositDtoP = "rapl_convertDoubleToP32 ";
std::string PositPtoD = "rapl_convertP32ToDouble ";
std::string PositPtoI32 = "rapl_p32_to_i32 ";
std::string PositPtoI64 = "rapl_p32_to_i64 ";
#elif P16
std::string PositMathFunc = "p16_";
std::string PositTY = "posit16_t ";
std::string PositDtoP = "convertDoubleToP16 ";
std::string PositPtoD = "convertP16ToDouble ";
std::string PositPtoI32 = "p16_to_i32 ";
std::string PositPtoI64 = "p16_to_i64 ";
#elif P8
std::string PositMathFunc = "p8_";
std::string PositTY = "posit8_t ";
std::string PositDtoP = "convertDoubleToP8 ";
std::string PositPtoD = "convertP8ToDouble ";
std::string PositPtoI32 = "p8_to_i32 ";
std::string PositPtoI64 = "p8_to_i64 ";
#endif
std::string FloatingTypeD = "double ";
std::string FloatingType = "double";
std::string FloatingTypeF = "float";
std::stringstream SSBefore;
//track temp variables
unsigned tmpCount = 0;

std::set<const Stmt *> ForceBracesStmts;
std::map<const IfStmt*, SourceLocation> BinIfLoc; 
std::map<const DoStmt*, SourceLocation> BinDoWhileLoc; 
std::map<const BinaryOperator*, std::string> BinOp_Temp; 
std::map<std::string, std::string> Temp_BinOp; 
std::map<const UnaryOperator*, std::string> UOp_Temp; 
std::map<const BinaryOperator*, SourceLocation> BinLoc_Temp; 
std::map<const BinaryOperator*, const CallExpr*> BinParentCE; 
std::map<const BinaryOperator*, const Stmt*> BinParentST; 
std::map<const BinaryOperator*, const ForStmt*> BinParentForST; 
std::map<const BinaryOperator*, const BinaryOperator*> BinParentBO; 
std::map<const BinaryOperator*, const VarDecl*> BinParentVD; 
std::stack<const BinaryOperator*> BOStack; 
SmallVector<unsigned, 8> ProcessedLine;
SmallVector<const FloatingLiteral*, 8> ProcessedFL;
SmallVector<const VarDecl*, 8> ProcessedInitVD;
SmallVector<const InitListExpr*, 8> ProcessedILE;
SmallVector<const IntegerLiteral*, 8> ProcessedIL;
SmallVector<const VarDecl*, 8> ProcessedVD;
SmallVector<const CallExpr*, 8> ProcessedCE;
SmallVector<const CStyleCastExpr*, 8> ProcessedCCast;
SmallVector<const IfStmt*, 8> ProcessedStmt;
std::map<const Expr*, std::string> ProcessedExpr; 

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

int getOffsetParamOld(const char *Buf, char Symbol)
{
  int Offset = 0;
  while (*Buf != Symbol) {
    if(*Buf == ',')
      break;
    Buf++;
    Offset++;
  }
  return Offset;
}
int getOffsetParam(const char *Buf, char Symbol)
{
  int Offset = 0;
  while (*Buf != ')') {
    if(*Buf == ',')
      break;
    Buf++;
    Offset++;
  }
  return Offset;
}

int getOffsetUntil(const char *Buf, char Symbol)
{
  int Offset = 0;
  while (*Buf != Symbol) {
    Buf++;
    Offset++;
  }
  return Offset;
}

unsigned int getConstArraySize( 
    const ConstantArrayType *CstArrayTy)
{
  unsigned int Sz;
  llvm::APInt Result = CstArrayTy->getSize();

  llvm::SmallString<8> IntStr;
  Result.toStringUnsigned(IntStr);

  std::stringstream TmpSS(IntStr.str());

  //  if (!(TmpSS >> Sz)) {
  //   TransAssert(0 && "Non-integer value!");
  //}
  return Sz;
}

unsigned getArraySize(const ArrayType *ATy, const ASTContext *Context)
{
  if (const ConstantArrayType *CstArrayTy =
      dyn_cast<ConstantArrayType>(ATy)) {
    return getConstArraySize(CstArrayTy);
  }

  if (const DependentSizedArrayType *DepArrayTy =
      dyn_cast<DependentSizedArrayType>(ATy)) {
    const Expr *E = DepArrayTy->getSizeExpr();
    Expr::EvalResult Result;
    if (E->EvaluateAsInt(Result, *Context)) {
      llvm::APSInt IVal = Result.Val.getInt();
      return (unsigned)(*IVal.getRawData());
    }
  }

  return 1;
}

unsigned int setSize(size_t *size,
    const ArrayType *ArrayTy)
{                                                                                                                                                                           
  unsigned int Dim = 1;
  const Type *ArrayElemTy = ArrayTy->getElementType().getTypePtr();
  if (const ConstantArrayType *CstArrayTy =
      dyn_cast<ConstantArrayType>(ArrayTy)) {
    *(size + Dim-1) = *CstArrayTy->getSize().getRawData();
  }
  while (ArrayElemTy->isArrayType()) {
    const ArrayType *AT = dyn_cast<ArrayType>(ArrayElemTy);
    ArrayElemTy = AT->getElementType().getTypePtr();
    Dim++;
    if (const ConstantArrayType *CstArrayTy =
        dyn_cast<ConstantArrayType>(AT)) {
      *(size + Dim-1) = *CstArrayTy->getSize().getRawData();
    }
  }
  return Dim;
}


unsigned int getArrayDimensionAndTypes(
    const ArrayType *ArrayTy)
{                                                                                                                                                                           
  unsigned int Dim = 1;
  const Type *ArrayElemTy = ArrayTy->getElementType().getTypePtr();
  if (const ConstantArrayType *CstArrayTy =
      dyn_cast<ConstantArrayType>(ArrayTy)) {
  }
  while (ArrayElemTy->isArrayType()) {
    const ArrayType *AT = dyn_cast<ArrayType>(ArrayElemTy);
    if (const ConstantArrayType *CstArrayTy =
        dyn_cast<ConstantArrayType>(AT)) {
    }
    ArrayElemTy = AT->getElementType().getTypePtr();
    Dim++;
  }
  return Dim;
}
void setDimAndSize(const ArrayType *ArrayTy){
  Dim = getArrayDimensionAndTypes(ArrayTy);
  arrSize = (size_t*)malloc(sizeof(size_t)*Dim);
  countD = (size_t*)malloc(sizeof(size_t)*Dim);
  memset(countD, 0, sizeof(size_t)*Dim);
  setSize(arrSize, ArrayTy);
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
/*      Rewrite.InsertText(StmtStartLoc, 
          "#include \"softposit.h\"\n", true, true);
        Rewrite.InsertText(StmtStartLoc, 
          "#include \"softposit_ext.h\"\n", true, true);
*/
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
    
      std::string buf(StartBuf);
      size_t found = buf.find(VD->getNameAsString()); 
      if (found != string::npos){ 
        StartBuf = StartBuf+(found-1);
        StartOffset = StartOffset+(found-1);
      }

      while (*StartBuf != '[' ){
        StartBuf++;
        StartOffset++;
      }
      while (*StartBuf != ';') {
        if(*StartBuf == ',') 
          break;
        if(*StartBuf == ')') 
          break;
        if(*StartBuf == '=') 
          break;
        StartBuf++;
        Offset++;
      }

      char *result = (char *)malloc(Offset+2);

      strncpy(result, OrigBuf+StartOffset, Offset);
      result[Offset] = '\0'; 
      return result;
    }

    void removeLineBO(SourceLocation StartLoc){
      SourceManager &SM = Rewrite.getSourceMgr();
      const char *StartBuf = SM.getCharacterData(StartLoc);
      unsigned lineNo = getLineNo(StartLoc);
      int Offset = getOffsetUntil(StartBuf, ';');
      Rewriter::RewriteOptions Opts;
      Opts.RemoveLineIfEmpty = false;
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
        Opts.RemoveLineIfEmpty = false;
        Rewrite.RemoveText(StartLoc, Offset+1, Opts); 
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
          funcName = PositLibFunc+"eq";
          break;
        case BO_NE:
          funcName = "!"+PositLibFunc+"eq";
          break;
        case BO_LE:
          funcName = PositLibFunc+"le";
          break;
        case BO_LT:
          funcName = PositLibFunc+"lt";
          break;
        case BO_GE:
          funcName = "!"+PositLibFunc+"lt";
          break;
        case BO_GT:
          funcName = "!"+PositLibFunc+"le";
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
          funcName = PositLibFunc+"mul";
          break;
        case BO_Div:
        case BO_DivAssign:
          funcName = PositLibFunc+"div";
          break;
        case UO_PostInc:
        case BO_Add:
        case BO_AddAssign:
          funcName = PositLibFunc+"add";
          break;
        case UO_PostDec:
        case BO_Sub:
        case BO_SubAssign:
          funcName = PositLibFunc+"sub";
          break;
        case 21:
          funcName = PositLibFunc+"sub";
          break;
        default:
          assert("This opcode is not handled!!!");
      }
      return funcName;
    }

    //func(double x) => func(posit32_t x)
    void ReplaceParmVDWithPositOld(SourceLocation StartLoc, char positLiteral){
      if(!StartLoc.isValid())
        return;
      SourceManager &SM = Rewrite.getSourceMgr();
      const char *StartBuf = SM.getCharacterData(StartLoc);
      int Offset = getOffsetParamOld(StartBuf, positLiteral);
      Rewrite.ReplaceText(SourceRange(StartLoc, StartLoc.getLocWithOffset(Offset-1)), 
          PositTY);	
    }
    //func(double x) => func(posit32_t x)
    void ReplaceParmVDWithPosit(const FunctionDecl *FD, SourceLocation StartLoc, std::string VDName){
      if(!StartLoc.isValid())
        return;
      Stmt *Body = FD->getBody();
      SourceLocation StartLocB = Body->getBeginLoc();
      StmtIterator I = Body->child_begin();
      std::string Indent;
      if (I == Body->child_end())
        Indent = "  ";
      else
        Indent = getStmtIndentString((*I)->getBeginLoc());
      SourceManager &SM = Rewrite.getSourceMgr();
      const char *StartBuf = SM.getCharacterData(StartLoc);
      int Offset = getOffsetParamOld(StartBuf, ' ');
      std::string temp1, temp2;
      temp1 = getTempDest();
      temp2 = getTempDest();

      Rewrite.ReplaceText(SourceRange(StartLoc, StartLoc.getLocWithOffset(Offset-1)), 
          PositTY);	
    }

    void ReplaceVDInitWithPosit(SourceLocation StartLoc, SourceLocation EndLoc, std::string positLiteral,
        std::string VDName){
      if(!StartLoc.isValid())
        return;
      
      std::string temp;
      temp = getTempDest();
      std::string positLiteral1 = PositTY+ temp + " = "+positLiteral+"\n";
      Rewrite.InsertText(StartLoc, positLiteral1, true, true);
      std::string positLiteral2 = VDName+ " = "+temp+";\n";
      Rewrite.InsertText(StartLoc, positLiteral2, true, true);
      removeLineVD(StartLoc);
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

    std::string handleMathFunc(std::string funcName){
      if(funcName == "sqrt" || funcName == "sqrtf"){
        return PositMathFunc+"sqrt"; 
      }
      else if(funcName == "fabsf"|| funcName == "cos" || funcName == "acos" ||
          funcName == "sin" || funcName == "tan" || funcName == "fabs" || funcName == "pow" ||
          funcName == "cosec" || funcName == "strtod" || funcName == "atan" || funcName == "floor" ||
          funcName == "exp"){
        return OtherMathFunc+funcName; 
      }
      else{
        return funcName; 
      }
    }
    //	x = y * 0.3 => t1 = convertdoubletoposit(0.3)
    void ReplaceBOLiteralWithPosit(SourceLocation StartLoc, std::string lhs, std::string rhs){
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
        case::UO_Minus:
        case::UO_PreDec:
        case::UO_PostDec:{
                           std::string func = getPositFuncName(BO_Sub);
                           Rewrite.InsertText(BOStartLoc, 
                               PositTY+temp+" = "+func+"("+Op1+","+Op2+");\n", true, true);
                           //removeLine(BO->getSourceRange().getBegin(), BO->getSourceRange().getEnd());
                           break;
                         }
        default:
                         return Op2;
      }
      return temp;
    }
    //handle all binary operators except assign
    // x = *0.4*y*z => t1 = posit_mul(y,z);
    void ReplaceBOWithPosit(ASTContext &Context, const BinaryOperator *BO, SourceLocation BOStartLoc, 
        std::string Op1, std::string Op2, const Expr *ExOp1, const Expr *ExOp2){
      unsigned Opcode = BO->getOpcode();
      std::string func = getPositFuncName(Opcode);
      SourceManager &SM = Rewrite.getSourceMgr();
      if(!BOStartLoc.isValid()){
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
                       Rewrite.InsertText(BOStartLoc, 
                           PositTY+temp+" = "+func+"("+Op1+","+Op2+");\n", true, true);
                       BinOp_Temp.insert(std::pair<const BinaryOperator*, std::string>(BO, temp));	
                       break;
                     }
        case::BO_Assign:{
                          std::string OrigOp1;
                          if(Temp_BinOp.count(Op1) != 0){
                            OrigOp1 = Temp_BinOp.at(Op1);
                          }
                          else{
                            OrigOp1 = Op1;
                          }
                          std::string positLiteral1 = OrigOp1 +" = "+ Op2+";\n";
                          Rewrite.InsertText(BOStartLoc, positLiteral1, true, true);
                          BinOp_Temp.insert(std::pair<const BinaryOperator*, std::string>(BO, OrigOp1));	
                          removeLineBO(BO->getLHS()->getSourceRange().getBegin());
                          return;
                        }
        case::BO_DivAssign:
        case::BO_MulAssign:
        case::BO_AddAssign:
        case::BO_SubAssign:{
                             std::string OrigOp1;
                             if(Temp_BinOp.count(Op1) != 0){
                                OrigOp1 = Temp_BinOp.at(Op1);
                             }
                             else{
                                OrigOp1 = Op1;
			     }
                             std::string tmpp = getTempDest();
                             Rewrite.InsertText(BOStartLoc, 
                                 PositTY+tmpp+" = "+func+"("+Op1+","+Op2+");\n", true, true);
                             std::string positLiteral1 = OrigOp1 +" = "+ tmpp+";\n";
                             Rewrite.InsertText(BOStartLoc, positLiteral1, true, true);
                             removeLineBO(BOStartLoc);
                             BinOp_Temp.insert(std::pair<const BinaryOperator*, std::string>(BO, Op1));	
                             BO->getRHS();
                             return;
                           }
        default:
		llvm::errs()<<"ReplaceBOWithPosit: Error!!! Operand is unknown\n\n";
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
        const CallExpr *CE = BinParentCE.at(BO);
        const FunctionDecl *Func = CE->getDirectCallee();
        const std::string funcName = Func->getNameInfo().getAsString();
        if(funcName == "printf" || funcName == "fprintf") {
          std::string convert;
          convert = PositPtoD+"(" + temp +");";
          std::string tmp;
          tmp = getTempDest();
          Rewrite.InsertText(CE->getSourceRange().getBegin(), 
              FloatingTypeD+tmp +" = "+convert+"\n", true, true);		
          Rewrite.ReplaceText(SourceRange(StartLoc, EndLoc), tmp);
        }
        else
          Rewrite.ReplaceText(SourceRange(StartLoc, EndLoc), temp);
      }
      else if(BinParentBO.count(BO) != 0){
        const BinaryOperator *ParentBP = BinParentBO.at(BO);
        //handle cast to int
        if(ParentBP->getType()->isIntegerType()){
          if(ParentBP->getRHS()->IgnoreImpCasts() == BO){
            std::string convert = PositPtoI32+"(" + temp +");";
            std::string tmp;
            tmp = getTempDest();
            Rewrite.InsertText(ParentBP->getSourceRange().getBegin(), 
                "int "+tmp +" = "+convert+"\n", true, true);		

            Rewrite.ReplaceText(SourceRange(StartLoc, EndLoc), tmp);
          }
          else{
            Rewrite.ReplaceText(SourceRange(StartLoc, EndLoc), temp);
          }
        }
        else{
          std::string ArgName;
          llvm::raw_string_ostream s(ArgName);
          ParentBP->getLHS()->printPretty(s, NULL, PrintingPolicy(LangOptions()));
        }
      }
      else if(BinParentVD.count(BO) != 0){
        const VarDecl* VD = BinParentVD.at(BO);
        const Stmt* SB = dyn_cast<Stmt>(BO);
        
        //int zi = (int)(z + 0.5);
        //this handles one level of cast
        if(isCastParent(Context, VD, SB)){
          std::string t1;
          t1 = getTempDest();
          if(VD->getType()->isIntegralType(Context)){
            std::string TypeStr = VD->getType().getAsString();
            Rewrite.InsertText(VD->getSourceRange().getBegin(), TypeStr+" "+t1+" = "+PositPtoI32+"("+temp+")"+";\n", true, true);

            Rewrite.InsertText(VD->getSourceRange().getBegin(), TypeStr+" "+VD->getNameAsString()+" = "+t1+";\n", true, true);
            removeLineVD(VD->getSourceRange().getBegin());
          }
        }
        if(isVDParent(Context, VD, SB)){
          SourceLocation StartLoc = VD->getSourceRange().getBegin();
          if (StartLoc.isMacroID())
            StartLoc = SM.getFileLoc(StartLoc);
          Rewrite.InsertText(StartLoc, PositTY+VD->getNameAsString()+";\n", true, true);
          std::string positLiteral1 = VD->getNameAsString() +" = "+ temp+";\n";
          Rewrite.InsertText(StartLoc, positLiteral1, true, true);
          removeLineVD(StartLoc);
        }
      }
      else if(BinParentForST.count(BO) != 0){
//        Rewrite.ReplaceText(SourceRange(BO->getSourceRange().getBegin(), 	
  //            BO->getSourceRange().getEnd()), temp);
      }
      else if(BinParentST.count(BO) != 0){
        SourceLocation StartLoc = BO->getSourceRange().getBegin();
        SourceLocation EndLoc = BO->getSourceRange().getEnd();
        if (StartLoc.isMacroID())
          StartLoc = SM.getFileLoc(StartLoc);
        if (EndLoc.isMacroID())
          EndLoc = SM.getFileLoc(EndLoc);

        Rewrite.ReplaceText(SourceRange(StartLoc, EndLoc), temp);
      }
    }

    const CallExpr* isVDCallParent(ASTContext &Context, const VarDecl *VD, const Stmt *ST){
      const auto& parents = Context.getParents(*ST);
      if ( parents.empty() ) {
        return nullptr;
      }
      if(const CallExpr *CE = parents[0].get<CallExpr>()){
        return CE;
      }
      return nullptr;
    }
    bool isCastParent(ASTContext &Context, const VarDecl *VD, const Stmt *ST){
      const auto& parents = Context.getParents(*ST);
      if ( parents.empty() ) {
        return false;
      }
      if(const CStyleCastExpr *CS = parents[0].get<CStyleCastExpr>()){
        if (!CS){
          return false;
        }
        else{
          return true;
        }
      }
      else if(const ImplicitCastExpr *PE = parents[0].get<ImplicitCastExpr>()){
        return isCastParent(Context, VD, PE);
      }
      else if(const ParenExpr *PE = parents[0].get<ParenExpr>()){
        return isCastParent(Context, VD, PE);
      }
      return false;
    }
    bool isVDParent(ASTContext &Context, const VarDecl *VD, const Stmt *ST){
      const auto& parents = Context.getParents(*ST);
      if ( parents.empty() ) {
        return false;
      }
      if(const VarDecl *PVD = parents[0].get<VarDecl>()){
        if (!PVD)
          return false;
        if (PVD = VD){
          return true;
        }
      }
      else if(const ImplicitCastExpr *PE = parents[0].get<ImplicitCastExpr>()){
        return isVDParent(Context, VD, PE);
      }
      else if(const ParenExpr *PE = parents[0].get<ParenExpr>()){
        return isVDParent(Context, VD, PE);
      }
      return false;
    }

    std::string handleVDLiteralFor(SourceLocation StartLoc, SourceLocation EndLoc,  std::string VDName, 
        const FloatingLiteral *FL, const IntegerLiteral *IL, 
        const ArraySubscriptExpr *ASE, const DeclRefExpr *DE, const UnaryOperator *U_lhs,
        const BinaryOperator *BO, const CallExpr *CE){
      std::string positLiteral;
      if(FL != NULL){
        Rewrite.InsertText(StartLoc, PositTY+VDName+";\n", true, true);
        positLiteral = convertFloatToPosit(FL);
      }
      else if(IL != NULL){
        Rewrite.InsertText(StartLoc, PositTY+VDName+";\n", true, true);
        positLiteral = convertIntToPosit(IL);
      }
      else if(ASE != NULL){
        std::string TypeS;
        llvm::raw_string_ostream s(TypeS);
        ASE->printPretty(s, 0, PrintingPolicy(LangOptions()));
        std::string lhs = getTempDest();
        Rewrite.InsertText(StartLoc, PositTY+VDName+";\n", true, true);
        Rewrite.InsertText(StartLoc, PositTY + lhs+ s.str(), true, true);
        std::string positLiteral2 = VDName +" = "+ lhs+";\n";
        Rewrite.InsertText(StartLoc, positLiteral2, true, true);
      }
      else if(CE != NULL){
        std::string positLiteral = handleCallArgs(StartLoc, CE);
        Rewrite.InsertText(StartLoc, PositTY+VDName+";\n", true, true);
        std::string positLiteral2 = VDName +" = "+ positLiteral+";\n";
        Rewrite.InsertText(StartLoc, positLiteral2, true, true);
      }
      else if(DE != NULL){
        const Type *Ty = DE->getType().getTypePtr();
        if(Ty->isFloatingType()){
          std::string TypeS;
          llvm::raw_string_ostream s(TypeS);
          DE->printPretty(s, 0, PrintingPolicy(LangOptions()));
          std::string lhs = getTempDest();
          Rewrite.InsertText(StartLoc, PositTY+VDName+";\n", true, true);
          //need original name for force_store
          //            Temp_BinOp.insert(std::pair<std::string, std::string>(lhs, Op1));	
          //Rewrite.InsertText(StartLoc, PositTY + lhs+ load, true, true);
          Rewrite.InsertText(StartLoc, PositTY + lhs+ s.str(), true, true);
          positLiteral = VDName +" = "+ lhs+";\n";
        }
        else{ //integral type
          std::string TypeS;
          llvm::raw_string_ostream s(TypeS);
          DE->printPretty(s, 0, PrintingPolicy(LangOptions()));
          positLiteral = PositDtoP+"("+s.str()+")";
        }
      }
      else if(U_lhs != NULL){
        std::string TypeS;
        llvm::raw_string_ostream s(TypeS);
        Rewrite.InsertText(StartLoc, PositTY+VDName+";\n", true, true);
        U_lhs->printPretty(s, 0, PrintingPolicy(LangOptions()));
        positLiteral = PositDtoP+"("+s.str()+");";
      }
      else if(BO != NULL){
        std::string TypeS;
        llvm::raw_string_ostream s(TypeS);
        BO->printPretty(s, 0, PrintingPolicy(LangOptions()));
        positLiteral = PositDtoP+"("+s.str()+");";
      }
      return positLiteral;
    }
    void handleVDLiteral(SourceLocation StartLoc, SourceLocation EndLoc,  std::string VDName, 
        const FloatingLiteral *FL, const IntegerLiteral *IL, 
        const ArraySubscriptExpr *ASE, const DeclRefExpr *DE, const UnaryOperator *U_lhs,
        const BinaryOperator *BO, const CallExpr *CE){
      if(FL != NULL){
        Rewrite.InsertText(StartLoc, PositTY+VDName+";\n", true, true);
        std::string positLiteral = convertFloatToPosit(FL);
        ReplaceVDInitWithPosit(StartLoc, EndLoc, 
            positLiteral, VDName);
      }
      else if(IL != NULL){
        Rewrite.InsertText(StartLoc, PositTY+VDName+";\n", true, true);
        std::string positLiteral = convertIntToPosit(IL);
        ReplaceVDInitWithPosit(StartLoc, EndLoc, 
            positLiteral, VDName);
      }
      else if(ASE != NULL){
        std::string TypeS;
        llvm::raw_string_ostream s(TypeS);
        ASE->printPretty(s, 0, PrintingPolicy(LangOptions()));
        std::string lhs = getTempDest();
        Rewrite.InsertText(StartLoc, PositTY+VDName+";\n", true, true);
        Rewrite.InsertText(StartLoc, PositTY + lhs+ s.str(), true, true);
        std::string positLiteral2 = VDName +" = "+ lhs+";\n";
        Rewrite.InsertText(StartLoc, positLiteral2, true, true);
        removeLineVD(StartLoc);
      }
      else if(CE != NULL){
        std::string positLiteral = handleCallArgs(StartLoc, CE);
        Rewrite.InsertText(StartLoc, PositTY+VDName+";\n", true, true);
        std::string positLiteral2 = VDName +" = "+ positLiteral+";\n";
        Rewrite.InsertText(StartLoc, positLiteral2, true, true);
        removeLineVD(StartLoc);
      }
      else if(DE != NULL){
        const Type *Ty = DE->getType().getTypePtr();
        if(Ty->isFloatingType()){
          std::string TypeS;
          llvm::raw_string_ostream s(TypeS);
          DE->printPretty(s, 0, PrintingPolicy(LangOptions()));
          std::string lhs = getTempDest();
          Rewrite.InsertText(StartLoc, PositTY+VDName+";\n", true, true);
          Rewrite.InsertText(StartLoc, PositTY + lhs+ s.str(), true, true);
          std::string positLiteral2 = VDName +" = "+ lhs+";\n";
          Rewrite.InsertText(StartLoc, positLiteral2, true, true);
          removeLineVD(StartLoc);
        }
        else{ //integral type
          std::string TypeS;
          llvm::raw_string_ostream s(TypeS);
          DE->printPretty(s, 0, PrintingPolicy(LangOptions()));
          std::string positLiteral = PositDtoP+"("+s.str()+")";
          ReplaceVDInitWithPosit(StartLoc, EndLoc, 
              positLiteral, VDName);
        }
      }
      else if(U_lhs != NULL){
        std::string TypeS;
        llvm::raw_string_ostream s(TypeS);
        Rewrite.InsertText(StartLoc, PositTY+VDName+";\n", true, true);
        U_lhs->printPretty(s, 0, PrintingPolicy(LangOptions()));
        std::string positLiteral = PositDtoP+"("+s.str()+");";
        ReplaceVDInitWithPosit(StartLoc, EndLoc, 
            positLiteral, VDName);
      }
      else if(BO != NULL){
        std::string TypeS;
        llvm::raw_string_ostream s(TypeS);
        BO->printPretty(s, 0, PrintingPolicy(LangOptions()));
        std::string positLiteral = PositDtoP+"("+s.str()+");";
        ReplaceVDInitWithPosit(StartLoc, EndLoc, 
            positLiteral, VDName);
      }

    }
    void handleBinOp(ASTContext &Context){
      const BinaryOperator *BO;
      while(BOStack.size() > 0){
        BO = BOStack.top();
        BOStack.pop();
        //check if this binaryoperator is processed
        //check if LHS is processed, get its temp variable
        //check if RHS is processed, get its temp variable
        //create new varible, if this BO doesnt have a VD as its LHS
        //push this in map
        //if(BinLoc_Temp.count(BO) == 0)
        //  return;
        SourceLocation StartLoc = BinLoc_Temp.at(BO);

        unsigned lineNo = getLineNo(StartLoc);
        unsigned opCode = BO->getOpcode();
        Expr *Op1 = removeParen(BO->getLHS());
        Expr *Op2 = removeParen(BO->getRHS());
        std::string Op1Str = handleOperand(StartLoc, BO, Op1);
        std::string Op2Str = handleOperand(StartLoc, BO, Op2);

        SourceManager &SM = Rewrite.getSourceMgr();



        ReplaceBOWithPosit(Context, BO, StartLoc, Op1Str, Op2Str, Op1, Op2);
      }	
    }
    bool isPointerToFloatingType(const Type *Ty){
      QualType QT = Ty->getPointeeType();
      while (!QT.isNull()) {
        Ty = QT.getTypePtr();
        if(isa<TypedefType>(Ty))
          return false;//no need to change typedefs
        QT = Ty->getPointeeType();
      }
      if(Ty->isFloatingType()){
        return true;
      }
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
      while (isa<ImplicitCastExpr>(Op) || isa<ParenExpr>(Op)) {
        if(isa<ImplicitCastExpr>(Op)){
          ImplicitCastExpr *PE = llvm::dyn_cast<ImplicitCastExpr>(Op);
          Op = PE->getSubExpr();
        }
        if(isa<ParenExpr>(Op)){
          ParenExpr *PE = llvm::dyn_cast<ParenExpr>(Op);
          Op = PE->getSubExpr();
        }
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

    void convertPToD(SourceLocation CEStartLoc, SourceLocation CEArgStartLoc, std::string Arg){
      SourceManager &SM = Rewrite.getSourceMgr();
      std::string convert;
      convert = PositPtoD+"(" + Arg+");";
      std::string temp;
      temp = getTempDest();
      Rewrite.InsertText(CEStartLoc, 
          FloatingTypeD+temp +" = "+convert+"\n", true, true);		
      const char *Buf = SM.getCharacterData(CEArgStartLoc);
      int StartOffset = 0, openB = 0, closeB = 0;
      while (*Buf != ';') {
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
      Rewrite.ReplaceText(CEArgStartLoc, StartOffset, temp);
    }

    std:: string handleCCast(SourceLocation StartLoc, const CStyleCastExpr *CS, std::string Arg){
      const Type *Ty =CS->getType().getTypePtr();
      QualType QT = Ty->getPointeeType();
      std::string indirect="";
      while (!QT.isNull()) {
        Ty = QT.getTypePtr();
        QT = Ty->getPointeeType();
        indirect += "*";
      }
      size_t pos = Arg.find(FloatingType);
      if (pos != std::string::npos){
        Arg.replace(pos, FloatingType.size(), PositTY); 
      }
      std::string temp;
      temp = getTempDest();
      Rewrite.InsertText(StartLoc,
          PositTY+temp +" = "+Arg+";\n", true, true);                                                                                                                 
      return temp;
    }

    std::string handleCCondition(SourceLocation StartLoc, const ConditionalOperator *CO, const BinaryOperator *BinOp){
      const Type *Ty = BinOp->getLHS()->getType().getTypePtr();
      SourceManager &SM = Rewrite.getSourceMgr();
      if(isPointerToFloatingType(Ty))
        if(Ty->isPointerType())
          return nullptr;

      std::string func = getPositBinOp(BinOp->getOpcode());
      unsigned opCode = BinOp->getOpcode();
      std::string Op1 = handleOperand(StartLoc, BinOp, BinOp->getLHS());
      std::string Op2 = handleOperand(StartLoc, BinOp, BinOp->getRHS());

      std::string Op, Opp;
      std::string Op11 = handleOperand(StartLoc, nullptr, CO->getLHS());
      std::string Op12 = handleOperand(StartLoc, nullptr, CO->getRHS());
      Op = func+"("+Op1+","+Op2+")"+"?"+Op11+":"+Op12+";";

      return Op;
    }

    std::string handleCondition(SourceLocation StartLoc, const BinaryOperator *BinOp){
      const Type *Ty = BinOp->getLHS()->getType().getTypePtr();
      SourceManager &SM = Rewrite.getSourceMgr();

      if(isPointerToFloatingType(Ty)){
        if(Ty->isPointerType()){
          return "";
        }
        if(ProcessedExpr.count(BinOp) != 0){
          return ProcessedExpr.at(BinOp);
        }
        std::string func = getPositBinOp(BinOp->getOpcode());
        Expr *BOp1 = removeParen(BinOp->getLHS());
        Expr *BOp2 = removeParen(BinOp->getRHS());
        std::string Op1 = handleOperand(StartLoc, BinOp, BOp1);
        std::string Op2 = handleOperand(StartLoc, BinOp, BOp2);

        std::string op = func+"("+Op1+","+Op2+")";
        ProcessedExpr.insert(std::pair<const Expr*, std::string>(BinOp, op));	
        return op;
      }
      else{
        return "";
      }
    }

    std::string handleCallArgs(SourceLocation StartLoc, const CallExpr *CE){
      if(ProcessedExpr.count(CE) != 0){
        return ProcessedExpr.at(CE);
      }
      const FunctionDecl *FD = CE->getDirectCallee();
      bool setArgFlag = false;
      std::string funcName = handleMathFunc(FD->getNameInfo().getAsString());
      std::string Op1 = funcName+"(";
      if (FD->isThisDeclarationADefinition() &&
          FD->hasBody() &&
          !FD->isDeleted() &&
          !FD->isDefaulted()){
        setArgFlag = true;
      }
      for(int i=0, j=CE->getNumArgs(); i<j; i++){
        const Type *Ty = CE->getArg(i)->getType().getTypePtr();
        if(Ty->isFloatingType()){
          auto Arg = dyn_cast<BinaryOperator>(CE->getArg(i));
          std::string arg = handleOperand(StartLoc, CE, CE->getArg(i)->IgnoreParenImpCasts()->IgnoreParens());
          Op1 += arg;
        }
        else{
          std::string TypeS;
          llvm::raw_string_ostream s(TypeS);
          CE->getArg(i)->printPretty(s, 0, PrintingPolicy(LangOptions()));
          Op1 += s.str();
        }
        if(i+1 != j)
          Op1 += ",";
      }
      Op1 += ");";
      if(isPointerToFloatingType(FD->getReturnType().getTypePtr())){
          std::string t1 = getTempDest();
          std::string lhs = getTempDest();

          Rewrite.InsertText(StartLoc, PositTY+t1+" = "+Op1+"\n", true, true);
          Op1 = t1;
      }
      else{
        Rewrite.InsertText(StartLoc, Op1+"\n", true, true);
      }

      ProcessedExpr.insert(std::pair<const Expr*, std::string>(CE, Op1));
      return Op1;
    }

    SourceLocation getParentLoc(const MatchFinder::MatchResult &Result, const BinaryOperator *BO){
      const ASTContext *Context = Result.Context;	
      const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("callbo");
      const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclbo");
      const Stmt *ST = Result.Nodes.getNodeAs<clang::ReturnStmt>("returnbo");
      auto IfST = Result.Nodes.getNodeAs<IfStmt>("ifstmtbo");
      auto WhileST = Result.Nodes.getNodeAs<WhileStmt>("whilebo");
      auto DoST = Result.Nodes.getNodeAs<DoStmt>("dostmtbo");
      auto ForST = Result.Nodes.getNodeAs<ForStmt>("forstmtbo");
      auto BA = Result.Nodes.getNodeAs<clang::BinaryOperator>("bobo");
      auto BC = Result.Nodes.getNodeAs<clang::BinaryOperator>("cbobo");
      SourceManager &SM = Rewrite.getSourceMgr();
      SourceLocation StartLoc;	
     
      if(CE){
        StartLoc = CE->getSourceRange().getBegin();
        if (StartLoc.isMacroID()) {
          StartLoc = SM.getFileLoc(StartLoc);
        }
      }
      else if(VD){
        StartLoc = VD->getSourceRange().getBegin();
        if (StartLoc.isMacroID()) {
          StartLoc = SM.getFileLoc(StartLoc);
        }
        BinParentVD.insert(std::pair<const BinaryOperator*, const VarDecl*>(BO, VD));	
      }
      else if(BC){
        StartLoc = BC->getSourceRange().getBegin();
        if (StartLoc.isMacroID()) {
          StartLoc = SM.getFileLoc(StartLoc); 
        }
        BinParentBO.insert(std::pair<const BinaryOperator*, const BinaryOperator*>(BO, BC));	
      }
      else if(BA){
        StartLoc = BA->getSourceRange().getBegin();
        if (StartLoc.isMacroID()) {
          StartLoc = SM.getFileLoc(StartLoc); 
        }
        BinParentBO.insert(std::pair<const BinaryOperator*, const BinaryOperator*>(BO, BA));	
      }
      else if(DoST){
        StartLoc = DoST->getSourceRange().getBegin();
        BinParentST.insert(std::pair<const BinaryOperator*, const DoStmt*>(BO, DoST));	
      }
      else if(WhileST){
        StartLoc = WhileST->getSourceRange().getBegin();
        BinParentST.insert(std::pair<const BinaryOperator*, const WhileStmt*>(BO, WhileST));	
      }
      else if(ForST){
        const Stmt *Body = ForST->getBody();
        StartLoc = ForST->getSourceRange().getBegin();
        BinParentForST.insert(std::pair<const BinaryOperator*, const ForStmt*>(BO, ForST));	
      }
      else if(IfST){
        StartLoc = IfST->getSourceRange().getBegin();
        BinParentST.insert(std::pair<const BinaryOperator*, const IfStmt*>(BO, IfST));	
      }
      else if(ST){
        StartLoc = ST->getSourceRange().getBegin();
      }
      else
      {
        StartLoc = BO->getLHS()->getSourceRange().getBegin();
      }
      return StartLoc;
    }

    std::string handleOperand(SourceLocation StartLoc, const Stmt *ST, const Expr *Op){
      std::string Op1, lhs, rhs;
      SourceManager &SM = Rewrite.getSourceMgr();
      if(const FloatingLiteral *FL_lhs = dyn_cast<FloatingLiteral>(Op)){
        lhs = getTempDest();
        rhs = convertFloatToPosit(FL_lhs);
        ReplaceBOLiteralWithPosit(StartLoc, lhs, " = "+rhs);
        Op1 = lhs;
      }
      else if(const IntegerLiteral *IL_lhs = dyn_cast<IntegerLiteral>(Op)){
        lhs = getTempDest();
        rhs = convertIntToPosit(IL_lhs);
        ReplaceBOLiteralWithPosit(StartLoc, lhs, " = "+rhs);
        Op1 = lhs;
      }
      else if(const UnaryOperator *U_lhs = dyn_cast<UnaryOperator>(Op)){
        unsigned Opcode = U_lhs->getOpcode();
        std::string opName, tmp;
        SourceLocation Loc = U_lhs->getSourceRange().getBegin();
        SourceLocation EndLoc = U_lhs->getSourceRange().getEnd();
        if (Loc.isMacroID()) 
          Loc = SM.getFileLoc(Loc);
        if (EndLoc.isMacroID()) 
          EndLoc = SM.getFileLoc(EndLoc);
        std::string TypeS;
        llvm::raw_string_ostream s(TypeS);
        U_lhs->printPretty(s, 0, PrintingPolicy(LangOptions()));
        if(UOp_Temp.count(U_lhs) != 0){
          Op1 = UOp_Temp.at(U_lhs);
          if(const BinaryOperator *BO = dyn_cast<BinaryOperator>(ST)){
            std::string unary = ReplaceUOWithPosit(U_lhs, StartLoc, lhs, Op1);
            if(unary != "")
              Op1 = unary;
          }
        }
        else if(Opcode == UO_Minus || Opcode == UO_PreDec ||
                    Opcode == UO_PostDec || Opcode == UO_PreInc ||
                    Opcode == UO_PostInc){
          lhs = getTempDest();

          const Type *Ty = U_lhs->getType().getTypePtr();
          QualType QT = Ty->getPointeeType();
          std::string indirect="";
          while (!QT.isNull()) {
            Ty = QT.getTypePtr();
            QT = Ty->getPointeeType();
            indirect += "*";
          }
          Op1 = handleOperand(StartLoc, U_lhs, U_lhs->getSubExpr());
          llvm::errs()<<"OP1:"<<Op1<<"\n";
          rhs = " = "+PositDtoP+"(0)";
          Rewrite.InsertText(StartLoc, 
                PositTY+lhs +rhs+";\n",true, true);		
          std::string unary = ReplaceUOWithPosit(U_lhs, StartLoc, lhs, Op1);
          if(unary != "")
            Op1 = unary;
        }
        else{
          Op1 = s.str();
          if(const BinaryOperator *BO = dyn_cast<BinaryOperator>(ST)){
            std::string unary = ReplaceUOWithPosit(U_lhs, StartLoc, lhs, Op1);
            if(unary != "")
              Op1 = unary;
          }
        }
      }
      else if(const BinaryOperator *BO_lhs = dyn_cast<BinaryOperator>(Op)){
        if(BinOp_Temp.count(BO_lhs) != 0){
          Op1 = BinOp_Temp.at(BO_lhs);
        }
        else{
          Op1 = handleCondition(StartLoc, BO_lhs);
          if(Op1.size()>0){
            std::string temp;
            temp = getTempDest();
            Rewrite.InsertTextAfterToken(StartLoc,
                "bool "+temp +" = "+Op1+";\n");   
            lhs = getTempDest();
            rhs = " = "+PositDtoP+"("+temp+");";
            ReplaceBOLiteralWithPosit(StartLoc, lhs, rhs);
            Op1 = lhs;
          }
          else{
            llvm::raw_string_ostream s(Op1);
            Op->printPretty(s, 0, PrintingPolicy(LangOptions()));
            //Op could be cast => (double *) => replace with => (posit32_t)
            size_t pos = 0;
            pos = s.str().find(FloatingType);
            std::string val = s.str();
            if (pos != std::string::npos){
              s.str().replace(pos, FloatingType.size(), PositTY); 
            }
            lhs = getTempDest();
            rhs = " = "+PositDtoP+"(" + Op1+");";
            ReplaceBOLiteralWithPosit(StartLoc, lhs, rhs);
            Op1 = lhs;
          }
        }
      }
      else if(const ImplicitCastExpr *ASE_lhs = dyn_cast<ImplicitCastExpr>(Op)){
        llvm::raw_string_ostream s(Op1);
        ASE_lhs->printPretty(s, NULL, PrintingPolicy(LangOptions()));
        size_t pos = 0;
        pos = s.str().find(FloatingType);
        std::string val = s.str();
        if (pos != std::string::npos){
          s.str().replace(pos, FloatingType.size(), PositTY); 
        }
        s.flush();

        //handle inttoD
        const Expr *SubExpr = ASE_lhs->getSubExpr();
        const Type *SubTy =  SubExpr->getType().getTypePtr();
        if(SubTy->isIntegerType()){
          lhs = getTempDest();
          rhs = " = "+PositDtoP+"(" + Op1+");";
          ReplaceBOLiteralWithPosit(StartLoc, lhs, rhs);
          Op1 = lhs;
        }
      }
      else if(const ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(Op)){
        std::string TypeS;
        llvm::raw_string_ostream s(TypeS);
        ASE->printPretty(s, 0, PrintingPolicy(LangOptions()));
        Op1 = s.str();
      }
      else if(const DeclRefExpr *DEL = dyn_cast<DeclRefExpr>(Op)){
        if(DEL->getType()->isIntegerType()){
          Op1 = DEL->getDecl()->getName();
          lhs = getTempDest();
          rhs = " = "+PositDtoP+"(" + Op1+");";
          ReplaceBOLiteralWithPosit(StartLoc, lhs, rhs);
          Op1 = lhs;
        }
        else{
          Op1 = DEL->getDecl()->getName();
          bool loadFlag = false;
          if(const BinaryOperator *BO = dyn_cast<BinaryOperator>(ST)){
            if(BO->getOpcode() == BO_Assign){
            }
            else{
              loadFlag = true;
            }
          }else if(const CallExpr *CE = dyn_cast<CallExpr>(ST)){
            loadFlag = true;
          }else if(const ReturnStmt *RT = dyn_cast<ReturnStmt>(ST)){
            loadFlag = true;
          }
        }
      }
      else if(const CStyleCastExpr *CCE = dyn_cast<CStyleCastExpr>(Op)){
        SmallVector<const CStyleCastExpr*, 8>::iterator it;

        ProcessedCCast.push_back(CCE);

        std::string subExpr;
        llvm::raw_string_ostream stream(subExpr);
        CCE->getSubExpr()->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
        stream.flush();
        std::string temp = getTempDest();
        std::string rhs = " = "+PositDtoP+"(" + subExpr+");";
        ReplaceBOLiteralWithPosit(StartLoc, temp, rhs);
        Op1 = temp;
      }
      else if(const CallExpr *CE = dyn_cast<CallExpr>(Op)){
        Op1 = handleCallArgs(StartLoc, CE);
      }
      else if (const ConditionalOperator *CO = dyn_cast<ConditionalOperator>(Op)) {
        std::string tmp = handleCCondition(StartLoc, CO, dyn_cast<BinaryOperator>(CO->getCond()));
        lhs = getTempDest();
        rhs = " = "+ tmp;
        ReplaceBOLiteralWithPosit(StartLoc, lhs, rhs);
        Op1 = lhs;
      }
      else if(ProcessedExpr.count(Op) != 0){
        return ProcessedExpr.at(Op);
      }
      else{
        llvm::raw_string_ostream s(Op1);
        Op->printPretty(s, 0, PrintingPolicy(LangOptions()));
        //Op could be cast => (double *) => replace with => (posit32_t)
        size_t pos = 0;
        pos = s.str().find(FloatingType);
        std::string val = s.str();
        if (pos != std::string::npos){
          s.str().replace(pos, FloatingType.size(), PositTY); 
        }
        s.flush();
      }
      ProcessedExpr.insert(std::pair<const Expr*, std::string>(Op, Op1));	
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
        BinDoWhileLoc.insert(std::pair<const DoStmt*, SourceLocation>(S, S->getSourceRange().getBegin()));
      } else if (auto S = Result.Nodes.getNodeAs<WhileStmt>("while")) {
        SourceLocation StartLoc = findRParenLoc(S, SM, Context);
        if (StartLoc.isInvalid())
          return;
        checkStmt(Result, S->getBody(), StartLoc);	
      } else if (auto S = Result.Nodes.getNodeAs<IfStmt>("if")) {
        SourceLocation StartLoc = findRParenLoc(S, SM, Context);
        if (StartLoc.isInvalid())
          return;
        if (ForceBracesStmts.erase(S))
          ForceBracesStmts.insert(S->getThen());
        bool BracedIf = checkStmtWithLoc(Result, S->getThen(), StartLoc, S->getElseLoc());
        const Stmt *Else = S->getElse();
        if (Else && BracedIf){
          ForceBracesStmts.insert(Else);
        }
        if (Else && !isa<IfStmt>(Else)) {
          // Omit 'else if' statements here, they will be handled directly.	
          checkStmt(Result, Else, S->getElseLoc());
        }
        if (Else && isa<IfStmt>(Else)){ 
          const IfStmt *child = dyn_cast<IfStmt>(Else);
          BinIfLoc.insert(std::pair<const IfStmt*, SourceLocation>(child, S->getSourceRange().getBegin()));
        }
        BinIfLoc.insert(std::pair<const IfStmt*, SourceLocation>(S, S->getSourceRange().getBegin()));
      }
      if (auto S = Result.Nodes.getNodeAs<IfStmt>("binif")) {
        auto BO = Result.Nodes.getNodeAs<BinaryOperator>("binop");
        SourceLocation StartLoc ;
          if(BinIfLoc.count(S) != 0){
            StartLoc = BinIfLoc.at(S);
          }
        BinLoc_Temp.insert(std::pair<const BinaryOperator*, SourceLocation>(BO, StartLoc));	


        if(ProcessedExpr.count(BO) != 0){
          return;
        }
        ProcessedExpr.insert(std::pair<const Expr*, std::string>(BO, ""));	
        BOStack.push(BO);
      }
      if (auto S = Result.Nodes.getNodeAs<DoStmt>("binwhile")) {
        auto BO = Result.Nodes.getNodeAs<BinaryOperator>("binop");
        const Stmt *Body = S->getBody();
        SourceLocation StartLoc = Body->getEndLoc();
        BinLoc_Temp.insert(std::pair<const BinaryOperator*, SourceLocation>(BO, StartLoc));	


        if(ProcessedExpr.count(BO) != 0){
          return;
        }
        ProcessedExpr.insert(std::pair<const Expr*, std::string>(BO, ""));	
        BOStack.push(BO);
      }
      if (auto BinOp = Result.Nodes.getNodeAs<BinaryOperator>("docond")) {
        auto Cond = Result.Nodes.getNodeAs<Expr>("ifstmtcond");
        if (auto S = Result.Nodes.getNodeAs<DoStmt>("dostmt")) {
          const Stmt *Body = S->getBody();
          SourceLocation StartLoc = Body->getEndLoc();
          std::string op  = handleCondition(StartLoc, BinOp);
          if(op != ""){
            Rewrite.ReplaceText(BinOp->getSourceRange(), op);
          }
          ProcessedExpr.insert(std::pair<const Expr*, std::string>(Cond, ""));	
        }
      }
      if (auto S = Result.Nodes.getNodeAs<WhileStmt>("whilecond")) {
        auto Cond = Result.Nodes.getNodeAs<Expr>("ifstmtcond");
        if (auto BinOp = Result.Nodes.getNodeAs<BinaryOperator>("cond")) {
          SourceLocation StartLoc = S->getSourceRange().getBegin();
          std::string op = handleCondition(StartLoc, BinOp);
          if(op != ""){
            Rewrite.ReplaceText(BinOp->getSourceRange(), op);
          }
          ProcessedExpr.insert(std::pair<const Expr*, std::string>(Cond, ""));	
        }
      }
      if (auto S = Result.Nodes.getNodeAs<ConditionalOperator>("c_cond")) {
        if (auto BinOp = Result.Nodes.getNodeAs<BinaryOperator>("cond")) {
          SourceLocation StartLoc = getParentLoc(Result, nullptr);
          Expr *Op1 = S->getTrueExpr();
          Expr *Op2 = S->getFalseExpr();
          std::string Op1Str = handleOperand(StartLoc, BinOp, Op1);
          std::string Op2Str = handleOperand(StartLoc, BinOp, Op2);
          std::string op = handleCondition(StartLoc, BinOp);
          if(op != ""){
            Rewrite.ReplaceText(BinOp->getSourceRange(), op);
            Rewrite.ReplaceText(Op1->getSourceRange(), Op1Str);
            Rewrite.ReplaceText(Op2->getSourceRange(), Op2Str);
          }
          ProcessedExpr.insert(std::pair<const Expr*, std::string>(BinOp, ""));	
        }
      }
      if (auto BinOp = Result.Nodes.getNodeAs<BinaryOperator>("ifcond")) {

        if (auto S = Result.Nodes.getNodeAs<BinaryOperator>("bineq")) {
          std::string op = handleCondition(S->getSourceRange().getBegin(), BinOp);
          if(op != ""){
            Rewrite.ReplaceText(BinOp->getSourceRange(), op);
          }
          ProcessedExpr.insert(std::pair<const Expr*, std::string>(BinOp, ""));	
        }
        if (auto S = Result.Nodes.getNodeAs<VarDecl>("vardecl")) {
          handleCondition(S->getSourceRange().getBegin(), BinOp);
        }
        if (auto S = Result.Nodes.getNodeAs<ForStmt>("forstmt")) {
          handleCondition(S->getSourceRange().getBegin(), BinOp);
        }
        if (auto S = Result.Nodes.getNodeAs<ReturnStmt>("rtstmt")) {
          handleCondition(S->getSourceRange().getBegin(), BinOp);
        }
        if (auto S = Result.Nodes.getNodeAs<IfStmt>("ifstmt")) {
          if(BinIfLoc.count(S) != 0){
            IfStartLoc = BinIfLoc.at(S);
          }
          auto Cond = Result.Nodes.getNodeAs<Expr>("ifstmtcond");
          std::string op = handleCondition(IfStartLoc, BinOp);
          if(op != ""){
            Rewrite.ReplaceText(BinOp->getSourceRange(), op);
          }
          ProcessedExpr.insert(std::pair<const Expr*, std::string>(Cond, ""));	
        }
      }
      if (auto S = Result.Nodes.getNodeAs<IfStmt>("curif")) {
      }
      if (const IntegerLiteral *IL = Result.Nodes.getNodeAs<clang::IntegerLiteral>("initintegerliteral")){
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
          Rewrite.ReplaceText(IL->getSourceRange().getBegin(), Offset, temp);
        }
      }
      if(const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("callfuncnoreturn")){
        const CompoundStmt *CS = Result.Nodes.getNodeAs<clang::CompoundStmt>("call_stmt");

        if(ProcessedExpr.count(CE) != 0){
          return ;
        }

        std::string positLiteral = handleCallArgs(CE->getSourceRange().getBegin(), CE);
        removeLineVD(CE->getSourceRange().getBegin());
        ProcessedExpr.insert(std::pair<const Expr*, std::string>(CE, positLiteral));
      }
      if(const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("callfunc3")){
        const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("call_binop");
        const CompoundStmt *CS = Result.Nodes.getNodeAs<clang::CompoundStmt>("call_stmt");
        const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vd");
        SourceLocation StartLoc, EndLoc;

        if(ProcessedExpr.count(CE) != 0){
          return ;
        }


        if(BO){
          StartLoc = BO->getLHS()->getSourceRange().getBegin();
          EndLoc = BO->getLHS()->getSourceRange().getEnd();
        }
        if(VD){
          StartLoc = VD->getSourceRange().getBegin();
          EndLoc = VD->getSourceRange().getEnd();
        }
        else{
          StartLoc = CE->getSourceRange().getBegin();
          EndLoc = CE->getSourceRange().getEnd();
        }
        std::string positLiteral = handleCallArgs(StartLoc, CE);
        Rewrite.ReplaceText(SourceRange(CE->getSourceRange().getBegin(), 
                  CE->getSourceRange().getEnd()), positLiteral);
        ProcessedExpr.insert(std::pair<const Expr*, std::string>(CE, positLiteral));
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

        ProcessedCE.push_back(CE);

        for(int i=0, j=CE->getNumArgs(); i<j; i++){
          if(isPointerToFloatingType(CE->getArg(i)->getType().getTypePtr())){
            VName = VD->getNameAsString();
            StartLoc = VD->getSourceRange().getBegin();
            EndLoc = VD->getSourceRange().getEnd();
            std::string temp;
            temp = getTempDest();
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
            Rewrite.ReplaceText(SourceRange(CE->getArg(i)->getSourceRange().getBegin(), 
                  CE->getArg(i)->getSourceRange().getEnd()), temp);
          }
        }
      }
      if (const FloatingLiteral *FL = Result.Nodes.getNodeAs<clang::FloatingLiteral>("finit")){
      }
      if (const FloatingLiteral *FL = Result.Nodes.getNodeAs<clang::FloatingLiteral>("initfloatliteral_vd")){
        const InitListExpr *ILE = Result.Nodes.getNodeAs<clang::InitListExpr>("init");
        const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("init_literal");
        const IntegerLiteral *IL = Result.Nodes.getNodeAs<clang::IntegerLiteral>("intliteral");
        const UnaryOperator *U_lhs = Result.Nodes.getNodeAs<clang::UnaryOperator>("unaryOp");

        //initListExpr is visited twice, need to keep a set of visited nodes
        SmallVector<const FloatingLiteral*, 8>::iterator it;
        it = std::find(ProcessedFL.begin(), ProcessedFL.end(), FL);		
        if(it != ProcessedFL.end())
          return;
        SourceLocation StartLoc;
        ProcessedFL.push_back(FL);
        if(ILE){

          std::string positLiteral;
          std::string temp;
          temp = getTempDest();
          if(U_lhs != NULL){
/*
      	    unsigned Opcode = U_lhs->getOpcode();
	    if(Opcode == UO_Minus){
		    std::string subExpr;
		    llvm::raw_string_ostream stream(temp);
		    U_lhs->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
		    stream.flush();
	    }
	    else*/{
            	StartLoc = U_lhs->getSourceRange().getBegin();
            	temp = handleOperand(VD->getSourceRange().getBegin(), ILE, U_lhs);
	    }
          } 
          else{
            StartLoc = FL->getSourceRange().getBegin();
            positLiteral = " = "+convertFloatToPosit(FL);

            Rewrite.InsertText(VD->getSourceRange().getBegin(), 
              PositTY+temp+ positLiteral+"\n", true, true);
          }
          SourceManager &SM = Rewrite.getSourceMgr();
          const char *Buf = SM.getCharacterData(StartLoc);
          int Offset = 0;
          while (*Buf != ';') {
            if(*Buf == ',')
              break;
            if(*Buf == '}')
              break;
            Buf++;
            Offset++;
          }
          Rewrite.ReplaceText(StartLoc, Offset, temp);

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
        //initListExpr is visited twice, need to keep a set of visited nodes
        SmallVector<const FloatingLiteral*, 8>::iterator it;
        it = std::find(ProcessedFL.begin(), ProcessedFL.end(), FL);		
        if(it != ProcessedFL.end())
          return;

        ProcessedFL.push_back(FL);
        const InitListExpr *ILE = Result.Nodes.getNodeAs<clang::InitListExpr>("init");
        const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("init_literal");

        const Type *Ty = VD->getType().getTypePtr();
        const ArrayType *ArrayTy = dyn_cast<ArrayType>(Ty);
        SmallVector<const VarDecl*, 8>::iterator it1;
        TypeLoc VarTypeLoc = VD->getTypeSourceInfo()->getTypeLoc();

        TypeLoc NextTL = VarTypeLoc.getNextTypeLoc();
        while (!NextTL.isNull()) {
          VarTypeLoc = NextTL;
          NextTL = NextTL.getNextTypeLoc();
        }
        it1 = std::find(ProcessedInitVD.begin(), ProcessedInitVD.end(), VD);		
        if(it1 == ProcessedInitVD.end()){
          ProcessedInitVD.push_back(VD);
          //set dimension and size for each dimension
          setDimAndSize(ArrayTy);
        }

        //initialize to 0
        std::string arrDim;
        for(int i = 0; i<Dim;i++){ 
          arrDim += "["+std::to_string(countD[i])+"]";
        }
        if(Dim > 1){
          for(int i = Dim-1; i>0;i--){ 
            if(countD[i] < arrSize[i]-1){
              countD[i]++;
            }
            else{
              countD[i] = 0;
              countD[i-1]++;
            }
          }
        }
        else{
          countD[0]++;
        }

        SmallVector<const InitListExpr*, 8>::iterator ite;
        ite = std::find(ProcessedILE.begin(), ProcessedILE.end(), ILE);
        if(ite == ProcessedILE.end()){
          std::string arrayDim = getArrayDim(VD);
          Rewrite.InsertText(VD->getSourceRange().getBegin(), 
              PositTY+" "+VD->getNameAsString()+arrayDim+";\n");
          Rewrite.InsertText(VD->getSourceRange().getBegin(), 
              "__attribute__ ((constructor)) void __init_"+VD->getNameAsString()+"(void){\n", true, true);
        }

        std::string temp;
        temp = getTempDest();
        std::string positLiteral = PositTY+ temp + " = "+convertFloatToPosit(FL)+"\n";
        Rewrite.InsertText(VD->getSourceRange().getBegin(), positLiteral, true, true);
        std::string positLiteral1 = VD->getNameAsString()+arrDim + " = "+temp+";\n";
        Rewrite.InsertText(VD->getSourceRange().getBegin(), positLiteral1, true, true);

        SourceLocation VDLoc = VD->getSourceRange().getBegin();

        const char *BufVD1 = SM.getCharacterData(VDLoc);
        int VDOffset = 0;
        while (*BufVD1 != ';') {
          if(*BufVD1 == ' ')
            break;
          if(*BufVD1 == '*')	
            break;
          BufVD1++;
          VDOffset++;
        }

        if(ite == ProcessedILE.end()){
          Rewrite.InsertTextAfterToken(VD->getSourceRange().getEnd().getLocWithOffset(1), 
              "\n}\n");

          ProcessedILE.push_back(ILE);
          removeLineVD(VD->getSourceRange().getBegin());
        }
      }
      if (const UnaryExprOrTypeTraitExpr *UE = Result.Nodes.getNodeAs<clang::UnaryExprOrTypeTraitExpr>("unarysizeof")){
        QualType QT = UE->getTypeOfArgument();
        std::string TypeStr = QT.getAsString();
        SourceManager &SM = Rewrite.getSourceMgr();
        SourceLocation StartLoc = UE->getSourceRange().getBegin();
        if (StartLoc.isMacroID()) {
          StartLoc = SM.getFileLoc(StartLoc);
        }
        if(TypeStr.find(FloatingType) == 0 || TypeStr.find(FloatingTypeF) == 0){
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
            if(*Buf == ')')
              break; 
            Offset++;
          }

          Rewrite.ReplaceText(UE->getSourceRange().getBegin().getLocWithOffset(StartOffset), Offset, PositTY);
        }
      }
      if (const FieldDecl *FD = Result.Nodes.getNodeAs<clang::FieldDecl>("struct")){
        ReplaceVDWithPosit(FD->getSourceRange().getBegin(), FD->getSourceRange().getEnd(), FD->getNameAsString()+";");
      }
      if(auto ForST = Result.Nodes.getNodeAs<ForStmt>("forloopinit")){
        const DeclStmt *DS = Result.Nodes.getNodeAs<DeclStmt>("forinit");
        const VarDecl *VD = dyn_cast<VarDecl>(DS->getSingleDecl());
        const FloatingLiteral *FL = Result.Nodes.getNodeAs<clang::FloatingLiteral>("floatliteral");
        const IntegerLiteral *IL = Result.Nodes.getNodeAs<clang::IntegerLiteral>("intliteral");
        const ArraySubscriptExpr *ASE = Result.Nodes.getNodeAs<clang::ArraySubscriptExpr>("arrayliteral");
        const DeclRefExpr *DE = Result.Nodes.getNodeAs<clang::DeclRefExpr>("declexpr");
        const UnaryOperator *U_lhs = Result.Nodes.getNodeAs<clang::UnaryOperator>("unaryOp");
        const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("binop");
        const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("callexpr");
        std::string literal = handleVDLiteralFor(ForST->getSourceRange().getBegin(), ForST->getSourceRange().getEnd(),
            VD->getNameAsString(), FL, IL, ASE, DE, U_lhs, BO, CE);
        Rewrite.InsertText(ForST->getSourceRange().getBegin(), literal);
                    
      }
      if(auto ForST = Result.Nodes.getNodeAs<ForStmt>("forloopinc")){
        /*
        const Expr *ExprCond = Result.Nodes.getNodeAs<Expr>("forincr");
        if(const BinaryOperator *BO = dyn_cast<BinaryOperator>(ExprCond)){
          const Expr *LHS = BO->getLHS();
          const Expr *RHS = BO->getRHS();
          std::string lhs = handleOperand(ForST->getSourceRange().getBegin(), LHS, ForST, LHS); 
          std::string rhs = handleOperand(ForST->getSourceRange().getBegin(), RHS, ForST, RHS); 
          Rewrite.InsertText(ForST->getSourceRange().getBegin(), lhs+"="+rhs, true, true);
          llvm::errs()<<"ForST: increment:\n";
          ExprCond->dump();
        }
        */
      }
      if(auto ForST = Result.Nodes.getNodeAs<ForStmt>("forloopcond")){
        const Expr *ExprCond = Result.Nodes.getNodeAs<Expr>("forcond");
        std::string op  = handleCondition(ForST->getSourceRange().getBegin(), dyn_cast<BinaryOperator>(ExprCond));
      }
      if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vd_literal")){
        SourceLocation StartLoc = VD->getSourceRange().getBegin();
        if (StartLoc.isMacroID()) {
          return;
        }
        const FloatingLiteral *FL = Result.Nodes.getNodeAs<clang::FloatingLiteral>("floatliteral");
        const IntegerLiteral *IL = Result.Nodes.getNodeAs<clang::IntegerLiteral>("intliteral");
        const ArraySubscriptExpr *ASE = Result.Nodes.getNodeAs<clang::ArraySubscriptExpr>("arrayliteral");
        const DeclRefExpr *DE = Result.Nodes.getNodeAs<clang::DeclRefExpr>("declexpr");
        const UnaryOperator *U_lhs = Result.Nodes.getNodeAs<clang::UnaryOperator>("unaryOp");
        const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("binop");
        const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("callexpr");

        handleVDLiteral(VD->getSourceRange().getBegin(), VD->getSourceRange().getEnd(), 
            VD->getNameAsString(), FL, IL, ASE, DE, U_lhs, BO, CE);
      }
      if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclnoinit")){
        SourceLocation StartLoc = VD->getSourceRange().getBegin();
        if (StartLoc.isMacroID()) {
          return;
        }
        const Type *Ty = VD->getType().getTypePtr();
        bool isArrayFlag=false;
        if(const DecayedType *DT = dyn_cast<DecayedType>(VD->getType())){
          Ty = DT->getOriginalType().getTypePtr();
          while (Ty->isArrayType()) {
            const ArrayType *AT = dyn_cast<ArrayType>(Ty);
            if(AT){
              Ty = AT->getElementType().getTypePtr();
              isArrayFlag = true;
            }
            else
              break;
          }
        }
        if(!Ty->isFloatingType())
          return;

        std::string arrayDim = "";
        if(isArrayFlag){
          arrayDim = getArrayDim(VD);
        }
        std::string vdName = VD->getNameAsString()+arrayDim;
        const FunctionDecl *FD = Result.Nodes.getNodeAs<clang::FunctionDecl>("funcDecl");
        const ParmVarDecl *PD = dyn_cast<ParmVarDecl>(VD);
        if (PD) {
          if (FD->isThisDeclarationADefinition() &&
              FD->hasBody() &&
              !FD->isDeleted() &&
              !FD->isDefaulted()){
            if(isArrayFlag){
              ReplaceParmVDWithPositOld(VD->getSourceRange().getBegin(), ' ');
            }
            else
              ReplaceParmVDWithPosit(FD, VD->getSourceRange().getBegin(), vdName);
          }
          else{
            ReplaceParmVDWithPositOld(VD->getSourceRange().getBegin(), ' ');
            }
        }
        else{
          ReplaceVDWithPosit(VD->getSourceRange().getBegin(), VD->getSourceRange().getEnd(), vdName+";");
        }
      }
      if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclarray")){
        SourceLocation StartLoc = VD->getSourceRange().getBegin();
        if (StartLoc.isMacroID()) {
          return;
        }
        const Type *Ty = VD->getType().getTypePtr();
        while (Ty->isArrayType()) {                                                             
          const ArrayType *AT = dyn_cast<ArrayType>(Ty);
          Ty = AT->getElementType().getTypePtr();
        }
        if(!Ty->isFloatingType())
          return;

        std::string arrayDim = getArrayDim(VD);
        //Rewrite.ReplaceText(VD->getSourceRange().getBegin(), 6, PositTY);
        ReplaceVDWithPosit(VD->getSourceRange().getBegin(), VD->getSourceRange().getEnd(), VD->getNameAsString()+arrayDim+";\n");
      }

      if (const FunctionDecl *FD = Result.Nodes.getNodeAs<clang::FunctionDecl>("funcdec")){
        //if (!FD->isThisDeclarationADefinition())
         // return;
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
          ReplaceParmVDWithPositOld(FuncStartLoc, '*');
        else
          ReplaceParmVDWithPositOld(FuncStartLoc, ' ');
          
      }
      if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclpointer")){
        if(!isPointerToFloatingType(VD->getType().getTypePtr()))
          return;
        SourceLocation StartLoc = VD->getSourceRange().getBegin();
        if (StartLoc.isMacroID()) {
          return;
        }

        if (StartLoc.isInvalid())
          return;

        std::string indirect="";
        const Type *Ty = VD->getType().getTypePtr();
        QualType QT = Ty->getPointeeType();
        while (!QT.isNull()) {
          Ty = QT.getTypePtr();
          QT = Ty->getPointeeType();
          indirect += "*";
        }
        const ParmVarDecl *PD = dyn_cast<ParmVarDecl>(VD);
        if(PD){
          ReplaceParmVDWithPositOld(VD->getSourceRange().getBegin(), '*');
        }
        else{
          ReplaceVDWithPosit(VD->getSourceRange().getBegin(), VD->getSourceRange().getEnd(), 
                indirect+VD->getNameAsString()+";");
        }
      }
      if(const FunctionDecl *FD = Result.Nodes.getNodeAs<clang::FunctionDecl>("addheader")){
        //	if(SrcManager->getFileID(Loc) != SrcManager->getMainFileID())	
        insertHeader(FD->getSourceRange().getBegin());	

      }
      if(const CStyleCastExpr *CS = Result.Nodes.getNodeAs<clang::CStyleCastExpr>("ccast")){
        const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vdcast");
        const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("binop");
        SmallVector<const CStyleCastExpr*, 8>::iterator it;
        it = std::find(ProcessedCCast.begin(), ProcessedCCast.end(), CS);		
        if(it != ProcessedCCast.end()){
          return;
        }

        ProcessedCCast.push_back(CS);

        //check if it is inside if stmt and if ifstmt is processed
        SmallVector<const IfStmt*, 8>::iterator ifit;
        auto IfST = Result.Nodes.getNodeAs<IfStmt>("ifstmt");
        auto Cond = Result.Nodes.getNodeAs<Expr>("ifstmtcond");
        if(Cond){
          if(ProcessedExpr.count(Cond) != 0){
            //            llvm::errs()<<"handleOperand is processed before....\n";
            //           return;
          }
          ProcessedExpr.insert(std::pair<const Expr*, std::string>(Cond, ""));	
        }
        QualType QT = CS->getTypeAsWritten();
        std::string TypeStr = QT.getAsString();
        if(TypeStr.find(FloatingType) == 0 || TypeStr.find(FloatingTypeF) == 0){
          const char *Buf = SM.getCharacterData(CS->getSourceRange().getBegin());
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
            if(*Buf == ')')
              break; 
            Offset++;
          }

          if(VD){
            std::string subExpr;
            CS->getSubExpr();
            llvm::raw_string_ostream stream(subExpr);
            CS->getSubExpr()->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
            stream.flush();
            std::string temp = getTempDest();
            std::string rhs = " = "+PositDtoP+"(" + subExpr+");";
            std::string positLiteral = PositTY+ temp + rhs+"\n";
            Rewrite.InsertText(VD->getSourceRange().getBegin(), PositTY+VD->getNameAsString()+";\n", true, true);
            Rewrite.InsertText(VD->getSourceRange().getBegin(), positLiteral, true, true);
            std::string positLiteral1 = VD->getNameAsString() +" = "+ temp+";\n";
            Rewrite.InsertText(VD->getSourceRange().getBegin(), positLiteral1, true, true);
            removeLineVD(VD->getSourceRange().getBegin());
          }
          /*
          else if(BO && const DeclRefExpr *DEL = dyn_cast<DeclRefExpr>(BO->getLHS())){
            llvm::errs()<<"Op is decl\n";
            Op1 = DEL->getDecl()->getName();
          }*/
          else{
            Rewrite.ReplaceText(CS->getSourceRange().getBegin().getLocWithOffset(StartOffset), Offset, PositTY);
          }
        }
#if 0
        CS->dump();
        if(ProcessedExpr.count(CS) != 0){
          llvm::errs()<<"handleOperand is processed before....\n";
          return;
        }
        ProcessedExpr.insert(std::pair<const Expr*, std::string>(CS, ""));	

        if(!isPointerToFloatingType(CS->getTypeAsWritten().getTypePtr()))
          return;
        const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("printfarg");
        const BinaryOperator *BA = Result.Nodes.getNodeAs<clang::BinaryOperator>("bobo");
        const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclbo");
        //if(BA)
        //return;

        const Expr *SubExpr = CS->getSubExpr();
        const Type *SubTy =  SubExpr->getType().getTypePtr();
        llvm::errs()<<"ccast ....\n";
        const char *StartBuf = SM.getCharacterData(CS->getSourceRange().getBegin());
        if(CE){
          llvm::errs()<<"CE huray..\n";
          for(int i=0, j=CE->getNumArgs(); i<j; i++){

            if(ProcessedExpr.count(CE->getArg(i)) != 0){
              llvm::errs()<<"handleOperand is processed before....\n";
              return;
            }
            ProcessedExpr.insert(std::pair<const Expr*, std::string>(CE->getArg(i), ""));	
            std::string TypeS;
            llvm::raw_string_ostream s(TypeS);
            CE->getArg(i)->printPretty(s, 0, PrintingPolicy(LangOptions()));

            if(isPointerToFloatingType(CE->getArg(i)->getType().getTypePtr())){
              if(isPointerToFloatingType(CS->getTypeAsWritten().getTypePtr())){
                const char *StartBuf = SM.getCharacterData(CS->getSourceRange().getBegin());
                std::string castTemp = handleCCast(CE->getSourceRange().getBegin(), CS, s.str());
                convertPToD(CE->getSourceRange().getBegin(), 
                    CE->getArg(i)->getSourceRange().getBegin(), castTemp);
              }
            }
          }
        }
        else if(isPointerToFloatingType(CS->getType().getTypePtr())){
          llvm::errs()<<"huray..1\n";
          if(CS->getType()->isPointerType()){
            llvm::errs()<<"huray..\n";
            const Type *Ty =CS->getType().getTypePtr();
            QualType QT = Ty->getPointeeType();
            std::string indirect="";
            while (!QT.isNull()) {
              Ty = QT.getTypePtr();
              QT = Ty->getPointeeType();
              indirect += "*";
            }
            SourceManager &SM = Rewrite.getSourceMgr();
            const char *StartBuf = SM.getCharacterData(CS->getSourceRange().getBegin().getLocWithOffset(1));
            int Offset = getOffsetUntil(StartBuf, ')');
            Rewrite.ReplaceText(SourceRange(CS->getSourceRange().getBegin().getLocWithOffset(1), 
                  CS->getSourceRange().getBegin().getLocWithOffset(Offset)),
                PositTY+indirect);					
          }
          else{
            llvm::errs()<<"huray..2\n";
            SourceLocation StartLoc = VD->getSourceRange().getBegin();
            std::string lhs, rhs, Op; 
            const Expr *SubExpr = CS->getSubExpr();
            llvm::raw_string_ostream stream(Op);
            SubExpr->printPretty(stream, NULL, PrintingPolicy(LangOptions()));
            stream.flush();
            const Type *SubTy =  SubExpr->getType().getTypePtr();
            if(SubTy->isIntegerType()){
              lhs = getTempDest();
              rhs = " = "+PositDtoP+"(" + Op+");";
              ReplaceBOLiteralWithPosit(StartLoc, lhs, rhs);
              Rewrite.ReplaceText(SourceRange(CS->getSourceRange().getBegin(), 
                    CS->getSourceRange().getEnd()), lhs);
              //              ReplaceVDWithPosit(VD->getSourceRange().getBegin(), VD->getSourceRange().getEnd(), "");
            }
            else{
              const char *StartBuf = SM.getCharacterData(CS->getSourceRange().getBegin().getLocWithOffset(1));
              int Offset = getOffsetUntil(StartBuf, ')');
              Rewrite.ReplaceText(SourceRange(CS->getSourceRange().getBegin().getLocWithOffset(1), 
                    CS->getSourceRange().getBegin().getLocWithOffset(Offset)),
                  PositTY);					
            }
          }
        }
#endif
      }

      if(const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("isnan")){
        const FunctionDecl *Func = CE->getDirectCallee();
        const std::string oldFuncName = Func->getNameInfo().getAsString();
        Rewrite.ReplaceText(CE->getSourceRange().getBegin(), oldFuncName.size(), "isNaN");
        ProcessedExpr.insert(std::pair<const Expr*, std::string>(CE, "isNaN"));
      }
      if(const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("printfsqrt")){
        SourceManager &SM = Rewrite.getSourceMgr();
        for(int i=0, j=CE->getNumArgs(); i<j; i++)
        {
          if(isPointerToFloatingType(CE->getArg(i)->getType().getTypePtr())){

            if(ProcessedExpr.count(CE->getArg(i)) != 0){
              continue;
            }
            ProcessedExpr.insert(std::pair<const Expr*, std::string>(CE->getArg(i), ""));
            if(isa<FloatingLiteral>(CE->getArg(i))){
              continue;
            }
            
            std::string tmp = handleOperand(CE->getSourceRange().getBegin(), CE, 
                CE->getArg(i)->IgnoreImpCasts()); 

            if(!(isa<clang::BinaryOperator>(CE->getArg(i)))){
              convertPToD(CE->getSourceRange().getBegin(), 
                  CE->getArg(i)->getSourceRange().getBegin(), tmp);
            }
          }
        }
      }
      if(const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("callsqrt")){
        std::string ArgName;
        llvm::raw_string_ostream s(ArgName);
        CE->printPretty(s, NULL, PrintingPolicy(LangOptions()));


        const FunctionDecl *Func = CE->getDirectCallee();
        const std::string oldFuncName = Func->getNameInfo().getAsString();
        std::string funcName = handleMathFunc(oldFuncName);
        Rewrite.ReplaceText(CE->getSourceRange().getBegin(), oldFuncName.size(), funcName);
      }
      if(const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("vardeclparent")){
        const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("op");
        BinParentVD.insert(std::pair<const BinaryOperator*, const VarDecl*>(BO, VD));	
      }
      if(const Stmt *ST = Result.Nodes.getNodeAs<clang::Stmt>("returnparent")){
        const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("op");
        BinParentST.insert(std::pair<const BinaryOperator*, const Stmt*>(BO, ST));	
      }
      if(const CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("callexprparent")){
        const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("op");
        //BinParentCE.insert(std::pair<const BinaryOperator*, const CallExpr*>(BO, CE));	
      }
      if (const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("boassign")){
        if(ProcessedExpr.count(BO) != 0){
          return;
        }
        ProcessedExpr.insert(std::pair<const Expr*, std::string>(BO, ""));	
        SourceLocation StartLoc = getParentLoc(Result, BO);
        BinLoc_Temp.insert(std::pair<const BinaryOperator*, SourceLocation>(BO, StartLoc));	
        BOStack.push(BO);
      }
      if (const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("fadd_be1")){
        SourceLocation StartLoc = getParentLoc(Result, BO);
        BinLoc_Temp.insert(std::pair<const BinaryOperator*, SourceLocation>(BO, StartLoc));	


        if(ProcessedExpr.count(BO) != 0){
          return;
        }
        ProcessedExpr.insert(std::pair<const Expr*, std::string>(BO, ""));	

        BOStack.push(BO);
      }
      if (const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("fadd_unary")){
          std::string ArgName;
          llvm::raw_string_ostream s(ArgName);
          BO->printPretty(s, NULL, PrintingPolicy(LangOptions()));
        SourceLocation StartLoc = getParentLoc(Result, BO);
        BinLoc_Temp.insert(std::pair<const BinaryOperator*, SourceLocation>(BO, StartLoc));	
        if(ProcessedExpr.count(BO) != 0){
          return;
        }
        ProcessedExpr.insert(std::pair<const Expr*, std::string>(BO, ""));	
        BOStack.push(BO);
      }
      if (const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("fadd_comp")){
        SourceLocation StartLoc = getParentLoc(Result, BO);
        BinLoc_Temp.insert(std::pair<const BinaryOperator*, SourceLocation>(BO, StartLoc));	
        if(ProcessedExpr.count(BO) != 0){
          return;
        }
        ProcessedExpr.insert(std::pair<const Expr*, std::string>(BO, ""));	
        BOStack.push(BO);
      }
      if (const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("fadd_be2")){
        //We need to handle deepest node first in AST, but there is no way to traverse AST from down to up.
        //We store all binaryoperator in stack.
        //Handle all binop in stack in handleBinOp
        //But there is a glitch, we need to know the location of parent node of binaryoperator
        //It would be better to store a object of binop and location of its parent node in stack, 
        //for now we are storing it in a seperate map
        //We also need to know source location range of parent node to remove it
        SourceLocation StartLoc = getParentLoc(Result, BO);
        BinLoc_Temp.insert(std::pair<const BinaryOperator*, SourceLocation>(BO, StartLoc));	
        if(ProcessedExpr.count(BO) != 0){
          return;
        }
        ProcessedExpr.insert(std::pair<const Expr*, std::string>(BO, ""));	
        BOStack.push(BO);
      }
      if (const BinaryOperator *BO = Result.Nodes.getNodeAs<clang::BinaryOperator>("fadd_be3")){
        SourceLocation StartLoc = getParentLoc(Result, BO);
        BinLoc_Temp.insert(std::pair<const BinaryOperator*, SourceLocation>(BO, StartLoc));	
        if(ProcessedExpr.count(BO) != 0){
          return;
        }
        ProcessedExpr.insert(std::pair<const Expr*, std::string>(BO, ""));	
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
      Matcher.addMatcher(ifStmt(unless(hasAncestor(ifStmt()))).bind("curif"), &HandlerFloatVarDecl);

      // match ifStmt(hasCondition(forEach(binaryOperator().bind("cond"))))
      //ifStmt(forEachDescendant(binaryOperator(hasOperatorName(">"))))
      const auto condBO = binaryOperator(anyOf(hasOperatorName(">"), hasOperatorName(">="), 
            hasOperatorName("=="), hasOperatorName("!="), hasOperatorName("<="),
            hasOperatorName("<")));
      const auto cond = forEachDescendant(
          condBO.bind("cond"));

      const auto ancestor  = anyOf(hasAncestor(varDecl().bind("vardeclbo")), 
       //   hasAncestor(callExpr(unless(hasAncestor(binaryOperator(hasOperatorName("=")))),
        //      unless(hasAncestor(returnStmt()))).bind("callbo")),
          hasAncestor(returnStmt().bind("returnbo")),
          hasAncestor(binaryOperator(hasOperatorName("=")).bind("bobo")),
          hasAncestor(binaryOperator(hasOperatorName("/=")).bind("cbobo")),
          hasAncestor(binaryOperator(hasOperatorName("*=")).bind("cbobo")),
          hasAncestor(binaryOperator(hasOperatorName("-=")).bind("cbobo")),
          hasAncestor(binaryOperator(hasOperatorName("+=")).bind("cbobo")));

      Matcher4.addMatcher( binaryOperator(anyOf(hasOperatorName(">"), hasOperatorName(">="), 
              hasOperatorName("=="), hasOperatorName("!="), hasOperatorName("<="),
              hasOperatorName("<")), 
            anyOf(hasAncestor(ifStmt(hasCondition(expr().bind("ifstmtcond"))).bind("ifstmt")),
              hasAncestor(returnStmt().bind("rtstmt")),
              hasAncestor(varDecl().bind("vardecl")),
//              hasAncestor(forStmt().bind("forstmt")),
              hasAncestor(binaryOperator(hasOperatorName("=")).bind("bineq"))),
            unless(hasAncestor(conditionalOperator()))
            ).bind("ifcond"), &HandlerFloatVarDecl);

      Matcher4.addMatcher(conditionalOperator(cond, ancestor, 
            unless(hasAncestor(binaryOperator(hasOperatorName("+")))),
            unless(hasAncestor(binaryOperator(hasOperatorName("*")))),
            unless(hasAncestor(binaryOperator(hasOperatorName("/")))),
            unless(hasAncestor(binaryOperator(hasOperatorName("-"))))).
          bind("c_cond"), &HandlerFloatVarDecl);

      Matcher4.addMatcher(whileStmt(cond).
          bind("whilecond"), &HandlerFloatVarDecl);

      Matcher4.addMatcher(forStmt(hasLoopInit(declStmt(
                anyOf(
              hasDescendant((
                  floatLiteral().bind("floatliteral"))), 
              hasDescendant((
                    arraySubscriptExpr().bind("arrayliteral"))), 
              hasDescendant((
                      declRefExpr().bind("declexpr"))), 
              hasDescendant((
                      callExpr().bind("callexpr"))))
                ).bind("forinit"))).
          bind("forloopinit"), &HandlerFloatVarDecl);

      Matcher4.addMatcher(forStmt(hasIncrement(expr().bind("forincr"))).
          bind("forloopinc"), &HandlerFloatVarDecl);

      //Matcher4.addMatcher(forStmt(hasCondition(expr().bind("forcond"))).
       //   bind("forloopcond"), &HandlerFloatVarDecl);
      Matcher4.addMatcher(doStmt(hasCondition(expr().bind("forcond"))).
          bind("forloopcond"), &HandlerFloatVarDecl);

      Matcher4.addMatcher( binaryOperator(anyOf(hasOperatorName(">"), hasOperatorName(">="), 
              hasOperatorName("=="), hasOperatorName("!="), hasOperatorName("<="),
              hasOperatorName("<")), 
            anyOf(hasAncestor(doStmt(hasCondition(expr().bind("ifstmtcond"))).bind("dostmt")),
              hasAncestor(returnStmt().bind("rtstmt")),
              hasAncestor(varDecl().bind("vardecl")),
              hasAncestor(forStmt().bind("forstmt")),
              hasAncestor(binaryOperator(hasOperatorName("=")).bind("bineq"))),
            unless(hasAncestor(conditionalOperator()))
            ).bind("docond"), &HandlerFloatVarDecl);
            
      Matcher4.addMatcher(doStmt(cond).
          bind("docond"), &HandlerFloatVarDecl);

      Matcher.addMatcher(whileStmt(hasCondition(expr().bind("whilecond"))).bind("while"), &HandlerFloatVarDecl);

      Matcher.addMatcher(doStmt().bind("do"), &HandlerFloatVarDecl);
      Matcher.addMatcher(forStmt().bind("for"), &HandlerFloatVarDecl);
      Matcher.addMatcher(cxxForRangeStmt().bind("for-range"), &HandlerFloatVarDecl);

      //matcher for  double x = 3.4, y = 5.6;
      //double sum = z;
      Matcher.addMatcher(
          varDecl(hasType(realFloatingPointType()), 
            unless(hasDescendant(binaryOperator())), 
            unless(hasParent(declStmt(hasParent(forStmt())))), //emit for(float x...; 
            anyOf(hasInitializer(ignoringParenImpCasts(
                  integerLiteral().bind("intliteral"))), 
              hasInitializer(ignoringParenImpCasts(unaryOperator(has(ignoringParenImpCasts(integerLiteral()))).
                  bind("unaryOp"))),
              hasInitializer(ignoringParenImpCasts(unaryOperator(has(ignoringParenImpCasts(floatLiteral()))).
                  bind("unaryOp"))),
              hasInitializer(ignoringParenImpCasts(
                  floatLiteral().bind("floatliteral"))), 
              hasInitializer(ignoringParenImpCasts(
                    arraySubscriptExpr().bind("arrayliteral"))), 
              hasInitializer(ignoringParenImpCasts(
                      declRefExpr().bind("declexpr"))), 
              hasInitializer(ignoringParenImpCasts(
                      callExpr().bind("callexpr"))), 
              hasInitializer(ignoringParenImpCasts(
                      binaryOperator(hasType(isInteger())).bind("binop"))), 
              hasDescendant(binaryOperator(hasOperatorName("="))))).bind("vd_literal"), &HandlerFloatVarDecl);
      /*
      //double x[2] = {2.3, 3.4}
      Matcher.addMatcher(
      floatLiteral(hasAncestor(initListExpr(
      unless(hasAncestor(initListExpr().bind("topinit"))), hasAncestor(varDecl().
      bind("init_literal"))).bind("init"))).
      bind("initfloatliteral"), &HandlerFloatVarDecl);
      */
      //this matchers finds size of the array
      Matcher.addMatcher(
          floatLiteral(hasAncestor(initListExpr(
            unless(hasAncestor(initListExpr().bind("init")))).bind("init")), 
            hasAncestor(varDecl(hasLocalStorage()).bind("init_literal")), hasParent(unaryOperator().bind("unaryOp"))).
              bind("initfloatliteral_vd"), &HandlerFloatVarDecl);

      Matcher.addMatcher(
          floatLiteral(hasAncestor(initListExpr(
            unless(hasAncestor(initListExpr().bind("init")))).bind("init")), 
            hasAncestor(varDecl(hasLocalStorage()).bind("init_literal"))).
              bind("initfloatliteral_vd"), &HandlerFloatVarDecl);

      Matcher.addMatcher(
          floatLiteral(hasAncestor(initListExpr(unless(hasAncestor(initListExpr().bind("init")))).bind("init")), 
            hasAncestor(varDecl(hasGlobalStorage()).
              bind("init_literal"))).
          bind("initfloatliteral"), &HandlerFloatVarDecl);
      //double x;
      //foo(x) 
      Matcher4.addMatcher(
          callExpr(
       //     hasDescendant(declRefExpr(hasType(realFloatingPointType()))),
            unless(hasAncestor(varDecl(hasType(realFloatingPointType())).bind("vd"))),
            hasAncestor(compoundStmt().bind("call_stmt")), 
            unless(callee(functionDecl(hasName("printf")))),
            unless(callee(functionDecl(hasName("fprintf")))),
            unless(hasAncestor(callExpr(callee(functionDecl(anyOf(hasName("printf"), hasName("fprintf"))))))),
            unless(hasAncestor(binaryOperator(hasOperatorName("=")))), 
            unless(hasAncestor(binaryOperator(hasOperatorName("+=")))), 
            unless(hasAncestor(binaryOperator(hasOperatorName("-=")))), 
            unless(hasAncestor(binaryOperator(hasOperatorName("/=")))), 
            unless(hasAncestor(binaryOperator(hasOperatorName("*=")))), 
            unless(hasAncestor(varDecl(hasType(realFloatingPointType())))), 
            unless(hasAncestor(binaryOperator(hasParent(ifStmt())))), 
            unless(hasDescendant(binaryOperator()))).bind("callfuncnoreturn"), &HandlerFloatVarDecl);
      //foo(-2.0)
      Matcher4.addMatcher(
          callExpr(hasDescendant(floatLiteral()),
            unless(hasAncestor(varDecl(hasType(realFloatingPointType())).bind("vd"))),
            hasAncestor(compoundStmt().bind("call_stmt")), 
            unless(callee(functionDecl(hasName("printf")))),
            unless(callee(functionDecl(hasName("fprintf")))),
            unless(hasAncestor(callExpr(callee(functionDecl(anyOf(hasName("printf"), hasName("fprintf"))))))),
            unless(hasAncestor(binaryOperator(hasOperatorName("=")))), 
            unless(hasAncestor(binaryOperator(hasOperatorName("+=")))), 
            unless(hasAncestor(binaryOperator(hasOperatorName("-=")))), 
            unless(hasAncestor(binaryOperator(hasOperatorName("/=")))), 
            unless(hasAncestor(binaryOperator(hasOperatorName("*=")))), 
            unless(hasAncestor(varDecl(hasType(realFloatingPointType())))), 
            unless(hasDescendant(binaryOperator()))).bind("callfuncnoreturn"), &HandlerFloatVarDecl);
      //int x = foo(-2.0)
      Matcher4.addMatcher(
          callExpr(hasDescendant(floatLiteral().bind("callfloatliteral")),
            unless(hasAncestor(varDecl(hasType(realFloatingPointType())).bind("vd"))),
            hasAncestor(varDecl().bind("vd")), 
            hasAncestor(compoundStmt().bind("call_stmt")), 
            unless(callee(functionDecl(hasName("printf")))),
            unless(callee(functionDecl(hasName("fprintf")))),
            unless(hasAncestor(binaryOperator(hasOperatorName("=")))), 
            unless(hasAncestor(varDecl(hasType(realFloatingPointType())))), 
            unless(hasDescendant(binaryOperator()))).bind("callfunc3"), &HandlerFloatVarDecl);
            /*
      //double x = foo(-2.0)
      Matcher4.addMatcher(
          callExpr(hasAncestor(varDecl(hasType(realFloatingPointType())).bind("vd")), 
            hasDescendant(floatLiteral().bind("callfloatliteral")), 
            unless(callee(functionDecl(anyOf(hasName("sqrt"), hasName("sqrtf"), hasName("fabsf"), 
                    hasName("cos"), hasName("acos"),
                                      hasName("sin"), hasName("tan"), hasName("strtod"))))),
            unless(hasDescendant(binaryOperator()))).bind("callfunc1"), &HandlerFloatVarDecl);
            */
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
          varDecl(
            unless( hasInitializer(ignoringParenImpCasts(floatLiteral()))), 		
            hasAncestor(functionDecl().bind("funcDecl")),
            unless(hasType(arrayType())), 
            unless( hasInitializer(ignoringParenImpCasts(integerLiteral()))), 
            unless( hasInitializer(ignoringParenImpCasts(arraySubscriptExpr()))), 
            unless(hasDescendant(callExpr())),
            unless(hasDescendant(declRefExpr())),
            unless(hasDescendant(unaryOperator(has(ignoringParenImpCasts(integerLiteral()))))),
            unless(hasDescendant(unaryOperator(has(ignoringParenImpCasts(floatLiteral()))))),
            unless(hasDescendant(binaryOperator(hasType(realFloatingPointType()))))).
          bind("vardeclnoinit"), &HandlerFloatVarDecl);
    
      const auto FloatPtrType = pointerType(pointee(realFloatingPointType()));
      const auto PointerToFloat = 
        hasType(qualType(hasCanonicalType(pointerType(pointee(realFloatingPointType())))));
      //pointer
      Matcher.addMatcher(
          varDecl(unless(typedefDecl()), hasType(pointerType())). 
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


      //float foo() => posit32_t foo()
      Matcher.addMatcher(
          functionDecl(returns(anyOf(realFloatingPointType(), pointerType()))). 
          bind("funcdec"), &HandlerFloatVarDecl);


      //sizeof(double)
      Matcher.addMatcher(
          unaryExprOrTypeTraitExpr(ofKind(UETT_SizeOf)).
          bind("unarysizeof"), &HandlerFloatVarDecl);

      //add softposit.h
      Matcher.addMatcher(
          functionDecl(functionDecl(hasName("main"))).bind("addheader")
          , &HandlerFloatVarDecl);

      //sqrt => p32_sqrt
      Matcher.addMatcher(
          callExpr(callee(functionDecl(anyOf(hasName("sqrt"), hasName("exp"), hasName("cos"), hasName("acos"), hasName("sqrtf"), 
                  hasName("sin"), hasName("tan"), hasName("strtod"), hasName("pow"), hasName("fabs")))),
            unless(hasAncestor(binaryOperator())),
            unless(hasAncestor(varDecl())),
            unless(hasAncestor(callExpr(callee(functionDecl(anyOf(hasName("printf"), hasName("fprintf")))))))).
          bind("callsqrt"), &HandlerFloatVarDecl);

      Matcher.addMatcher(callExpr(callee(functionDecl(hasName("isnan")))).bind("isnan"), &HandlerFloatVarDecl);
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
          hasRHS(Op1));
      //mass = z = sum = 2.3
      Matcher.addMatcher(
          binaryOperator(hasOperatorName("="), hasType(realFloatingPointType()), 
            unless(hasParent(forStmt())), //avoid assignment in for loop
            hasLHS(anything()),
            hasRHS(anyOf(
                  ignoringParenImpCasts(unaryOperator(hasType(realFloatingPointType())).bind("unaryOp")), 
                  ignoringParenImpCasts(callExpr().bind("callexpr")), 
                  ignoringParenImpCasts(floatLiteral().bind("binfloat")), 
                  ignoringParenImpCasts(integerLiteral().bind("binint")),
                  ignoringParenImpCasts(binaryOperator().bind("binop")),
                  ignoringParenImpCasts(declRefExpr(hasType(isInteger())).bind("declint")),
                  ignoringParenImpCasts(declRefExpr(hasType(realFloatingPointType())).bind("declfloat")),
                  ignoringParenImpCasts(binaryOperator(hasType(isInteger())).bind("binop"))
                  ))).
          bind("boassign"), &HandlerFloatVarDecl);

      const auto BinOp1 = binaryOperator(hasType(realFloatingPointType()),
          unless(hasOperatorName("=")), 
          ancestor
          ).bind("fadd_be2");
      Matcher1.addMatcher(BinOp1, &HandlerFloatVarDecl);

      const auto BinOp11 = doStmt(hasCondition(binaryOperator(
           unless(hasOperatorName(">")), unless(hasOperatorName(">=")),
            unless(hasOperatorName("==")), unless(hasOperatorName("!=")), 
            unless(hasOperatorName("<=")),
            unless(hasOperatorName("<")),
          unless(hasOperatorName("=")))
          )).bind("binwhile");

      Matcher1.addMatcher(binaryOperator(hasAncestor(BinOp11), hasType(realFloatingPointType()), 
           unless(hasOperatorName(">")), unless(hasOperatorName(">=")),
            unless(hasOperatorName("==")), unless(hasOperatorName("!=")), 
            unless(hasOperatorName("<=")),
            unless(hasOperatorName("<"))
            ).bind("binop")
          , &HandlerFloatVarDecl);

      const auto BinOp111 = ifStmt(hasCondition(binaryOperator(
           unless(hasOperatorName(">")), unless(hasOperatorName(">=")),
            unless(hasOperatorName("==")), unless(hasOperatorName("!=")), 
            unless(hasOperatorName("<=")),
            unless(hasOperatorName("<")),
          unless(hasOperatorName("=")))
          )).bind("binif");

      Matcher1.addMatcher(binaryOperator(hasAncestor(BinOp111), hasType(realFloatingPointType()), 
           unless(hasOperatorName(">")), unless(hasOperatorName(">=")),
            unless(hasOperatorName("==")), unless(hasOperatorName("!=")), 
            unless(hasOperatorName("<=")),
            unless(hasOperatorName("<"))
            ).bind("binop")
          , &HandlerFloatVarDecl);
/*
      const auto BinOp22 = binaryOperator(hasType(realFloatingPointType()),
          unless(hasOperatorName("=")), 
          ignoringParenImpCasts(hasAncestor(binaryOperator(hasParent(forStmt().bind("forstmtbo"))))) //avoid assignment in for loop
          ).bind("fadd_be2");
      Matcher1.addMatcher(BinOp22, &HandlerFloatVarDecl);
*/
      const auto BinOp4 = binaryOperator(hasType(realFloatingPointType()),
          unless(hasOperatorName("=")),
          unless(hasOperatorName("*=")),
          unless(hasOperatorName("/=")),
          unless(hasOperatorName("+=")),
          unless(hasOperatorName("-=")),
          anyOf(hasAncestor(varDecl().bind("vardeclbo")), 
            hasAncestor(callExpr(unless(hasAncestor(binaryOperator(hasOperatorName("="))))).bind("callbo")),
            hasAncestor(returnStmt().bind("returnbo")),
            hasAncestor(ifStmt().bind("ifstmtbo")),
            hasAncestor(forStmt().bind("forstmtbo")),
            hasAncestor(doStmt().bind("dostmtbo")),
            hasAncestor(whileStmt().bind("whilebo"))),
          unless(hasAncestor(binaryOperator(hasOperatorName("=")))),
          unless(hasAncestor(binaryOperator(hasOperatorName("/=")))),
          unless(hasAncestor(binaryOperator(hasOperatorName("*=")))),
          unless(hasAncestor(binaryOperator(hasOperatorName("-=")))),
          unless(hasAncestor(binaryOperator(hasOperatorName("+="))))
          ).bind("fadd_be3");
//       Matcher1.addMatcher(BinOp4, &HandlerFloatVarDecl);

      //y = x++;
      //y = x++ + z;
      //y = ++x;
      const auto BinOp2 = binaryOperator(hasType(realFloatingPointType()), hasOperatorName("="),
          hasEitherOperand(ignoringParenImpCasts(
              unaryOperator(unless(anyOf(hasOperatorName("*"), hasOperatorName("-")))))
            )).bind("fadd_unary");

      Matcher1.addMatcher(BinOp2, &HandlerFloatVarDecl);
      //x /= y;
      const auto BinOp3 = binaryOperator(hasType(realFloatingPointType()), anyOf(hasOperatorName("/="), 
            hasOperatorName("+="), hasOperatorName("-="), hasOperatorName("*="))
          ).bind("fadd_comp");

      Matcher1.addMatcher(BinOp3, &HandlerFloatVarDecl);

      //printf("%e", x); => t1 = convertP32toDouble; printf("%e", t1);
      Matcher2.addMatcher(
          callExpr(callee(functionDecl(anyOf(hasName("printf"), hasName("fprintf"))))).bind("printfsqrt")
          , &HandlerFloatVarDecl);

      Matcher4.addMatcher(
          binaryOperator(hasOperatorName("="), 
            hasDescendant(cStyleCastExpr(unless(hasAncestor(binaryOperator(anyOf(hasOperatorName("*"),
                          hasOperatorName("/"),
                          hasOperatorName("-"),
                          hasOperatorName("+")))))).bind("ccast")
              )).bind("binop"), &HandlerFloatVarDecl);     

      Matcher4.addMatcher(
          binaryOperator(hasOperatorName("="), 
            hasLHS(hasDescendant(cStyleCastExpr(unless(hasAncestor(binaryOperator(anyOf(hasOperatorName("*"),
                          hasOperatorName("/"),
                          hasOperatorName("-"),
                          hasOperatorName("+")))))).bind("ccast")))).bind("binop"), &HandlerFloatVarDecl);     

      Matcher4.addMatcher(
          binaryOperator(hasOperatorName("="), 
            hasRHS(hasDescendant(cStyleCastExpr(unless(hasAncestor(binaryOperator(anyOf(hasOperatorName("*"),
                          hasOperatorName("/"),
                          hasOperatorName("-"),
                          hasOperatorName("+")))))).bind("ccast")))).bind("binop"), &HandlerFloatVarDecl);     

      Matcher4.addMatcher(
          varDecl(hasDescendant(cStyleCastExpr(unless(hasAncestor(binaryOperator(anyOf(hasOperatorName("*"),
                          hasOperatorName("/"),
                          hasOperatorName("-"),
                          hasOperatorName("+")))))).bind("ccast"))).bind("vdcast"), &HandlerFloatVarDecl);     

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
//        (*b)->dump();
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

      // Now emit the rewritten buffer.
      std::string outName (SM.getFileEntryForID(SM.getMainFileID())->getName());

      size_t ext = outName.rfind(".");
      if (ext == std::string::npos)
        ext = outName.length();
      outName.insert(ext, "_pos");

      std::error_code OutErrorInfo;
      std::error_code ok;
      llvm::raw_fd_ostream outFile(llvm::StringRef(outName), OutErrorInfo, llvm::sys::fs::F_None);

      TheRewriter.getEditBuffer(SM.getMainFileID()).write(outFile);
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
