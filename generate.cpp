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
#include <cxxabi.h> // abi::__cxa_demangle
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ExecutionEngine/GenericValue.h"

using namespace llvm;

#include "declarations.h"

int trace_translate ;//= 1;
static std::list<VTABLE_WORK> vtablework;
const Function *EntryFn;
const char *globalName;
std::map<std::string,ClassMethodTable *> classCreate;
std::map<Function *,ClassMethodTable *> functionIndex;
static std::list<const StructType *> structWork;
static int structWork_run;
std::map<std::string, void *> nameMap;
static DenseMap<const Value*, unsigned> AnonValueNumbers;
unsigned NextAnonValueNumber;
DenseMap<const StructType*, unsigned> UnnamedStructIDs;
unsigned NextTypeID;
int regen_methods;
int generateRegion;

INTMAP_TYPE predText[] = {
    {FCmpInst::FCMP_FALSE, "false"}, {FCmpInst::FCMP_OEQ, "oeq"},
    {FCmpInst::FCMP_OGT, "ogt"}, {FCmpInst::FCMP_OGE, "oge"},
    {FCmpInst::FCMP_OLT, "olt"}, {FCmpInst::FCMP_OLE, "ole"},
    {FCmpInst::FCMP_ONE, "one"}, {FCmpInst::FCMP_ORD, "ord"},
    {FCmpInst::FCMP_UNO, "uno"}, {FCmpInst::FCMP_UEQ, "ueq"},
    {FCmpInst::FCMP_UGT, "ugt"}, {FCmpInst::FCMP_UGE, "uge"},
    {FCmpInst::FCMP_ULT, "ult"}, {FCmpInst::FCMP_ULE, "ule"},
    {FCmpInst::FCMP_UNE, "une"}, {FCmpInst::FCMP_TRUE, "true"},
    {ICmpInst::ICMP_EQ, "=="}, {ICmpInst::ICMP_NE, "!="},
    {ICmpInst::ICMP_SGT, ">"}, {ICmpInst::ICMP_SGE, ">="},
    {ICmpInst::ICMP_SLT, "<"}, {ICmpInst::ICMP_SLE, "<="},
    {ICmpInst::ICMP_UGT, ">"}, {ICmpInst::ICMP_UGE, ">="},
    {ICmpInst::ICMP_ULT, "<"}, {ICmpInst::ICMP_ULE, "<="}, {}};
INTMAP_TYPE opcodeMap[] = {
    {Instruction::Add, "+"}, {Instruction::FAdd, "+"},
    {Instruction::Sub, "-"}, {Instruction::FSub, "-"},
    {Instruction::Mul, "*"}, {Instruction::FMul, "*"},
    {Instruction::UDiv, "/"}, {Instruction::SDiv, "/"}, {Instruction::FDiv, "/"},
    {Instruction::URem, "%"}, {Instruction::SRem, "%"}, {Instruction::FRem, "%"},
    {Instruction::And, "&"}, {Instruction::Or, "|"}, {Instruction::Xor, "^"},
    {Instruction::Shl, "<<"}, {Instruction::LShr, ">>"}, {Instruction::AShr, " >> "}, {}};

/*
 * Common utilities for processing Instruction lists
 */

const char *intmapLookup(INTMAP_TYPE *map, int value)
{
    while (map->name) {
        if (map->value == value)
            return map->name;
        map++;
    }
    return "unknown";
}

void recursiveDelete(Value *V)
{
    Instruction *I = dyn_cast<Instruction>(V);
    if (!I)
        return;
    for (unsigned i = 0, e = I->getNumOperands(); i != e; ++i) {
        Value *OpV = I->getOperand(i);
        I->setOperand(i, 0);
        if (OpV->use_empty())
            recursiveDelete(OpV);
    }
    I->eraseFromParent();
}

static uint64_t LoadValueFromMemory(PointerTy Ptr, Type *Ty)
{
    const DataLayout *TD = EE->getDataLayout();
    const unsigned LoadBytes = TD->getTypeStoreSize(Ty);
    uint64_t rv = 0;

    if (trace_full)
        printf("[%s:%d] bytes %d type %x\n", __FUNCTION__, __LINE__, LoadBytes, Ty->getTypeID());
    switch (Ty->getTypeID()) {
    case Type::IntegerTyID:
        if (LoadBytes > sizeof(rv)) {
            printf("[%s:%d]\n", __FUNCTION__, __LINE__);
            exit(1);
        }
        memcpy(&rv, (uint8_t*)Ptr, LoadBytes);
        break;
    case Type::PointerTyID:
        if (!Ptr) {
            printf("[%s:%d] %p\n", __FUNCTION__, __LINE__, Ptr);
            exit(1);
        }
        else
            rv = (uint64_t) *((PointerTy*)Ptr);
        break;
    default:
        outs() << "Cannot load value of type " << *Ty << "!";
        exit(1);
    }
    if (trace_full)
        printf("[%s:%d] rv %llx\n", __FUNCTION__, __LINE__, (long long)rv);
    return rv;
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
std::string CBEMangle(const std::string &S)
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
static const char *lookupMember(const StructType *STy, uint64_t ind, int tag)
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
        //printf("[%s:%d] tag %x name %s\n", __FUNCTION__, __LINE__, Ty.getTag(), CBEMangle(Ty.getName().str()).c_str());
        if (Ty.getTag() == tag) {
            if (!ind--)
                return CBEMangle(Ty.getName().str()).c_str();
        }
    }
    }
    sprintf(temp, "field%d", (int)ind);
    return temp;
}
const char *fieldName(const StructType *STy, uint64_t ind)
{
    return lookupMember(STy, ind, dwarf::DW_TAG_member);
}
const char *methodName(const StructType *STy, uint64_t ind)
{
    return lookupMember(STy, ind, dwarf::DW_TAG_subprogram);
}
std::string getStructName(const StructType *STy)
{
    std::string name;
    assert(STy);
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

const StructType *findThisArgument(Function *func)
{
    const PointerType *PTy;
    const StructType *STy = NULL;
    if (func->arg_begin() != func->arg_end()
     && func->arg_begin()->getName() == "this"
     && (PTy = dyn_cast<PointerType>(func->arg_begin()->getType())))
        STy = dyn_cast<StructType>(PTy->getPointerElementType());
    return STy;
}

const StructType *findThisArgumentType(FunctionType *func)
{
    const PointerType *PTy;
    const StructType *STy = NULL;
    if (func->getNumParams() > 0
     && (PTy = dyn_cast<PointerType>(func->getParamType(0))))
        STy = dyn_cast<StructType>(PTy->getPointerElementType());
    return STy;
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
  if (generateRegion == 1 && VarName != "this")
      return globalName + ("_" + VarName);
  if (regen_methods)
      return VarName;
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
char *writeOperandWithCastICmp(Function ***thisp, Value* Operand, bool shouldCast, bool typeIsSigned)
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
char *writeOperandWithCast(Function ***thisp, Value* Operand, unsigned Opcode)
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
const char *writeInstructionCast(const Instruction &I)
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
const char *printCast(unsigned opc, Type *SrcTy, Type *DstTy)
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
/*
 * GEP and Load instructions interpreter functions
 * (just execute using the memory areas allocated by the constructors)
 */
static char *printGEPExpression(Function ***thisp, Value *Ptr, gep_type_iterator I, gep_type_iterator E)
{
    char cbuffer[10000];
    cbuffer[0] = 0;
    ConstantInt *CI;
    void *tval = NULL;
    char *p;
    uint64_t Total = 0;
    const DataLayout *TD = EE->getDataLayout();
    if (I == E)
        return getOperand(thisp, Ptr, false);
    VectorType *LastIndexIsVector = 0;
    for (gep_type_iterator TmpI = I; TmpI != E; ++TmpI) {
        LastIndexIsVector = dyn_cast<VectorType>(*TmpI);
    //for (; I != E; ++I) 
        if (StructType *STy = dyn_cast<StructType>(*TmpI)) {
            const StructLayout *SLO = TD->getStructLayout(STy);
            const ConstantInt *CPU = cast<ConstantInt>(TmpI.getOperand());
            Total += SLO->getElementOffset(CPU->getZExtValue());
        } else {
            SequentialType *ST = cast<SequentialType>(*TmpI);
            // Get the index number for the array... which must be long type...
            const Constant *CV = dyn_cast<Constant>(TmpI.getOperand());
            const ConstantInt *CI;
            ERRORIF(!CV || isa<GlobalValue>(CV) || !(CI = dyn_cast<ConstantInt>(CV)));
            Total += TD->getTypeAllocSize(ST->getElementType()) * CI->getZExtValue();
        }
    }
    strcat(cbuffer, "(");
    if (LastIndexIsVector)
        strcat(cbuffer, printType(PointerType::getUnqual(LastIndexIsVector->getElementType()), false, "", "((", ")("));
    Value *FirstOp = I.getOperand();
    if (!isa<Constant>(FirstOp) || !cast<Constant>(FirstOp)->isNullValue()) {
    p = getOperand(thisp, Ptr, false);
    if (strlen(p) > 12 && !strncmp(p, "(*((", 4) && !strncmp(&p[strlen(p)-9], "*))this))", 9)) {
        PointerType *PTy;
        FunctionType *FTy;
        const StructType *STy;
        if ((PTy = dyn_cast<PointerType>(Ptr->getType()))
         && (PTy = dyn_cast<PointerType>(PTy->getElementType()))
         && (FTy = dyn_cast<FunctionType>(PTy->getElementType()))
         && (STy = findThisArgumentType(FTy))) {
            const char *name = methodName(STy, 1+        //// WHY????????????????
                   Total/sizeof(void *));
            printf("[%s:%d] name %s\n", __FUNCTION__, __LINE__, name);
            strcat(cbuffer, "&");
            strcat(cbuffer, name);
            goto exitlab;
        }
    }
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
          const char *p = getOperand(thisp, Ptr, true);
          strcat(cbuffer, p);
        } else if (I != E && (*I)->isStructTy()) {
          const char *p = getOperand(thisp, Ptr, false);
          char *ptemp = strdup(p);
          std::map<std::string, void *>::iterator NI = nameMap.find(p);
          const char *fieldp = fieldName(dyn_cast<StructType>(*I), cast<ConstantInt>(I.getOperand())->getZExtValue());
//printf("[%s:%d] writeop %s found %d\n", __FUNCTION__, __LINE__, p, (NI != nameMap.end()));
          if (!strcmp(fieldp, "module") && !strcmp(p, "Vthis")) {
              tval = thisp;
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
          if (strcmp(p, "this")) {
              strcat(cbuffer, p);
              strcat(cbuffer, "->");
          }
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
      const char *p = processInstruction(thisp, I);
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
char *printFunctionSignature(const Function *F, const char *altname, bool Prototype, const char *postfix, int skip)
{
  std::string tstr;
  raw_string_ostream FunctionInnards(tstr);
  const char *sep = "";
  const char *statstr = "";
  FunctionType *FT = cast<FunctionType>(F->getFunctionType());
  ERRORIF (F->hasDLLImportLinkage() || F->hasDLLExportLinkage() || F->hasStructRetAttr() || FT->isVarArg());
  if (F->hasLocalLinkage()) statstr = "static ";
  if (altname)
      FunctionInnards << altname;
  else
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
int processVar(const GlobalVariable *GV)
{
  if (GV->isDeclaration() || GV->getSection() == "llvm.metadata"
   || (GV->hasAppendingLinkage() && GV->use_empty()
    && (GV->getName() == "llvm.global_ctors" || GV->getName() == "llvm.global_dtors")))
      return 0;
  return 1;
}
int checkIfRule(Type *aTy)
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

ClassMethodTable *findClass(std::string tname)
{
    std::map<std::string, ClassMethodTable *>::iterator FI = classCreate.find(tname);
    if (FI != classCreate.end())
        return classCreate[tname];
    return NULL;
}

const char *processInstruction(Function ***thisp, Instruction *ins)
{
    //outs() << "processinst" << *ins;
    switch (ins->getOpcode()) {
    case Instruction::GetElementPtr:
        {
        GetElementPtrInst &IG = static_cast<GetElementPtrInst&>(*ins);
        return printGEPExpression(thisp, IG.getPointerOperand(), gep_type_begin(IG), gep_type_end(IG));
        }
        break;
    case Instruction::Load:
        {
        LoadInst &IL = static_cast<LoadInst&>(*ins);
        ERRORIF (IL.isVolatile());
        return getOperand(thisp, ins->getOperand(0), true);
        }
        break;
    default:
        {
        if (generateRegion == 2)
            return processCInstruction(thisp, *ins);
        else
            return generateRegion ? generateVerilog(thisp, *ins)
                            : calculateGuardUpdate(thisp, *ins);
        }
        break;
    case Instruction::Alloca: // ignore
        if (generateRegion == 2) {
            if (const AllocaInst *AI = isDirectAlloca(&*ins))
              return printType(AI->getAllocatedType(), false, GetValueName(AI), "    ", ";    /* Address-exposed local */\n");
        }
        break;
    }
    return NULL;
}

static int getClassName(const char *name, const char **className, const char **methodName)
{
    int status;
    static char temp[1000];
    char *pmethod = temp;
    temp[0] = 0;
    *className = NULL;
    *methodName = NULL;
    const char *demang = abi::__cxa_demangle(name, 0, 0, &status);
    if (demang) {
        strcpy(temp, demang);
        while (*pmethod && pmethod[0] != '(')
            pmethod++;
        *pmethod = 0;
        while (pmethod > temp && pmethod[0] != ':')
            pmethod--;
        char *p = pmethod++;
        while (p[0] == ':')
            *p-- = 0;
        int len = 0;
        const char *p1 = demang;
        while (*p1 && *p1 != '(')
            p1++;
        while (p1 != demang && *p1 != ':') {
            len++;
            p1--;
        }
        *className = temp;
        *methodName = pmethod;
        return 1;
    }
    return 0;
}

/*
 * Walk all BasicBlocks for a Function, calling requested processing function
 */
static std::map<const Function *, int> funcSeen;
void processFunction(VTABLE_WORK &work, const char *newName, FILE *outputFile)
{
    Function *func = work.f;
    std::string guardName;
    int hasRet = (func->getReturnType() != Type::getVoidTy(func->getContext()));

    //if (regen_methods && getClassName(func->getName().str().c_str(), &className, &methodName))
        //globalName = methodName;
    if (newName)
        globalName = newName;
    else {
        globalName = strdup(func->getName().str().c_str());
        fprintf(outputFile, "\n//processing %s\n", globalName);
    }
printf("[%s:%d] %p processing %s\n", __FUNCTION__, __LINE__, func, globalName);
    if (generateRegion == 1 && !strncmp(&globalName[strlen(globalName) - 9], "6updateEv", 9)) {
        guardName = globalName;
        guardName = guardName.substr(1, strlen(globalName) - 10);
        fprintf(outputFile, "    if (%s5guardEv && %s__ENA) begin\n", guardName.c_str(), globalName);
    }
    NextAnonValueNumber = 0;
    nameMap.clear();
    if (trace_translate) {
        printf("FULL_AFTER_OPT: %s\n", func->getName().str().c_str());
        func->dump();
        printf("TRANSLATE:\n");
    }
#if 0
    /* connect up argument formal param names with actual values */
    for (Function::const_arg_iterator AI = func->arg_begin(), AE = func->arg_end(); AI != AE; ++AI) {
        int slotindex = getLocalSlot(AI);
        if (AI->hasByValAttr()) {
            printf("[%s] hasByVal param not supported\n", __FUNCTION__);
            exit(1);
        }
        slotarray[slotindex].name = strdup(AI->getName().str().c_str());
    }
#endif
    /* Generate Verilog for all instructions.  Record function calls for post processing */
    if (generateRegion == 2) {
        int status;
        const char *demang = abi::__cxa_demangle(globalName, 0, 0, &status);
        std::map<const Function *, int>::iterator MI = funcSeen.find(func);
        if (!newName && ((demang && strstr(demang, "::~"))
         || func->isDeclaration() || !strcmp(globalName, "_Z16run_main_programv") || !strcmp(globalName, "main")
         || !strcmp(globalName, "__dtor_echoTest")
         || MI != funcSeen.end()))
            return; // MI->second->name;
        funcSeen[func] = 1;
        fprintf(outputFile, "%s", printFunctionSignature(func, newName, false, " {\n", newName != NULL));
    }
    //manually done (only for methods) nameMap["Vthis"] = work.thisp;
    for (Function::iterator BB = func->begin(), E = func->end(); BB != E; ++BB) {
        if (trace_translate && BB->hasName())         // Print out the label if it exists...
            printf("LLLLL: %s\n", BB->getName().str().c_str());
        for (BasicBlock::iterator ins = BB->begin(), ins_end = BB->end(); ins != ins_end;) {
            char instruction_label[MAX_CHAR_BUFFER];

            BasicBlock::iterator next_ins = llvm::next(BasicBlock::iterator(ins));
            if (trace_translate)
            printf("%s    XLAT:%14s", instruction_label, ins->getOpcodeName());
            if (!isInlinableInst(*ins)) {
                if (trace_translate && generateRegion == 2)
                    printf("/*before %p opcode %d.=%s*/\n", &*ins, ins->getOpcode(), ins->getOpcodeName());
            const char *vout = processInstruction(work.thisp, ins);
            if (vout) {
                if (vout[0] == '&' && !isDirectAlloca(&*ins) && ins->getType() != Type::getVoidTy(BB->getContext())
                 && ins->use_begin() != ins->use_end()) {
                    std::string name = GetValueName(&*ins);
                    void *tval = mapLookup(vout+1);
                    if (tval)
                        nameMap[name] = tval;
                }
                if (generateRegion == 2) {
                    if (strcmp(vout, "")) {
                        if (!isDirectAlloca(&*ins) && ins->getType() != Type::getVoidTy(BB->getContext())
                         && ins->use_begin() != ins->use_end()) {
                            std::string name = GetValueName(&*ins);
                            fprintf(outputFile, "%s", printType(ins->getType(), false, name, "", " = "));
                        }
                        fprintf(outputFile, "        %s;\n", vout);
                    }
                }
                else {
                    fprintf(outputFile, "        ");
                    if (!hasRet)
                        fprintf(outputFile, "    ");
                    if (!isDirectAlloca(&*ins) && ins->getType() != Type::getVoidTy(BB->getContext())
                     && ins->use_begin() != ins->use_end()) {
                        fprintf(outputFile, "%s = ", GetValueName(&*ins).c_str());
                    }
                    fprintf(outputFile, "%s\n", vout);
                }
            }
            }
            if (trace_translate)
                printf("\n");
            ins = next_ins;
        }
    }
    if (guardName != "")
        fprintf(outputFile, "    end; // if (%s5guardEv && %s__ENA) \n", guardName.c_str(), globalName);
    if (generateRegion == 2)
        fprintf(outputFile, "}\n\n");
    else if (!newName)
        fprintf(outputFile, "\n");
}

void pushWork(Function *func, Function ***thisp)
{
    const char *className, *methodName;
    const StructType *STy;

    if (!func)
        return;
    if (getClassName(func->getName().str().c_str(), &className, &methodName)
     && (STy = findThisArgument(func))) {
        std::string sname = getStructName(STy);
        std::string tname = STy->getName();
        CLASS_META *mptr = lookup_class(tname.c_str());
        if (mptr->node->getNumOperands() > 3
         && mptr->node->getOperand(3)->getName() == className) {
            if (!findClass(sname))
                classCreate[sname] = new ClassMethodTable(sname, className);
            classCreate[sname]->method[func] = methodName;
            functionIndex[func] = classCreate[sname];
        }
    }
    vtablework.push_back(VTABLE_WORK(func, thisp));
}

/*
 * Symbolically run through all rules, running either preprocessing or
 * generating verilog.
 */
static void processRules(Function ***modp, FILE *outputFile, FILE *outputNull)
{
    int ModuleRfirst= lookup_field("class.Module", "rfirst")/sizeof(uint64_t);
    int ModuleNext  = lookup_field("class.Module", "next")/sizeof(uint64_t);
    int RuleNext    = lookup_field("class.Rule", "next")/sizeof(uint64_t);

    // Walk the rule lists for all modules, generating work items
    while (modp) {                   // loop through all modules
        printf("Module %p: rfirst %p next %p\n", modp, modp[ModuleRfirst], modp[ModuleNext]);
        Function ***rulep = (Function ***)modp[ModuleRfirst];        // Module.rfirst
        while (rulep) {                      // loop through all rules for module
            printf("Rule %p: next %p\n", rulep, rulep[RuleNext]);
            static std::string method[] = { "update", "guard", ""};
            std::string *p = method;
            do {
                pushWork(rulep[0][lookup_method("class.Rule", *p)], (Function ***)rulep);
            } while (*++p != "");
            rulep = (Function ***)rulep[RuleNext];           // Rule.next
        }
        modp = (Function ***)modp[ModuleNext]; // Module.next
    }

    // Walk list of work items, generating code
    while (vtablework.begin() != vtablework.end()) {
        std::map<Function *,ClassMethodTable *>::iterator NI = functionIndex.find(vtablework.begin()->f);
        processFunction(*vtablework.begin(), NULL,
           ((NI != functionIndex.end()) ? outputNull : outputFile));
        vtablework.pop_front();
    }
}

static std::map<const Type *, int> structMap;
static void printContainedStructs(const Type *Ty, FILE *OStr)
{
    std::map<const Type *, int>::iterator FI = structMap.find(Ty);
    const PointerType *PTy = dyn_cast<PointerType>(Ty);
    if (PTy) {
        const StructType *subSTy = dyn_cast<StructType>(PTy->getElementType());
        if (subSTy) { /* Not recursion!  These are generated afterword, if we didn't generate before */
            std::map<const Type *, int>::iterator FI = structMap.find(subSTy);
            if (FI != structMap.end())
                structWork.push_back(subSTy);
        }
    }
    else if (FI == structMap.end() && !Ty->isPrimitiveType() && !Ty->isIntegerTy()) {
        const StructType *STy = dyn_cast<StructType>(Ty);
        std::string name;
        structMap[Ty] = 1;
        if (STy) {
            name = getStructName(STy);
            if (generateRegion == 2)
                fprintf(OStr, "class %s;\n", name.c_str());
        }
        for (Type::subtype_iterator I = Ty->subtype_begin(), E = Ty->subtype_end(); I != E; ++I)
            printContainedStructs(*I, OStr);
        if (STy) {
            for (StructType::element_iterator I = STy->element_begin(), E = STy->element_end(); I != E; ++I)
                printContainedStructs(*I, OStr);
            if (generateRegion == 1)
                generateModuleDef(STy, OStr);
            else
                generateClassDef(STy, OStr);
        }
    }
}
void generateStructs(FILE *OStr)
{
    structWork_run = 1;
    structMap.clear();
    while (structWork.begin() != structWork.end()) {
        printContainedStructs(*structWork.begin(), OStr);
        structWork.pop_front();
    }
    structWork_run = 0;
}

char GeneratePass::ID = 0;
bool GeneratePass::runOnModule(Module &Mod)
{
    std::string ErrorMsg;
    // preprocessing dwarf debuf info before running anything
    NamedMDNode *CU_Nodes = Mod.getNamedMetadata("llvm.dbg.cu");
    if (CU_Nodes)
        process_metadata(CU_Nodes);

    EngineBuilder builder(&Mod);
    builder.setMArch(MArch);
    builder.setMCPU("");
    builder.setMAttrs(MAttrs);
    builder.setErrorStr(&ErrorMsg);
    builder.setEngineKind(EngineKind::Interpreter);
    builder.setOptLevel(CodeGenOpt::None);

    // Create the execution environment and allocate memory for static items
    EE = builder.create();
    if (!EE) {
        printf("llvm-translate: unknown error creating EE!\n");
        exit(1);
    }

    Function **** modfirst = (Function ****)EE->getPointerToGlobal(Mod.getNamedValue("_ZN6Module5firstE"));
    EntryFn = Mod.getFunction("main");
    if (!EntryFn || !modfirst) {
        printf("'main' function not found in module.\n");
        exit(1);
    }

    *modfirst = NULL;       // init the Module list before calling constructors
    // run Constructors
    EE->runStaticConstructorsDestructors(false);

    // Construct the address -> symbolic name map using dwarf debug info
    constructAddressMap(CU_Nodes);

    // Preprocess the body rules, creating shadow variables and moving items to guard() and update()
    generateRegion = 0;
    processRules(*modfirst, OutNull, OutNull);

    // Generating code for all rules
    generateRegion = 1;
    fprintf(outputFile, "module top(input CLK, input RST);\n  always @( posedge CLK) begin\n    if RST then begin\n    end\n    else begin\n");
    processRules(*modfirst, outputFile, OutNull);
    fprintf(outputFile, "  end // always @ (posedge CLK)\nendmodule \n\n");
    generateStructs(outputFile);
    generateVerilogHeader(Mod, outputFile, OutNull);

    // Generate cpp code
    generateRegion = 2;
    generateCppData(Out, Mod);

    // Generating code for all rules
    processRules(*modfirst, Out, OutNull);

    generateStructs(OutHeader);
    generateCppHeader(Mod, OutHeader);
    return false;
}
