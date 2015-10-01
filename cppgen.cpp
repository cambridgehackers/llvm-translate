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
#include <cxxabi.h> // abi::__cxa_demangle
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/InstIterator.h"

using namespace llvm;

#include "declarations.h"

enum {CastOther, CastUnsigned, CastSigned, CastGEP, CastSExt, CastZExt, CastFPToSI};
std::list<StructType *> structWork;
int structWork_run;
static std::map<Type *, int> structMap;
static DenseMap<const Value*, unsigned> AnonValueNumbers;
unsigned NextAnonValueNumber;
static DenseMap<StructType*, unsigned> UnnamedStructIDs;
static unsigned NextTypeID;
static char *printConstant(const char *prefix, Constant *CPV, bool Static);
static char *writeOperand(Value *Operand, bool Indirect, bool Static = false);

/******* Util functions ******/
bool isInlinableInst(const Instruction &I)
{
  if (isa<CmpInst>(I))
    return true;
  if (I.getType() == Type::getVoidTy(I.getContext()) || !I.hasOneUse()
      || isa<TerminatorInst>(I) || isa<CallInst>(I) || isa<PHINode>(I)
      || isa<LoadInst>(I) || isa<VAArgInst>(I) || isa<InsertElementInst>(I)
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
      typeOutstr << "struct " << getStructName(cast<StructType>(Ty)) << " " << NameSoFar;
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
static char *printConstantDataArray(ConstantDataArray *CPA, bool Static)
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
        strcat(cbuffer, printConstant(sep, cast<Constant>(CPA->getOperand(i)), Static));
        sep = ", ";
    }
    strcat(cbuffer, " }");
  }
    return strdup(cbuffer);
}
static char *writeOperandWithCastICmp(Value* Operand, bool shouldCast, bool typeIsSigned)
{
    char cbuffer[10000];
    cbuffer[0] = 0;
  if (shouldCast) {
      Type* OpTy = Operand->getType();
      ERRORIF (OpTy->isPointerTy());
      strcat(cbuffer, printType(OpTy, typeIsSigned, "", "((", ")"));
  }
  strcat(cbuffer, writeOperand(Operand, false));
  if (shouldCast)
      strcat(cbuffer, ")");
    return strdup(cbuffer);
}
static char *writeOperandWithCast(Value* Operand, unsigned Opcode)
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
  return writeOperandWithCastICmp(Operand, shouldCast, castIsSigned);
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
static char *printConstantWithCast(Constant* CPV, unsigned Opcode, const char *postfix)
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
      strcat(cbuffer, printConstant("", CPV, false));
      goto exitlab;
  }
  strcat(cbuffer, printType(CPV->getType(), typeIsSigned, "", "((", ")"));
  strcat(cbuffer, printConstant("", CPV, false));
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
      goto exitlab; // These don't need a source cast.
    case Instruction::IntToPtr: case Instruction::PtrToInt:
      strcat(cbuffer, "(unsigned long)");
      goto exitlab;
  }
  strcat(cbuffer, printType(SrcTy, TypeIsSigned, "", "(", ")"));
exitlab:
    return strdup(cbuffer);
}
static char *printGEPExpression(Value *Ptr, gep_type_iterator I, gep_type_iterator E, bool Static)
{
    char cbuffer[10000];
    cbuffer[0] = 0;
  ConstantInt *CI;
  if (I == E)
    return writeOperand(Ptr, false);
  VectorType *LastIndexIsVector = 0;
  for (gep_type_iterator TmpI = I; TmpI != E; ++TmpI)
      LastIndexIsVector = dyn_cast<VectorType>(*TmpI);
  strcat(cbuffer, "(");
  if (LastIndexIsVector)
    strcat(cbuffer, printType(PointerType::getUnqual(LastIndexIsVector->getElementType()), false, "", "((", ")("));
  Value *FirstOp = I.getOperand();
  if (!isa<Constant>(FirstOp) || !cast<Constant>(FirstOp)->isNullValue()) {
    strcat(cbuffer, "&");
    strcat(cbuffer, writeOperand(Ptr, false));
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
                strcat(cbuffer, printConstantDataArray(CA, Static));
                goto next;
            }
        }
        strcat(cbuffer, writeOperand(Ptr, true, Static));
next:
        if (val) {
            char temp[100];
            sprintf(temp, "+%ld", val);
            strcat(cbuffer, temp);
        }
    }
    else {
        strcat(cbuffer, "&");
        if (expose) {
          strcat(cbuffer, writeOperand(Ptr, true, Static));
        } else if (I != E && (*I)->isStructTy()) {
          strcat(cbuffer, writeOperand(Ptr, false));
          strcat(cbuffer, "->");
          strcat(cbuffer, fieldName(dyn_cast<StructType>(*I), cast<ConstantInt>(I.getOperand())->getZExtValue()));
          ++I;  // eat the struct index as well.
        } else {
          strcat(cbuffer, "(");
          strcat(cbuffer, writeOperand(Ptr, true));
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
      strcat(cbuffer, writeOperand(I.getOperand(), false));
      strcat(cbuffer, "]");
    } else {
      if (!isa<Constant>(I.getOperand()) || !cast<Constant>(I.getOperand())->isNullValue()) {
        strcat(cbuffer, ")+(");
        strcat(cbuffer, writeOperandWithCast(I.getOperand(), Instruction::GetElementPtr));
      }
      strcat(cbuffer, "))");
    }
  }
  strcat(cbuffer, ")");
    return strdup(cbuffer);
}
static char *printConstant(const char *prefix, Constant *CPV, bool Static)
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
      strcat(cbuffer, printConstant("", CE->getOperand(0), Static));
      if (CE->getType() == Type::getInt1Ty(CPV->getContext()) &&
          (op == Instruction::Trunc || op == Instruction::FPToUI ||
           op == Instruction::FPToSI || op == Instruction::PtrToInt))
        strcat(cbuffer, "&1u");
      break;
    case Instruction::GetElementPtr:
      strcat(cbuffer, printGEPExpression(CE->getOperand(0), gep_type_begin(CPV), gep_type_end(CPV), Static));
      break;
    case Instruction::Select:
      strcat(cbuffer, printConstant("", CE->getOperand(0), Static));
      strcat(cbuffer, printConstant("?", CE->getOperand(1), Static));
      strcat(cbuffer, printConstant(":", CE->getOperand(2), Static));
      break;
    case Instruction::Add: case Instruction::FAdd: case Instruction::Sub:
    case Instruction::FSub: case Instruction::Mul: case Instruction::FMul:
    case Instruction::SDiv: case Instruction::UDiv: case Instruction::FDiv:
    case Instruction::URem: case Instruction::SRem: case Instruction::FRem:
    case Instruction::And: case Instruction::Or: case Instruction::Xor:
    case Instruction::ICmp: case Instruction::Shl: case Instruction::LShr:
    case Instruction::AShr:
    {
      strcat(cbuffer, printConstantWithCast(CE->getOperand(0), op, " "));
      if (op == Instruction::ICmp)
        strcat(cbuffer, intmapLookup(predText, CE->getPredicate()));
      else
        strcat(cbuffer, intmapLookup(opcodeMap, op));
      strcat(cbuffer, " ");
      strcat(cbuffer, printConstantWithCast(CE->getOperand(1), op, ""));
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
        strcat(cbuffer, printConstantWithCast(CE->getOperand(0), op, ", "));
        strcat(cbuffer, printConstantWithCast(CE->getOperand(1), op, ")"));
      }
      strcat(cbuffer, printConstExprCast(CE));
      break;
    }
    default:
      errs() << "printConstant Error: Unhandled constant expression: " << *CE << "\n";
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
  ERRORIF (tid != Type::PointerTyID && !Static);
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
            strcat(cbuffer, printConstant(sep, cast<Constant>(CPA->getOperand(i)), Static));
            sep = ", ";
        }
        strcat(cbuffer, " }");
      }
    }
    else if (ConstantDataArray *CA = dyn_cast<ConstantDataArray>(CPV))
      strcat(cbuffer, printConstantDataArray(CA, Static));
    else
      ERRORIF(1);
    break;
  case Type::VectorTyID:
    strcat(cbuffer, "{");
    if (ConstantVector *CV = dyn_cast<ConstantVector>(CPV)) {
        for (unsigned i = 0, e = CV->getNumOperands(); i != e; ++i) {
            strcat(cbuffer, printConstant(sep, cast<Constant>(CV->getOperand(i)), Static));
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
        strcat(cbuffer, printConstant(sep, cast<Constant>(CPV->getOperand(i)), Static));
        sep = ", ";
    }
    strcat(cbuffer, " }");
    break;
  case Type::PointerTyID:
    if (isa<ConstantPointerNull>(CPV))
      strcat(cbuffer, printType(CPV->getType(), false, "", "((", ")/*NULL*/0)")); // sign doesn't matter
    else if (GlobalValue *GV = dyn_cast<GlobalValue>(CPV))
      strcat(cbuffer, writeOperand(GV, false, Static));
    break;
  default:
    errs() << "Unknown constant type: " << *CPV << "\n";
    llvm_unreachable(0);
  }
  }
exitlab:
    return strdup(cbuffer);
}
char *writeOperand(Value *Operand, bool Indirect, bool Static)
{
    char cbuffer[10000];
    cbuffer[0] = 0;
  Instruction *I = dyn_cast<Instruction>(Operand);
  bool isAddressImplicit = isAddressExposed(Operand);
  if (Indirect) {
      if (isAddressImplicit)
          isAddressImplicit = false;
      else
          strcat(cbuffer, "*");
  }
  if (isAddressImplicit)
      strcat(cbuffer, "(&");  // Global variables are referenced as their addresses by llvm
  if (I && isInlinableInst(*I)) {
      strcat(cbuffer, "(");
      strcat(cbuffer, processInstruction(NULL, *I));
      strcat(cbuffer, ")");
  }
  else {
      Constant* CPV = dyn_cast<Constant>(Operand);
      if (CPV && !isa<GlobalValue>(CPV))
          strcat(cbuffer, printConstant("", CPV, Static));
      else
          strcat(cbuffer, GetValueName(Operand).c_str());
  }
  if (isAddressImplicit)
      strcat(cbuffer, ")");
  return strdup(cbuffer);
}
char *printFunctionSignature(const Function *F, bool Prototype, const char *postfix)
{
  std::string tstr;
  raw_string_ostream FunctionInnards(tstr);
  const char *sep = "";
  const char *statstr = "";
  FunctionType *FT = cast<FunctionType>(F->getFunctionType());
  ERRORIF (F->hasDLLImportLinkage() || F->hasDLLExportLinkage() || F->hasStructRetAttr() || FT->isVarArg());
  if (F->hasLocalLinkage()) statstr = "static ";
  FunctionInnards << GetValueName(F) << '(';
  if (F->isDeclaration()) {
    for (FunctionType::param_iterator I = FT->param_begin(), E = FT->param_end(); I != E; ++I) {
      FunctionInnards << printType(*I, /*isSigned=*/false, "", sep, "");
      sep = ", ";
    }
  } else if (!F->arg_empty()) {
    for (Function::const_arg_iterator I = F->arg_begin(), E = F->arg_end(); I != E; ++I) {
      std::string ArgName = (I->hasName() || !Prototype) ? GetValueName(I) : "";
      FunctionInnards << printType(I->getType(), /*isSigned=*/false, ArgName, sep, "");
      sep = ", ";
    }
  }
  if (!strcmp(sep, ""))
    FunctionInnards << "void"; // ret() -> ret(void) in C.
  FunctionInnards << ')';
  return printType(F->getReturnType(), /*isSigned=*/false, FunctionInnards.str(), statstr, postfix);
}

/*
 * Output instructions
 */
const char *processInstruction(Function ***thisp, Instruction &I)
{
    char cbuffer[10000];
    cbuffer[0] = 0;
    int op = I.getOpcode();
    switch(I.getOpcode()) {
    case Instruction::Ret: {
        if (I.getNumOperands() != 0 || I.getParent()->getParent()->size() != 1) {
          strcat(cbuffer, "  return ");
          if (I.getNumOperands())
            strcat(cbuffer, writeOperand(I.getOperand(0), false));
          strcat(cbuffer, ";\n");
        }
        }
        break;
    case Instruction::Add: case Instruction::FAdd: case Instruction::Sub:
    case Instruction::FSub: case Instruction::Mul: case Instruction::FMul:
    case Instruction::UDiv: case Instruction::SDiv: case Instruction::FDiv:
    case Instruction::URem: case Instruction::SRem: case Instruction::FRem:
    case Instruction::Shl: case Instruction::LShr: case Instruction::AShr:
    case Instruction::And: case Instruction::Or: case Instruction::Xor: {
        assert(!I.getType()->isPointerTy());
        bool needsCast = false;
        if (I.getType() ==  Type::getInt8Ty(I.getContext())
         || I.getType() == Type::getInt16Ty(I.getContext())
         || I.getType() == Type::getFloatTy(I.getContext())) {
          needsCast = true;
          strcat(cbuffer, printType(I.getType(), false, "", "((", ")("));
        }
        if (BinaryOperator::isNeg(&I)) {
          strcat(cbuffer, "-(");
          strcat(cbuffer, writeOperand(BinaryOperator::getNegArgument(cast<BinaryOperator>(&I)), false));
          strcat(cbuffer, ")");
        } else if (BinaryOperator::isFNeg(&I)) {
          strcat(cbuffer, "-(");
          strcat(cbuffer, writeOperand(BinaryOperator::getFNegArgument(cast<BinaryOperator>(&I)), false));
          strcat(cbuffer, ")");
        } else if (I.getOpcode() == Instruction::FRem) {
          if (I.getType() == Type::getFloatTy(I.getContext()))
            strcat(cbuffer, "fmodf(");
          else if (I.getType() == Type::getDoubleTy(I.getContext()))
            strcat(cbuffer, "fmod(");
          else  // all 3 flavors of long double
            strcat(cbuffer, "fmodl(");
          strcat(cbuffer, writeOperand(I.getOperand(0), false));
          strcat(cbuffer, ", ");
          strcat(cbuffer, writeOperand(I.getOperand(1), false));
          strcat(cbuffer, ")");
        } else {
          strcat(cbuffer, writeOperandWithCast(I.getOperand(0), I.getOpcode()));
          strcat(cbuffer, " ");
          strcat(cbuffer, intmapLookup(opcodeMap, I.getOpcode()));
          strcat(cbuffer, " ");
          strcat(cbuffer, writeOperandWithCast(I.getOperand(1), I.getOpcode()));
          strcat(cbuffer, writeInstructionCast(I));
        }
        if (needsCast) {
          strcat(cbuffer, "))");
        }
        }
        break;
    case Instruction::Load: {
        LoadInst &IL = static_cast<LoadInst&>(I);
        ERRORIF (IL.isVolatile());
        strcat(cbuffer, writeOperand(I.getOperand(0), true));
        }
        break;
    case Instruction::Store: {
        StoreInst &IS = static_cast<StoreInst&>(I);
        ERRORIF (IS.isVolatile());
        strcat(cbuffer, writeOperand(IS.getPointerOperand(), true));
        strcat(cbuffer, " = ");
        Value *Operand = I.getOperand(0);
        Constant *BitMask = 0;
        IntegerType* ITy = dyn_cast<IntegerType>(Operand->getType());
        if (ITy && !ITy->isPowerOf2ByteWidth())
            BitMask = ConstantInt::get(ITy, ITy->getBitMask());
        if (BitMask)
          strcat(cbuffer, "((");
        strcat(cbuffer, writeOperand(Operand, false));
        if (BitMask) {
          strcat(cbuffer, printConstant(") & ", BitMask, false));
          strcat(cbuffer, ")");
        }
        }
        break;
    case Instruction::GetElementPtr: {
        GetElementPtrInst &IG = static_cast<GetElementPtrInst&>(I);
        strcat(cbuffer, printGEPExpression(IG.getPointerOperand(), gep_type_begin(IG), gep_type_end(IG), false));
        }
        break;
    case Instruction::Trunc:
    case Instruction::ZExt: case Instruction::SExt: case Instruction::FPToUI:
    case Instruction::FPToSI: case Instruction::UIToFP: case Instruction::SIToFP:
    case Instruction::FPTrunc: case Instruction::FPExt: case Instruction::PtrToInt:
    case Instruction::IntToPtr: case Instruction::BitCast: case Instruction::AddrSpaceCast: {
        Type *DstTy = I.getType();
        Type *SrcTy = I.getOperand(0)->getType();
        strcat(cbuffer, "(");
        strcat(cbuffer, printCast(op, SrcTy, DstTy));
        if (SrcTy == Type::getInt1Ty(I.getContext()) && op == Instruction::SExt)
          strcat(cbuffer, "0-");
        strcat(cbuffer, writeOperand(I.getOperand(0), false));
        if (DstTy == Type::getInt1Ty(I.getContext()) &&
            (op == Instruction::Trunc || op == Instruction::FPToUI ||
             op == Instruction::FPToSI || op == Instruction::PtrToInt)) {
          strcat(cbuffer, "&1u");
        }
        strcat(cbuffer, ")");
        }
        break;
    case Instruction::ICmp: {
        ICmpInst &ICM = static_cast<ICmpInst&>(I);
        bool shouldCast = ICM.isRelational();
        bool typeIsSigned = ICM.isSigned();
        strcat(cbuffer, writeOperandWithCastICmp(I.getOperand(0), shouldCast, typeIsSigned));
        strcat(cbuffer, " ");
        strcat(cbuffer, intmapLookup(predText, ICM.getPredicate()));
        strcat(cbuffer, " ");
        strcat(cbuffer, writeOperandWithCastICmp(I.getOperand(1), shouldCast, typeIsSigned));
        strcat(cbuffer, writeInstructionCast(I));
        if (shouldCast)
          strcat(cbuffer, "))");
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
        PointerType  *PTy = cast<PointerType>(Callee->getType());
        FunctionType *FTy = cast<FunctionType>(PTy->getElementType());
        ConstantExpr *CE = dyn_cast<ConstantExpr>(Callee);
        Function *RF = NULL;
        if (CE && CE->isCast() && (RF = dyn_cast<Function>(CE->getOperand(0)))) {
            Callee = RF;
            strcat(cbuffer, printType(ICL.getCalledValue()->getType(), false, "", "((", ")(void*)"));
        }
        strcat(cbuffer, writeOperand(Callee, false));
        if (RF)
            strcat(cbuffer, ")");
        ERRORIF(FTy->isVarArg() && !FTy->getNumParams());
        unsigned len = FTy->getNumParams();
        CallSite CS(&I);
        CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();
        strcat(cbuffer, "(");
        for (; AI != AE; ++AI, ++ArgNo) {
            strcat(cbuffer, sep);
            if (ArgNo < len && (*AI)->getType() != FTy->getParamType(ArgNo))
                strcat(cbuffer, printType(FTy->getParamType(ArgNo), /*isSigned=*/false, "", "(", ")"));
            strcat(cbuffer, writeOperand(*AI, false));
            sep = ", ";
        }
        strcat(cbuffer, ")");
        vtablework.push_back(VTABLE_WORK(func, NULL, SLOTARRAY_TYPE()));
        }
        break;
    default:
        errs() << "C Writer does not know about " << I;
        llvm_unreachable(0);
    }
    return strdup(cbuffer);
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
void generateCppData(FILE *OStr, Module &Mod)
{
    ArrayType *ATy;
    PointerType *PTy, *PPTy;
    const FunctionType *FT;
    StructType *STy, *ISTy;
    const ConstantExpr *CE;
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
                fprintf(OStr, " = %s", writeOperand(I->getInitializer(), false, true));
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
              fprintf(OStr, "%s", printType(Ty, false, GetValueName(I), "", ""));
              if (!I->getInitializer()->isNullValue()) {
                fprintf(OStr, " = " );
                Constant* CPV = dyn_cast<Constant>(I->getInitializer());
                if (ConstantArray *CA = dyn_cast<ConstantArray>(CPV)) {
                    Type *ETy = CA->getType()->getElementType();
                    ERRORIF (ETy == Type::getInt8Ty(CA->getContext()) || ETy == Type::getInt8Ty(CA->getContext()));
                    fprintf(OStr, "{");
                    const char *sep = " ";
                    if ((CE = dyn_cast<ConstantExpr>(CA->getOperand(3))) && CE->getOpcode() == Instruction::BitCast
                     && (PTy = cast<PointerType>(CE->getOperand(0)->getType())) && (FT = dyn_cast<FunctionType>(PTy->getElementType()))
                     && FT->getNumParams() >= 1 && (PPTy = cast<PointerType>(FT->getParamType(0)))
                     && (STy = cast<StructType>(PPTy->getElementType()))
                     && STy->getNumElements() > 0 && STy->getElementType(0)->getTypeID() == Type::StructTyID
                     && (ISTy = cast<StructType>(STy->getElementType(0))) && !strcmp(ISTy->getName().str().c_str(), "class.Rule"))
                        for (unsigned i = 2, e = CA->getNumOperands(); i != e; ++i) {
                          Constant* V = dyn_cast<Constant>(CA->getOperand(i));
                          fprintf(OStr, "%s", printConstant(sep, V, true));
                          sep = ", ";
                        }
                    else
                        fprintf(OStr, "0");
                    fprintf(OStr, " }");
                }
              }
              fprintf(OStr, ";\n");
          }
    }
}

static void printContainedStructs(Type *Ty, FILE *OStr)
{
    std::map<Type *, int>::iterator FI = structMap.find(Ty);
    if (FI == structMap.end() && !Ty->isPointerTy() && !Ty->isPrimitiveType() && !Ty->isIntegerTy()) {
        structMap[Ty] = 1;
        for (Type::subtype_iterator I = Ty->subtype_begin(), E = Ty->subtype_end(); I != E; ++I)
            printContainedStructs(*I, OStr);
        if (StructType *STy = dyn_cast<StructType>(Ty)) {
            std::string name = getStructName(STy);
            fprintf(OStr, "typedef struct %s {\n", name.c_str());
            unsigned Idx = 0;
            for (StructType::element_iterator I = STy->element_begin(), E = STy->element_end(); I != E; ++I)
              fprintf(OStr, "%s", printType(*I, false, fieldName(STy, Idx++), "  ", ";\n"));
            fprintf(OStr, "} %s;\n\n", name.c_str());
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
        if (!(I->isIntrinsic() || I->getName() == "main" || I->getName() == "atexit"
         || I->getName() == "printf" || I->getName() == "__cxa_pure_virtual"
         || I->getName() == "setjmp" || I->getName() == "longjmp" || I->getName() == "_setjmp"))
            fprintf(OStr, "%s", printFunctionSignature(I, true, ";\n"));
    }
    UnnamedStructIDs.clear();
}
