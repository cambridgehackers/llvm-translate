/* Copyright (c) 2015 The Connectal Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
// Portions of this program were derived from source with the license:
//     This file is distributed under the University of Illinois Open Source
//     License. See LICENSE.TXT for details.
#include <stdio.h>
#include <string.h>
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/InstIterator.h"

using namespace llvm;

#include "declarations.h"

enum {CastOther, CastUnsigned, CastSigned, CastGEP, CastSExt, CastZExt, CastFPToSI};
std::list<StructType *> structWork;
int structWork_run;
static std::map<Type *, int> structMap;
std::map<std::string, void *> nameMap;
static DenseMap<const Value*, unsigned> AnonValueNumbers;
unsigned NextAnonValueNumber;
static DenseMap<StructType*, unsigned> UnnamedStructIDs;
static unsigned NextTypeID;
char *printConstant(Function ***thisp, const char *prefix, Constant *CPV);
static char *writeOperandWithCast(Function ***thisp, Value* Operand, unsigned Opcode);
static char *writeOperandWithCastICmp(Function ***thisp, Value* Operand, bool shouldCast, bool typeIsSigned);
static const char *writeInstructionCast(const Instruction &I);
static const char *printCast(unsigned opc, Type *SrcTy, Type *DstTy);

/*
 * Output instructions
 */
const char *processCInstruction(Function ***thisp, Instruction &I)
{
    char vout[MAX_CHAR_BUFFER];
    int opcode = I.getOpcode();
    vout[0] = 0;
    switch(opcode) {
    // Terminators
    case Instruction::Ret:
        if (I.getNumOperands() != 0 || I.getParent()->getParent()->size() != 1) {
          strcat(vout, "  return ");
          if (I.getNumOperands())
            strcat(vout, writeOperand(thisp, I.getOperand(0), false));
          strcat(vout, ";\n");
        }
        break;
    case Instruction::Unreachable:
        break;

    // Standard binary operators...
    case Instruction::Add: case Instruction::FAdd:
    case Instruction::Sub: case Instruction::FSub:
    case Instruction::Mul: case Instruction::FMul:
    case Instruction::UDiv: case Instruction::SDiv: case Instruction::FDiv:
    case Instruction::URem: case Instruction::SRem: case Instruction::FRem:
    case Instruction::Shl: case Instruction::LShr: case Instruction::AShr:
    // Logical operators...
    case Instruction::And: case Instruction::Or: case Instruction::Xor: {
        assert(!I.getType()->isPointerTy());
        bool needsCast = false;
        if (I.getType() ==  Type::getInt8Ty(I.getContext())
         || I.getType() == Type::getInt16Ty(I.getContext())
         || I.getType() == Type::getFloatTy(I.getContext())) {
          needsCast = true;
          strcat(vout, printType(I.getType(), false, "", "((", ")("));
        }
        if (BinaryOperator::isNeg(&I)) {
          strcat(vout, "-(");
          strcat(vout, writeOperand(thisp, BinaryOperator::getNegArgument(cast<BinaryOperator>(&I)), false));
          strcat(vout, ")");
        } else if (BinaryOperator::isFNeg(&I)) {
          strcat(vout, "-(");
          strcat(vout, writeOperand(thisp, BinaryOperator::getFNegArgument(cast<BinaryOperator>(&I)), false));
          strcat(vout, ")");
        } else if (I.getOpcode() == Instruction::FRem) {
          if (I.getType() == Type::getFloatTy(I.getContext()))
            strcat(vout, "fmodf(");
          else if (I.getType() == Type::getDoubleTy(I.getContext()))
            strcat(vout, "fmod(");
          else  // all 3 flavors of long double
            strcat(vout, "fmodl(");
          strcat(vout, writeOperand(thisp, I.getOperand(0), false));
          strcat(vout, ", ");
          strcat(vout, writeOperand(thisp, I.getOperand(1), false));
          strcat(vout, ")");
        } else {
          strcat(vout, writeOperandWithCast(thisp, I.getOperand(0), I.getOpcode()));
          strcat(vout, " ");
          strcat(vout, intmapLookup(opcodeMap, I.getOpcode()));
          strcat(vout, " ");
          strcat(vout, writeOperandWithCast(thisp, I.getOperand(1), I.getOpcode()));
          strcat(vout, writeInstructionCast(I));
        }
        if (needsCast) {
          strcat(vout, "))");
        }
        }
        break;

    // Memory instructions...
    case Instruction::Store: {
        StoreInst &IS = static_cast<StoreInst&>(I);
        ERRORIF (IS.isVolatile());
        const char *pdest = writeOperand(thisp, IS.getPointerOperand(), true);
        Value *Operand = I.getOperand(0);
        Constant *BitMask = 0;
        IntegerType* ITy = dyn_cast<IntegerType>(Operand->getType());
        if (ITy && !ITy->isPowerOf2ByteWidth())
            BitMask = ConstantInt::get(ITy, ITy->getBitMask());
        const char *sval = writeOperand(thisp, Operand, false);
        if (!strncmp(pdest, "*((0x", 5)) {
            char *endptr = NULL;
            void *pint = (void *)strtol(pdest+5, &endptr, 16);
            const char *pname = mapAddress(pint, "", NULL);
            if (strncmp(pname, "0x", 2) && !strcmp(endptr, "))"))
                pdest = pname;
        }
        strcat(vout, pdest);
        strcat(vout, " = ");
        if (BitMask)
          strcat(vout, "((");
        std::map<std::string, void *>::iterator NI = nameMap.find(sval);
//printf("[%s:%d] storeval %s found %d\n", __FUNCTION__, __LINE__, sval, (NI != nameMap.end()));
        if (NI != nameMap.end() && NI->second) {
            sval = mapAddress(NI->second, "", NULL);
//printf("[%s:%d] second %p pname %s\n", __FUNCTION__, __LINE__, NI->second, sval);
        }
        strcat(vout, sval);
        if (BitMask) {
          strcat(vout, printConstant(thisp, ") & ", BitMask));
          strcat(vout, ")");
        }
        }
        break;

    // Convert instructions...
    case Instruction::SExt:
    case Instruction::FPTrunc: case Instruction::FPExt:
    case Instruction::FPToUI: case Instruction::FPToSI:
    case Instruction::UIToFP: case Instruction::SIToFP:
    case Instruction::IntToPtr: case Instruction::PtrToInt:
    case Instruction::AddrSpaceCast:
    case Instruction::Trunc: case Instruction::ZExt: case Instruction::BitCast: {
        Type *DstTy = I.getType();
        Type *SrcTy = I.getOperand(0)->getType();
        const char *p = getOperand(thisp, I.getOperand(0), false);
        char ptemp[1000];
        //printf("[%s:%d] RRRRRR %s %s thisp %p\n", __FUNCTION__, __LINE__, I.getOpcodeName(), p, thisp);
        if (!strcmp(p, "Vthis") && thisp) {
            sprintf(ptemp, "0x%lx", (unsigned long)thisp);
            p = ptemp;
        }
        if (p[0] == '&') {
            void *tval = mapLookup(p+1);
            if (tval) {
                strcat(vout, p);
                break;
            }
        }
        if (!strncmp(p, "0x", 2)) {
            strcat(vout, p);
            break;
        }
        strcat(vout, "(");
        strcat(vout, printCast(opcode, SrcTy, DstTy));
        if (SrcTy == Type::getInt1Ty(I.getContext()) && opcode == Instruction::SExt)
          strcat(vout, "0-");
        strcat(vout, p);
        if (DstTy == Type::getInt1Ty(I.getContext()) &&
            (opcode == Instruction::Trunc || opcode == Instruction::FPToUI ||
             opcode == Instruction::FPToSI || opcode == Instruction::PtrToInt)) {
          strcat(vout, "&1u");
        }
        strcat(vout, ")");
        }
        break;

    // Other instructions...
    case Instruction::ICmp: case Instruction::FCmp: {
        ICmpInst &CI = static_cast<ICmpInst&>(I);
        bool shouldCast = CI.isRelational();
        bool typeIsSigned = CI.isSigned();
        strcat(vout, writeOperandWithCastICmp(thisp, I.getOperand(0), shouldCast, typeIsSigned));
        strcat(vout, " ");
        strcat(vout, intmapLookup(predText, CI.getPredicate()));
        strcat(vout, " ");
        strcat(vout, writeOperandWithCastICmp(thisp, I.getOperand(1), shouldCast, typeIsSigned));
        strcat(vout, writeInstructionCast(I));
        if (shouldCast)
          strcat(vout, "))");
        }
        break;
    case Instruction::Call: {
        CallInst &ICL = static_cast<CallInst&>(I);
        unsigned ArgNo = 0;
        const char *sep = "";
        Function *func = ICL.getCalledFunction();
        ERRORIF(func && (Intrinsic::ID)func->getIntrinsicID());
        ERRORIF (ICL.hasStructRetAttr() || ICL.hasByValArgument() || ICL.isTailCall());
        Value *Callee = ICL.getCalledValue();
        ConstantExpr *CE = dyn_cast<ConstantExpr>(Callee);
        Function *RF = NULL;
        CallSite CS(&I);
        CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();
        const char *cthisp = getOperand(thisp, *AI, false);
        Function ***called_thisp = NULL;
        if (!strncmp(cthisp, "0x", 2))
            called_thisp = (Function ***)mapLookup(cthisp);
        if (CE && CE->isCast() && (RF = dyn_cast<Function>(CE->getOperand(0)))) {
            Callee = RF;
            strcat(vout, printType(ICL.getCalledValue()->getType(), false, "", "((", ")(void*)"));
        }
        const char *p = writeOperand(thisp, Callee, false);
        //strcat(vout, writeOperand(thisp, Callee, false));
printf("[%s:%d] p %s func %p thisp %p called_thisp %p\n", __FUNCTION__, __LINE__, p, func, thisp, called_thisp);
if (!strncmp(p, "&0x", 3) && !func) {
void *tval = mapLookup(p+1);
if (tval) {
func = static_cast<Function *>(tval);
if (func) {
    p = func->getName().str().c_str();
}
printf("[%s:%d] tval %p pnew %s\n", __FUNCTION__, __LINE__, tval, p);
}
}
        strcat(vout, p);
        if (RF)
            strcat(vout, ")");
        PointerType  *PTy = (func) ? cast<PointerType>(func->getType()) : cast<PointerType>(Callee->getType());
        FunctionType *FTy = cast<FunctionType>(PTy->getElementType());
        ERRORIF(FTy->isVarArg() && !FTy->getNumParams());
        unsigned len = FTy->getNumParams();
        strcat(vout, "(");
        for (; AI != AE; ++AI, ++ArgNo) {
            strcat(vout, sep);
            if (ArgNo < len && (*AI)->getType() != FTy->getParamType(ArgNo))
                strcat(vout, printType(FTy->getParamType(ArgNo), /*isSigned=*/false, "", "(", ")"));
            const char *p = writeOperand(thisp, *AI, false);
            strcat(vout, p);
            sep = ", ";
        }
        strcat(vout, ")");
        pushWork(func, called_thisp, 2);
        }
        break;
    default:
        printf("Other opcode %d.=%s\n", opcode, I.getOpcodeName());
        exit(1);
        break;
    }
    return strdup(vout);
}

/******* Util functions ******/
bool isInlinableInst(const Instruction &I)
{
  if (isa<CmpInst>(I))
    return true;
  if (isa<LoadInst>(I))
    return true;
  if (I.getType() == Type::getVoidTy(I.getContext()) || !I.hasOneUse()
      || isa<TerminatorInst>(I) || isa<CallInst>(I) || isa<PHINode>(I)
      //|| isa<LoadInst>(I)
      || isa<VAArgInst>(I) || isa<InsertElementInst>(I)
      || isa<InsertValueInst>(I) || isa<AllocaInst>(I))
    return false;
  if (I.hasOneUse()) {
    const Instruction &User = cast<Instruction>(*I.use_back());
    if (isa<ExtractElementInst>(User) || isa<ShuffleVectorInst>(User))
      return false;
  }
  if ( I.getParent() != cast<Instruction>(I.use_back())->getParent()) {
      printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      exit(1);
  }
  return true;
}
const AllocaInst *isDirectAlloca(const Value *V)
{
  const AllocaInst *AI = dyn_cast<AllocaInst>(V);
  if (!AI || AI->isArrayAllocation()
   || AI->getParent() != &AI->getParent()->getParent()->getEntryBlock())
    return 0;
  return AI;
}
static bool isAddressExposed(const Value *V)
{
  return isa<GlobalVariable>(V) || isDirectAlloca(V);
}
static char *printString(const char *cp, int len)
{
    char cbuffer[10000];
    char temp[100];
    cbuffer[0] = 0;
    if (!cp[len-1])
        len--;
    strcat(cbuffer, "\"");
    bool LastWasHex = false;
    for (unsigned i = 0, e = len; i != e; ++i) {
      unsigned char C = cp[i];
      if (isprint(C) && (!LastWasHex || !isxdigit(C))) {
        LastWasHex = false;
        if (C == '"' || C == '\\')
          strcat(cbuffer, "\\");
        sprintf(temp, "%c", (char)C);
        strcat(cbuffer, temp);
      } else {
        LastWasHex = false;
        switch (C) {
        case '\n': strcat(cbuffer, "\\n"); break;
        case '\t': strcat(cbuffer, "\\t"); break;
        case '\r': strcat(cbuffer, "\\r"); break;
        case '\v': strcat(cbuffer, "\\v"); break;
        case '\a': strcat(cbuffer, "\\a"); break;
        case '\"': strcat(cbuffer, "\\\""); break;
        case '\'': strcat(cbuffer, "\\\'"); break;
        default:
          strcat(cbuffer, "\\x");
          sprintf(temp, "%c", (char)(( C/16  < 10) ? ( C/16 +'0') : ( C/16 -10+'A')));
          strcat(cbuffer, temp);
          sprintf(temp, "%c", (char)(((C&15) < 10) ? ((C&15)+'0') : ((C&15)-10+'A')));
          strcat(cbuffer, temp);
          LastWasHex = true;
          break;
        }
      }
    }
    strcat(cbuffer, "\"");
    return strdup(cbuffer);
}
static int getCastGroup(int op)
{
  switch (op) {
  case Instruction::Add: case Instruction::Sub: case Instruction::Mul:
  case Instruction::LShr: case Instruction::URem: case Instruction::UDiv:
    return CastUnsigned;
    break;
  case Instruction::AShr: case Instruction::SRem: case Instruction::SDiv:
    return CastSigned;
    break;
  case Instruction::GetElementPtr:
    return CastGEP;
  case Instruction::SExt:
    return CastSExt;
  case Instruction::ZExt: case Instruction::Trunc: case Instruction::FPTrunc:
  case Instruction::FPExt: case Instruction::UIToFP: case Instruction::SIToFP:
  case Instruction::FPToUI: case Instruction::PtrToInt:
  case Instruction::IntToPtr: case Instruction::BitCast:
    return CastZExt;
  case Instruction::FPToSI:
    return CastFPToSI;
  default:
    return CastOther;
  }
}
/*
 * Name functions
 */
static std::string CBEMangle(const std::string &S)
{
  std::string Result;
  for (unsigned i = 0, e = S.size(); i != e; ++i)
    if (isalnum(S[i]) || S[i] == '_') {
      Result += S[i];
    } else {
      Result += '_';
      Result += 'A'+(S[i]&15);
      Result += 'A'+((S[i]>>4)&15);
      Result += '_';
    }
  return Result;
}
static const char *fieldName(StructType *STy, uint64_t ind)
{
    static char temp[MAX_CHAR_BUFFER];
    if (!STy->isLiteral()) { // unnamed items
    CLASS_META *classp = lookup_class(STy->getName().str().c_str());
    ERRORIF (!classp);
    if (classp->inherit) {
        DIType Ty(classp->inherit);
        if (!ind--)
            return CBEMangle(Ty.getName().str()).c_str();
    }
    for (std::list<const MDNode *>::iterator MI = classp->memberl.begin(), ME = classp->memberl.end(); MI != ME; MI++) {
        DIType Ty(*MI);
        if (Ty.getTag() == dwarf::DW_TAG_member) {
            if (!ind--)
                return CBEMangle(Ty.getName().str()).c_str();
        }
    }
    }
    sprintf(temp, "field%d", (int)ind);
    return temp;
}
static std::string getStructName(StructType *STy)
{
    std::string name;
    if (!STy->isLiteral() && !STy->getName().empty())
        name = CBEMangle("l_"+STy->getName().str());
    else {
        if (!UnnamedStructIDs[STy])
            UnnamedStructIDs[STy] = NextTypeID++;
        name = "l_unnamed_" + utostr(UnnamedStructIDs[STy]);
    }
    if (!structWork_run)
        structWork.push_back(STy);
    return name;
}
std::string GetValueName(const Value *Operand)
{
  const GlobalAlias *GA = dyn_cast<GlobalAlias>(Operand);
  const Value *V;
  if (GA && (V = GA->resolveAliasedGlobal(false)))
      Operand = V;
  if (const GlobalValue *GV = dyn_cast<GlobalValue>(Operand))
    return CBEMangle(GV->getName().str());
  std::string Name = Operand->getName();
  if (Name.empty()) { // Assign unique names to local temporaries.
    unsigned &No = AnonValueNumbers[Operand];
    if (No == 0)
      No = ++NextAnonValueNumber;
    Name = "tmp__" + utostr(No);
  }
  std::string VarName;
  VarName.reserve(Name.capacity());
  for (std::string::iterator charp = Name.begin(), E = Name.end(); charp != E; ++charp) {
    char ch = *charp;
    if (isalnum(ch) || ch == '_')
      VarName += ch;
    else {
      char buffer[5];
      sprintf(buffer, "_%x_", ch);
      VarName += buffer;
    }
  }
  return "V" + VarName;
}
/*
 * Output types
 */
char *printType(Type *Ty, bool isSigned, std::string NameSoFar, std::string prefix, std::string postfix)
{
  char cbuffer[10000];
  cbuffer[0] = 0;
  const char *sp = (isSigned?"signed":"unsigned");
  const char *sep = "";
  std::string ostr;
  raw_string_ostream typeOutstr(ostr);

  typeOutstr << prefix;
  switch (Ty->getTypeID()) {
  case Type::VoidTyID:
      typeOutstr << "void " << NameSoFar;
      break;
  case Type::IntegerTyID: {
      unsigned NumBits = cast<IntegerType>(Ty)->getBitWidth();
      assert(NumBits <= 128 && "Bit widths > 128 not implemented yet");
      if (NumBits == 1)
          typeOutstr << "bool";
      else if (NumBits <= 8)
          typeOutstr << sp << " char";
      else if (NumBits <= 16)
          typeOutstr << sp << " short";
      else if (NumBits <= 32)
          typeOutstr << sp << " int";
      else if (NumBits <= 64)
          typeOutstr << sp << " long long";
      }
      typeOutstr << " " << NameSoFar;
      break;
  case Type::FunctionTyID: {
      FunctionType *FTy = cast<FunctionType>(Ty);
      std::string tstr;
      raw_string_ostream FunctionInnards(tstr);
      FunctionInnards << " (" << NameSoFar << ") (";
      for (FunctionType::param_iterator I = FTy->param_begin(), E = FTy->param_end(); I != E; ++I) {
          FunctionInnards << printType(*I, /*isSigned=*/false, "", sep, "");
          sep = ", ";
      }
      if (FTy->isVarArg()) {
          if (!FTy->getNumParams())
              FunctionInnards << " int"; //dummy argument for empty vaarg functs
          FunctionInnards << ", ...";
      } else if (!FTy->getNumParams())
          FunctionInnards << "void";
      FunctionInnards << ')';
      typeOutstr << printType(FTy->getReturnType(), /*isSigned=*/false, FunctionInnards.str(), "", "");
      break;
      }
  case Type::StructTyID:
      typeOutstr << "class " << getStructName(cast<StructType>(Ty)) << " " << NameSoFar;
      break;
  case Type::ArrayTyID: {
      ArrayType *ATy = cast<ArrayType>(Ty);
      unsigned len = ATy->getNumElements();
      if (len == 0) len = 1;
      typeOutstr << printType(ATy->getElementType(), false, "", "", "") << NameSoFar << "[" + utostr(len) + "]";
      break;
      }
  case Type::PointerTyID: {
      PointerType *PTy = cast<PointerType>(Ty);
      std::string ptrName = "*" + NameSoFar;
      if (PTy->getElementType()->isArrayTy() || PTy->getElementType()->isVectorTy())
          ptrName = "(" + ptrName + ")";
      typeOutstr << printType(PTy->getElementType(), false, ptrName, "", "");
      break;
      }
  default:
      llvm_unreachable("Unhandled case in getTypeProps!");
  }
  typeOutstr << postfix;
  return strdup(typeOutstr.str().c_str());
}

/*
 * Output expressions
 */
static char *printConstantDataArray(Function ***thisp, ConstantDataArray *CPA)
{
    char cbuffer[10000];
    cbuffer[0] = 0;
  const char *sep = " ";
  if (CPA->isString()) {
    StringRef value = CPA->getAsString();
    strcat(cbuffer, printString(value.str().c_str(), value.str().length()));
  } else {
    strcat(cbuffer, "{");
    for (unsigned i = 0, e = CPA->getNumOperands(); i != e; ++i) {
        strcat(cbuffer, printConstant(thisp, sep, cast<Constant>(CPA->getOperand(i))));
        sep = ", ";
    }
    strcat(cbuffer, " }");
  }
    return strdup(cbuffer);
}
static char *writeOperandWithCastICmp(Function ***thisp, Value* Operand, bool shouldCast, bool typeIsSigned)
{
    char cbuffer[10000];
    cbuffer[0] = 0;
  if (shouldCast) {
      Type* OpTy = Operand->getType();
      ERRORIF (OpTy->isPointerTy());
      strcat(cbuffer, printType(OpTy, typeIsSigned, "", "((", ")"));
  }
  strcat(cbuffer, writeOperand(thisp, Operand, false));
  if (shouldCast)
      strcat(cbuffer, ")");
    return strdup(cbuffer);
}
static char *writeOperandWithCast(Function ***thisp, Value* Operand, unsigned Opcode)
{
  bool castIsSigned = false;
  bool shouldCast = true;
  switch (getCastGroup(Opcode)) {
    case CastUnsigned:
      break;
    case CastSigned: case CastGEP:
      castIsSigned = true;
      break;
    default:
      shouldCast = false;
      break;
  }
  return writeOperandWithCastICmp(thisp, Operand, shouldCast, castIsSigned);
}
static const char *writeInstructionCast(const Instruction &I)
{
  bool typeIsSigned = false;
  switch (getCastGroup(I.getOpcode())) {
  case CastUnsigned:
    break;
  case CastSigned:
    typeIsSigned = true;
    break;
  default:
    return "";
  }
  return printType(I.getOperand(0)->getType(), typeIsSigned, "", "((", ")())");
}
static const char *printConstExprCast(const ConstantExpr* CE)
{
  Type *Ty = CE->getOperand(0)->getType();
  bool TypeIsSigned = false;
  switch (getCastGroup(CE->getOpcode())) {
  case CastUnsigned:
    break;
  case CastSExt:
    Ty = CE->getType();
    /* fall through */
  case CastSigned:
    TypeIsSigned = true;
    break;
  case CastZExt: case CastFPToSI:
    Ty = CE->getType();
    break;
  default: return "";
  }
  if (!Ty->isIntegerTy() || Ty == Type::getInt1Ty(Ty->getContext()))
      TypeIsSigned = false; // not integer, sign doesn't matter
  return printType(Ty, TypeIsSigned, "", "((", ")())");
}
static char *printConstantWithCast(Function ***thisp, Constant* CPV, unsigned Opcode, const char *postfix)
{
    char cbuffer[10000];
    cbuffer[0] = 0;
  bool typeIsSigned = false;
  switch (getCastGroup(Opcode)) {
    case CastUnsigned:
      break;
    case CastSigned:
      typeIsSigned = true;
      break;
    default:
      strcat(cbuffer, printConstant(thisp, "", CPV));
      goto exitlab;
  }
  strcat(cbuffer, printType(CPV->getType(), typeIsSigned, "", "((", ")"));
  strcat(cbuffer, printConstant(thisp, "", CPV));
  strcat(cbuffer, ")");
  strcat(cbuffer, postfix);
exitlab:
    return strdup(cbuffer);
}
static const char *printCast(unsigned opc, Type *SrcTy, Type *DstTy)
{
    char cbuffer[10000];
    cbuffer[0] = 0;
  bool TypeIsSigned = false;
  switch (getCastGroup(opc)) {
  case CastZExt:
      break;
  case CastSExt: case CastFPToSI:
      TypeIsSigned = true;
      break;
  default:
      llvm_unreachable("Invalid cast opcode");
  }
  strcat(cbuffer, printType(DstTy, TypeIsSigned, "", "(", ")"));
  switch (opc) {
  case Instruction::Trunc: case Instruction::BitCast: case Instruction::FPExt:
  case Instruction::FPTrunc: case Instruction::FPToSI: case Instruction::FPToUI:
      break; // These don't need a source cast.
  case Instruction::IntToPtr: case Instruction::PtrToInt:
      strcat(cbuffer, "(unsigned long)");
      break;
  default:
      strcat(cbuffer, printType(SrcTy, TypeIsSigned, "", "(", ")"));
  }
    return strdup(cbuffer);
}
char *printGEPExpression(Function ***thisp, Value *Ptr, gep_type_iterator I, gep_type_iterator E)
{
    char cbuffer[10000];
    cbuffer[0] = 0;
  ConstantInt *CI;
  void *tval = NULL;
  char *p;
  uint64_t Total = executeGEPOperation(I, E);
  if (I == E)
    return getOperand(thisp, Ptr, false);
  VectorType *LastIndexIsVector = 0;
  for (gep_type_iterator TmpI = I; TmpI != E; ++TmpI)
      LastIndexIsVector = dyn_cast<VectorType>(*TmpI);
  strcat(cbuffer, "(");
  if (LastIndexIsVector)
    strcat(cbuffer, printType(PointerType::getUnqual(LastIndexIsVector->getElementType()), false, "", "((", ")("));
  Value *FirstOp = I.getOperand();
  if (!isa<Constant>(FirstOp) || !cast<Constant>(FirstOp)->isNullValue()) {
    p = getOperand(thisp, Ptr, false);
    if (p[0] == '(' && p[strlen(p) - 1] == ')') {
        char *ptemp = strdup(p+1);
        ptemp[strlen(ptemp)-1] = 0;
        if ((tval = mapLookup(ptemp)))
            goto tvallab;
    }
    if ((tval = mapLookup(p)))
        goto tvallab;
    strcat(cbuffer, "&");
    strcat(cbuffer, p);
  } else {
    bool expose = isAddressExposed(Ptr);
    ++I;  // Skip the zero index.
    if (expose && I != E && (*I)->isArrayTy()
      && (CI = dyn_cast<ConstantInt>(I.getOperand()))) {
        uint64_t val = CI->getZExtValue();
        ++I;     // we processed this index
        GlobalVariable *gv = dyn_cast<GlobalVariable>(Ptr);
        if (gv && !gv->getInitializer()->isNullValue()) {
            Constant* CPV = dyn_cast<Constant>(gv->getInitializer());
            if (ConstantArray *CA = dyn_cast<ConstantArray>(CPV)) {
                (void)CA;
                ERRORIF (val != 2);
                val = 0;
            }
            else if (ConstantDataArray *CA = dyn_cast<ConstantDataArray>(CPV)) {
                ERRORIF (val);
                strcat(cbuffer, printConstantDataArray(thisp, CA));
                goto next;
            }
        }
        strcat(cbuffer, getOperand(thisp, Ptr, true));
next:
        if (val) {
            char temp[100];
            sprintf(temp, "+%ld", val);
            strcat(cbuffer, temp);
        }
    }
    else {
        if (expose) {
          strcat(cbuffer, "&");
          strcat(cbuffer, getOperand(thisp, Ptr, true));
        } else if (I != E && (*I)->isStructTy()) {
          const char *p = getOperand(thisp, Ptr, false);
          char *ptemp = strdup(p);
          std::map<std::string, void *>::iterator NI = nameMap.find(p);
          const char *fieldp = fieldName(dyn_cast<StructType>(*I), cast<ConstantInt>(I.getOperand())->getZExtValue());
//printf("[%s:%d] writeop %s found %d\n", __FUNCTION__, __LINE__, p, (NI != nameMap.end()));
if (!strcmp(fieldp, "module") && !strcmp(p, "Vthis")) {
tval = thisp;
printf("[%s:%d] MMMMMMMMMMMMMMMMM %s VthisModule %p\n", __FUNCTION__, __LINE__, p, tval);
goto tvallab;
}
          if (NI != nameMap.end() && NI->second) {
              sprintf(&cbuffer[strlen(cbuffer)], "0x%lx", Total + (long)NI->second);
              goto exitlab;
          }
          if (!strncmp(p, "0x", 2)) {
              tval = mapLookup(p);
              goto tvallab;
          }
          if (!strncmp(p, "(0x", 3) && p[strlen(p) - 1] == ')') {
              ptemp += 1;
              ptemp[strlen(ptemp)-1] = 0;
              p = ptemp;
              tval = mapLookup(p);
              goto tvallab;
          }
          if (!strncmp(p, "(&", 2) && p[strlen(p) - 1] == ')') {
              ptemp += 2;
              ptemp[strlen(ptemp)-1] = 0;
              p = ptemp;
              tval = mapLookup(p);
              if (tval)
                  goto tvallab;
              else {
                  strcat(cbuffer, "&");
                  strcat(cbuffer, p);
                  strcat(cbuffer, ".");
                  strcat(cbuffer, fieldp);
              }
          }
          strcat(cbuffer, "&");
          strcat(cbuffer, p);
          strcat(cbuffer, "->");
          strcat(cbuffer, fieldp);
          ++I;  // eat the struct index as well.
        } else {
          strcat(cbuffer, "&");
          strcat(cbuffer, "(");
          strcat(cbuffer, getOperand(thisp, Ptr, true));
          strcat(cbuffer, ")");
        }
    }
  }
  for (; I != E; ++I) {
    if ((*I)->isStructTy()) {
      StructType *STy = dyn_cast<StructType>(*I);
      strcat(cbuffer, ".");
      strcat(cbuffer, fieldName(STy, cast<ConstantInt>(I.getOperand())->getZExtValue()));
    } else if ((*I)->isArrayTy() || !(*I)->isVectorTy()) {
      strcat(cbuffer, "[");
      strcat(cbuffer, getOperand(thisp, I.getOperand(), false));
      strcat(cbuffer, "]");
    } else {
      if (!isa<Constant>(I.getOperand()) || !cast<Constant>(I.getOperand())->isNullValue()) {
        strcat(cbuffer, ")+(");
        strcat(cbuffer, writeOperandWithCast(thisp, I.getOperand(), Instruction::GetElementPtr));
      }
      strcat(cbuffer, "))");
    }
  }
  goto exitlab;
tvallab:
    sprintf(&cbuffer[strlen(cbuffer)], "0x%lx", Total + (long)tval);
exitlab:
    strcat(cbuffer, ")");
    p = strdup(cbuffer);
    if (!strncmp(p, "(0x", 3)) {
        p++;
        p[strlen(p) - 1] = 0;
    }
#if 0
    printf("[%s:%d] return %s; ", __FUNCTION__, __LINE__, p);
    if (!strncmp(p, "0x", 2)) {
        char *endptr;
        void **pint = (void **)strtol(p+2, &endptr, 16);
        printf(" [%p]= %p", pint, *pint);
    }
    printf("\n");
#endif
    return p;
}
char *printConstant(Function ***thisp, const char *prefix, Constant *CPV)
{
    char cbuffer[10000];
    cbuffer[0] = 0;
  const char *sep = " ";
  /* handle expressions */
  strcat(cbuffer, prefix);
  if (const ConstantExpr *CE = dyn_cast<ConstantExpr>(CPV)) {
    strcat(cbuffer, "(");
    int op = CE->getOpcode();
    switch (op) {
    case Instruction::Trunc: case Instruction::ZExt: case Instruction::SExt:
    case Instruction::FPTrunc: case Instruction::FPExt: case Instruction::UIToFP:
    case Instruction::SIToFP: case Instruction::FPToUI: case Instruction::FPToSI:
    case Instruction::PtrToInt: case Instruction::IntToPtr: case Instruction::BitCast:
      strcat(cbuffer, printCast(op, CE->getOperand(0)->getType(), CE->getType()));
      if (op == Instruction::SExt &&
          CE->getOperand(0)->getType() == Type::getInt1Ty(CPV->getContext()))
        strcat(cbuffer, "0-");
      strcat(cbuffer, printConstant(thisp, "", CE->getOperand(0)));
      if (CE->getType() == Type::getInt1Ty(CPV->getContext()) &&
          (op == Instruction::Trunc || op == Instruction::FPToUI ||
           op == Instruction::FPToSI || op == Instruction::PtrToInt))
        strcat(cbuffer, "&1u");
      break;
    case Instruction::GetElementPtr:
      strcat(cbuffer, printGEPExpression(thisp, CE->getOperand(0), gep_type_begin(CPV), gep_type_end(CPV)));
      break;
    case Instruction::Select:
      strcat(cbuffer, printConstant(thisp, "", CE->getOperand(0)));
      strcat(cbuffer, printConstant(thisp, "?", CE->getOperand(1)));
      strcat(cbuffer, printConstant(thisp, ":", CE->getOperand(2)));
      break;
    case Instruction::Add: case Instruction::FAdd: case Instruction::Sub:
    case Instruction::FSub: case Instruction::Mul: case Instruction::FMul:
    case Instruction::SDiv: case Instruction::UDiv: case Instruction::FDiv:
    case Instruction::URem: case Instruction::SRem: case Instruction::FRem:
    case Instruction::And: case Instruction::Or: case Instruction::Xor:
    case Instruction::ICmp: case Instruction::Shl: case Instruction::LShr:
    case Instruction::AShr:
    {
      strcat(cbuffer, printConstantWithCast(thisp, CE->getOperand(0), op, " "));
      if (op == Instruction::ICmp)
        strcat(cbuffer, intmapLookup(predText, CE->getPredicate()));
      else
        strcat(cbuffer, intmapLookup(opcodeMap, op));
      strcat(cbuffer, " ");
      strcat(cbuffer, printConstantWithCast(thisp, CE->getOperand(1), op, ""));
      strcat(cbuffer, printConstExprCast(CE));
      break;
    }
    case Instruction::FCmp: {
      if (CE->getPredicate() == FCmpInst::FCMP_FALSE)
        strcat(cbuffer, "0");
      else if (CE->getPredicate() == FCmpInst::FCMP_TRUE)
        strcat(cbuffer, "1");
      else {
        strcat(cbuffer, "llvm_fcmp_");
        strcat(cbuffer, intmapLookup(predText, CE->getPredicate()));
        strcat(cbuffer, "(");
        strcat(cbuffer, printConstantWithCast(thisp, CE->getOperand(0), op, ", "));
        strcat(cbuffer, printConstantWithCast(thisp, CE->getOperand(1), op, ")"));
      }
      strcat(cbuffer, printConstExprCast(CE));
      break;
    }
    default:
      outs() << "printConstant Error: Unhandled constant expression: " << *CE << "\n";
      llvm_unreachable(0);
    }
    strcat(cbuffer, ")");
    goto exitlab;
  }
  ERRORIF(isa<UndefValue>(CPV) && CPV->getType()->isSingleValueType()); /* handle 'undefined' */
  /* handle int */
  if (ConstantInt *CI = dyn_cast<ConstantInt>(CPV)) {
    char temp[100];
    Type* Ty = CI->getType();
    if (Ty == Type::getInt1Ty(CPV->getContext()))
      strcat(cbuffer, CI->getZExtValue() ? "1" : "0");
    else if (Ty == Type::getInt32Ty(CPV->getContext()) || Ty->getPrimitiveSizeInBits() > 32) {
      sprintf(temp, "%ld", CI->getZExtValue());
      strcat(cbuffer, temp);
    }
    else {
      strcat(cbuffer, printType(Ty, false, "", "((", ")"));
      if (CI->isMinValue(true))
        sprintf(temp, "%ld", CI->getZExtValue());// << 'u';
      else
        sprintf(temp, "%ld", CI->getSExtValue());
      strcat(cbuffer, temp);
      strcat(cbuffer, ")");
    }
    goto exitlab;
  }
  {
  int tid = CPV->getType()->getTypeID();
  /* handle structured types */
  switch (tid) {
  case Type::ArrayTyID:
    if (ConstantArray *CPA = dyn_cast<ConstantArray>(CPV)) {
      int len = CPA->getNumOperands();
      Type *ETy = CPA->getType()->getElementType();
      if (ETy == Type::getInt8Ty(CPA->getContext()) && len
       && cast<Constant>(*(CPA->op_end()-1))->isNullValue()) {
        char *cp = (char *)malloc(len);
        for (int i = 0; i != len-1; ++i)
            cp[i] = cast<ConstantInt>(CPA->getOperand(i))->getZExtValue();
        strcat(cbuffer, printString(cp, len));
        free(cp);
      } else {
        strcat(cbuffer, "{");
        for (unsigned i = 0, e = len; i != e; ++i) {
            strcat(cbuffer, printConstant(thisp, sep, cast<Constant>(CPA->getOperand(i))));
            sep = ", ";
        }
        strcat(cbuffer, " }");
      }
    }
    else if (ConstantDataArray *CA = dyn_cast<ConstantDataArray>(CPV))
      strcat(cbuffer, printConstantDataArray(thisp, CA));
    else
      ERRORIF(1);
    break;
  case Type::VectorTyID:
    strcat(cbuffer, "{");
    if (ConstantVector *CV = dyn_cast<ConstantVector>(CPV)) {
        for (unsigned i = 0, e = CV->getNumOperands(); i != e; ++i) {
            strcat(cbuffer, printConstant(thisp, sep, cast<Constant>(CV->getOperand(i))));
            sep = ", ";
        }
    }
    else
      ERRORIF(1);
    strcat(cbuffer, " }");
    break;
  case Type::StructTyID:
    strcat(cbuffer, "{");
    ERRORIF(isa<ConstantAggregateZero>(CPV) || isa<UndefValue>(CPV));
    for (unsigned i = 0, e = CPV->getNumOperands(); i != e; ++i) {
        strcat(cbuffer, printConstant(thisp, sep, cast<Constant>(CPV->getOperand(i))));
        sep = ", ";
    }
    strcat(cbuffer, " }");
    break;
  case Type::PointerTyID:
    if (isa<ConstantPointerNull>(CPV))
      strcat(cbuffer, printType(CPV->getType(), false, "", "((", ")/*NULL*/0)")); // sign doesn't matter
    else if (GlobalValue *GV = dyn_cast<GlobalValue>(CPV))
      strcat(cbuffer, writeOperand(thisp, GV, false));
    break;
  default:
    outs() << "Unknown constant type: " << *CPV << "\n";
    llvm_unreachable(0);
  }
  }
exitlab:
    return strdup(cbuffer);
}
char *getOperand(Function ***thisp, Value *Operand, bool Indirect)
{
    char cbuffer[10000];
    cbuffer[0] = 0;
  Instruction *I = dyn_cast<Instruction>(Operand);
  bool isAddressImplicit = isAddressExposed(Operand);
  const char *prefix = "";
  if (Indirect && isAddressImplicit) {
      isAddressImplicit = false;
      Indirect = false;
  }
  if (Indirect)
      prefix = "*";
  if (isAddressImplicit)
      prefix = "(&";  // Global variables are referenced as their addresses by llvm
  if (I && isInlinableInst(*I)) {
      const char *p = processInstruction(thisp, I, 2);
      char *ptemp = strdup(p);
      p = ptemp;
      if (ptemp[0] == '(' && ptemp[strlen(ptemp) - 1] == ')') {
          p++;
          ptemp[strlen(ptemp) - 1] = 0;
      }
      if (!strcmp(prefix, "*") && p[0] == '&') {
          prefix = "";
          p++;
      }
      if (!strcmp(prefix, "*") && !strncmp(p, "0x", 2)) {
          char *endptr;
          void **pint = (void **)strtol(p+2, &endptr, 16);
          sprintf(cbuffer, "0x%lx", (unsigned long)*pint);
      }
      else {
          int addparen = strncmp(p, "0x", 2) && (p[0] != '(' || p[strlen(p)-1] != ')');
          strcat(cbuffer, prefix);
          if (addparen)
              strcat(cbuffer, "(");
          strcat(cbuffer, p);
          if (addparen)
              strcat(cbuffer, ")");
      }
  }
  else {
      Constant* CPV = dyn_cast<Constant>(Operand);
      if (CPV && !isa<GlobalValue>(CPV))
          strcat(cbuffer, printConstant(thisp, prefix, CPV));
      else {
          strcat(cbuffer, prefix);
          strcat(cbuffer, GetValueName(Operand).c_str());
      }
  }
  if (isAddressImplicit)
      strcat(cbuffer, ")");
  return strdup(cbuffer);
}
char *writeOperand(Function ***thisp, Value *Operand, bool Indirect)
{
    char *p = getOperand(thisp, Operand, Indirect);
    void *tval = mapLookup(p);
    if (tval) {
        char temp[1000];
        sprintf(temp, "%s%s", Indirect ? "" : "&", mapAddress(tval, "", NULL));
        return strdup(temp);
    }
    return p;
}
char *printFunctionSignature(const Function *F, bool Prototype, const char *postfix, int skip)
{
  std::string tstr;
  raw_string_ostream FunctionInnards(tstr);
  const char *sep = "";
  const char *statstr = "";
  FunctionType *FT = cast<FunctionType>(F->getFunctionType());
  ERRORIF (F->hasDLLImportLinkage() || F->hasDLLExportLinkage() || F->hasStructRetAttr() || FT->isVarArg());
  if (F->hasLocalLinkage()) statstr = "static ";
  FunctionInnards << GetValueName(F);
  FunctionInnards << '(';
  if (F->isDeclaration()) {
    for (FunctionType::param_iterator I = FT->param_begin(), E = FT->param_end(); I != E; ++I) {
      if (!skip) {
      FunctionInnards << printType(*I, /*isSigned=*/false, "", sep, "");
      sep = ", ";
      }
      skip = 0;
    }
  } else if (!F->arg_empty()) {
    for (Function::const_arg_iterator I = F->arg_begin(), E = F->arg_end(); I != E; ++I) {
      if (!skip) {
      std::string ArgName = (I->hasName() || !Prototype) ? GetValueName(I) : "";
      FunctionInnards << printType(I->getType(), /*isSigned=*/false, ArgName, sep, "");
      sep = ", ";
      }
      skip = 0;
    }
  }
  if (!strcmp(sep, ""))
    FunctionInnards << "void"; // ret() -> ret(void) in C.
  FunctionInnards << ')';
  return printType(F->getReturnType(), /*isSigned=*/false, FunctionInnards.str(), statstr, postfix);
}
/*
 * Pass control functions
 */
static int processVar(const GlobalVariable *GV)
{
  if (GV->isDeclaration() || GV->getSection() == "llvm.metadata"
   || (GV->hasAppendingLinkage() && GV->use_empty()
    && (GV->getName() == "llvm.global_ctors" || GV->getName() == "llvm.global_dtors")))
      return 0;
  return 1;
}
static int checkIfRule(Type *aTy)
{
    Type *Ty;
    FunctionType *FTy;
    PointerType  *PTy;
    if ((PTy = dyn_cast<PointerType>(aTy))
     && PTy && (FTy = dyn_cast<FunctionType>(PTy->getElementType()))
     && FTy && (PTy = dyn_cast<PointerType>(FTy->getParamType(0)))
     && PTy && (Ty = PTy->getElementType())
     && Ty  && (Ty->getNumContainedTypes() > 1)
     && (Ty = dyn_cast<StructType>(Ty->getContainedType(0)))
     && Ty  && (Ty->getStructName() == "class.Rule"))
       return 1;
    return 0;
}
void generateCppData(FILE *OStr, Module &Mod)
{
    ArrayType *ATy;
    const ConstantExpr *CE;
    std::list<std::string> ruleList;
    NextTypeID = 1;
    fprintf(OStr, "\n\n/* Global Variable Definitions and Initialization */\n");
    for (Module::global_iterator I = Mod.global_begin(), E = Mod.global_end(); I != E; ++I) {
        ERRORIF (I->hasWeakLinkage() || I->hasDLLImportLinkage() || I->hasDLLExportLinkage()
          || I->isThreadLocal() || I->hasHiddenVisibility() || I->hasExternalWeakLinkage());
        if (processVar(I)) {
          Type *Ty = I->getType()->getElementType();
          if (!(Ty->getTypeID() == Type::ArrayTyID && (ATy = cast<ArrayType>(Ty))
              && ATy->getElementType()->getTypeID() == Type::PointerTyID)
           && I->getInitializer()->isNullValue()) {
              if (I->hasLocalLinkage())
                fprintf(OStr, "static ");
              fprintf(OStr, "%s", printType(Ty, false, GetValueName(I), "", ""));
              if (!I->getInitializer()->isNullValue())
                fprintf(OStr, " = %s", writeOperand(NULL, I->getInitializer(), false));
              fprintf(OStr, ";\n");
          }
        }
    }
    fprintf(OStr, "\n\n//******************** vtables for Classes *******************\n");
    for (Module::global_iterator I = Mod.global_begin(), E = Mod.global_end(); I != E; ++I)
        if (processVar(I)) {
          Type *Ty = I->getType()->getElementType();
          if (Ty->getTypeID() == Type::ArrayTyID && (ATy = cast<ArrayType>(Ty))
           && ATy->getElementType()->getTypeID() == Type::PointerTyID) {
              if (I->hasLocalLinkage())
                fprintf(OStr, "static ");
              std::string strName = GetValueName(I);
              fprintf(OStr, "%s", printType(Ty, false, strName, "", ""));
              if (!I->getInitializer()->isNullValue()) {
                fprintf(OStr, " = " );
                Constant* CPV = dyn_cast<Constant>(I->getInitializer());
                if (ConstantArray *CA = dyn_cast<ConstantArray>(CPV)) {
                    Type *ETy = CA->getType()->getElementType();
                    ERRORIF (ETy == Type::getInt8Ty(CA->getContext()) || ETy == Type::getInt8Ty(CA->getContext()));
                    fprintf(OStr, "{");
                    const char *sep = " ";
                    if ((CE = dyn_cast<ConstantExpr>(CA->getOperand(3))) && CE->getOpcode() == Instruction::BitCast
                     && checkIfRule(CE->getOperand(0)->getType())) {
                        ruleList.push_back(strName);
                        for (unsigned i = 2, e = CA->getNumOperands(); i != e; ++i) {
                          Constant* V = dyn_cast<Constant>(CA->getOperand(i));
                          fprintf(OStr, "%s", printConstant(NULL, sep, V));
                          sep = ", ";
                        }
                    }
                    else
                        fprintf(OStr, "0");
                    fprintf(OStr, " }");
                }
              }
              fprintf(OStr, ";\n");
          }
    }
    fprintf(OStr, "typedef unsigned char **RuleVTab;//Rules:\nconst RuleVTab ruleList[] = {\n    ");
    while (ruleList.begin() != ruleList.end()) {
        fprintf(OStr, "%s, ", ruleList.begin()->c_str());
        ruleList.pop_front();
    }
    fprintf(OStr, "NULL};\n");
}

static void printContainedStructs(Type *Ty, FILE *OStr)
{
    std::map<Type *, int>::iterator FI = structMap.find(Ty);
    PointerType *PTy = dyn_cast<PointerType>(Ty);
    if (PTy)
            printContainedStructs(PTy->getElementType(), OStr);
    else if (FI == structMap.end() && !Ty->isPrimitiveType() && !Ty->isIntegerTy()) {
        StructType *STy = dyn_cast<StructType>(Ty);
        std::string name;
        structMap[Ty] = 1;
        if (STy) {
            name = getStructName(STy);
            fprintf(OStr, "class %s;\n", name.c_str());
            for (StructType::element_iterator I = STy->element_begin(), E = STy->element_end(); I != E; ++I)
                printContainedStructs(*I, OStr);
        }
        for (Type::subtype_iterator I = Ty->subtype_begin(), E = Ty->subtype_end(); I != E; ++I)
            printContainedStructs(*I, OStr);
        if (STy) {
            fprintf(OStr, "class %s {\npublic:\n", name.c_str());
            unsigned Idx = 0;
            for (StructType::element_iterator I = STy->element_begin(), E = STy->element_end(); I != E; ++I)
              fprintf(OStr, "%s", printType(*I, false, fieldName(STy, Idx++), "  ", ";\n"));
            fprintf(OStr, "};\n\n");
        }
    }
}
void generateCppHeader(Module &Mod, FILE *OStr)
{
    structWork_run = 1;
    while (structWork.begin() != structWork.end()) {
        printContainedStructs(*structWork.begin(), OStr);
        structWork.pop_front();
    }
    fprintf(OStr, "\n/* External Global Variable Declarations */\n");
    for (Module::global_iterator I = Mod.global_begin(), E = Mod.global_end(); I != E; ++I)
        if (I->hasExternalLinkage() || I->hasCommonLinkage())
          fprintf(OStr, "%s", printType(I->getType()->getElementType(), false, GetValueName(I), "extern ", ";\n"));
    fprintf(OStr, "\n/* Function Declarations */\n");
    for (Module::iterator I = Mod.begin(), E = Mod.end(); I != E; ++I) {
        ERRORIF(I->hasExternalWeakLinkage() || I->hasHiddenVisibility() || (I->hasName() && I->getName()[0] == 1));
        std::string name = I->getName().str();
        int skip = 0;
        //if (checkIfRule(I->getType()))
            //skip = 1;
        if (!(I->isIntrinsic() || name == "main" || name == "atexit"
         || name == "printf" || name == "__cxa_pure_virtual"
         || name == "setjmp" || name == "longjmp" || name == "_setjmp"))
            fprintf(OStr, "%s", printFunctionSignature(I, true, ";\n", skip));
    }
    UnnamedStructIDs.clear();
}
