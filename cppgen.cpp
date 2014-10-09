/* Copyright (c) 2014 Quanta Research Cambridge, Inc
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
#include <list>
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/InstIterator.h"

using namespace llvm;

#include "declarations.h"

static std::list<StructType *> structWork;
static std::map<Type *, int> structMap;
static int structWork_run;
char CWriter::ID = 0;

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
std::string CWriter::getStructName(StructType *ST)
{
    if (!ST->isLiteral() && !ST->getName().empty())
        return CBEMangle("l_"+ST->getName().str());
    if (!UnnamedStructIDs[ST])
        UnnamedStructIDs[ST] = NextTypeID++;
    return "l_unnamed_" + utostr(UnnamedStructIDs[ST]);
}
void CWriter::printType(raw_ostream &Out, Type *Ty, bool isSigned, std::string NameSoFar, bool IgnoreName, bool isStatic, std::string prefix, std::string postfix)
{
  const char *sp = (isSigned?"signed":"unsigned");
  Out << prefix;
restart_label:
  switch (Ty->getTypeID()) {
  case Type::VoidTyID:
      Out << "void";
      Out << " " << NameSoFar;
      break;
  case Type::IntegerTyID: {
      unsigned NumBits = cast<IntegerType>(Ty)->getBitWidth();
      assert(NumBits <= 128 && "Bit widths > 128 not implemented yet");
      if (NumBits == 1)
        Out << "bool";
      else if (NumBits <= 8)
        Out << sp << " char";
      else if (NumBits <= 16)
        Out << sp << " short";
      else if (NumBits <= 32)
        Out << sp << " int";
      else if (NumBits <= 64)
        Out << sp << " long long";
      }
      Out << " " << NameSoFar;
      break;
  case Type::VectorTyID: {
      VectorType *VTy = cast<VectorType>(Ty);
      Ty = VTy->getElementType();
      NameSoFar = " __attribute__((vector_size(""utostr(TD->getTypeAllocSize(VTy))"" ))) " + NameSoFar;
      goto restart_label;
      }
  case Type::MetadataTyID:
      Out << "MetadataTyIDSTUB\n";
      Out << " " << NameSoFar;
      break;
  case Type::FunctionTyID: {
    FunctionType *FTy = cast<FunctionType>(Ty);
    std::string tstr;
    raw_string_ostream FunctionInnards(tstr);
    FunctionInnards << " (" << NameSoFar << ") (";
    bool PrintedArg = false;
    for (FunctionType::param_iterator I = FTy->param_begin(), E = FTy->param_end(); I != E; ++I) {
      if (PrintedArg)
          FunctionInnards << ", ";
      printType(FunctionInnards, *I, /*isSigned=*/false, "", false, false, "", "");
      PrintedArg = true;
    }
    if (FTy->isVarArg()) {
      if (!FTy->getNumParams())
        FunctionInnards << " int"; //dummy argument for empty vaarg functs
      FunctionInnards << ", ...";
    } else if (!FTy->getNumParams())
      FunctionInnards << "void";
    FunctionInnards << ')';
    printType(Out, FTy->getReturnType(), /*isSigned=*/false, FunctionInnards.str(), false, false, "", "");
    break;
  }
  case Type::StructTyID: {
    StructType *STy = cast<StructType>(Ty);
    if (!structWork_run)
        structWork.push_back(STy);
    Out << "struct " << getStructName(STy) << " ";
    if (!IgnoreName || (!strncmp(getStructName(STy).c_str(), "l_", 2) && getStructName(STy) != NameSoFar)) {
      Out << NameSoFar;
      break;
    }
    Out << "{\n";
    unsigned Idx = 0;
    for (StructType::element_iterator I = STy->element_begin(), E = STy->element_end(); I != E; ++I) {
      printType(Out, *I, false, fieldName(STy, Idx++), false, false, "  ", ";\n");
    }
    Out << "} ";
    if (!isStatic)
        Out << NameSoFar;
    break;
  }
  case Type::ArrayTyID: {
    ArrayType *ATy = cast<ArrayType>(Ty);
    unsigned NumElements = ATy->getNumElements();
    if (NumElements == 0) NumElements = 1;
    Out << "struct " << NameSoFar << " ";
    if (!IgnoreName) {
      Out << NameSoFar;
      break;
    }
    printType(Out, ATy->getElementType(), false, "array[" + utostr(NumElements) + "]", false, false, "{ ", "; } ");
    if (!isStatic)
        Out << NameSoFar;
    break;
  }
  case Type::PointerTyID: {
    PointerType *PTy = cast<PointerType>(Ty);
    std::string ptrName = "*" + NameSoFar;
    if (PTy->getElementType()->isArrayTy() || PTy->getElementType()->isVectorTy())
      ptrName = "(" + ptrName + ")";
    printType(Out, PTy->getElementType(), false, ptrName, false, false, "", "");
    break;
  }
  default:
    llvm_unreachable("Unhandled case in getTypeProps!");
  }
  Out << postfix;
}

void CWriter::printString(const char *cp, int len)
{
    if (!cp[len-1])
        len--;
    Out << '\"';
    bool LastWasHex = false;
    for (unsigned i = 0, e = len; i != e; ++i) {
      unsigned char C = cp[i];
      if (isprint(C) && (!LastWasHex || !isxdigit(C))) {
        LastWasHex = false;
        if (C == '"' || C == '\\')
          Out << "\\" << (char)C;
        else
          Out << (char)C;
      } else {
        LastWasHex = false;
        switch (C) {
        case '\n': Out << "\\n"; break;
        case '\t': Out << "\\t"; break;
        case '\r': Out << "\\r"; break;
        case '\v': Out << "\\v"; break;
        case '\a': Out << "\\a"; break;
        case '\"': Out << "\\\""; break;
        case '\'': Out << "\\\'"; break;
        default:
          Out << "\\x";
          Out << (char)(( C/16  < 10) ? ( C/16 +'0') : ( C/16 -10+'A'));
          Out << (char)(((C&15) < 10) ? ((C&15)+'0') : ((C&15)-10+'A'));
          LastWasHex = true;
          break;
        }
      }
    }
    Out << '\"';
}

void CWriter::printConstantArray(ConstantArray *CPA, bool Static)
{
  int len = CPA->getNumOperands();
  Type *ETy = CPA->getType()->getElementType();
  bool isString = (ETy == Type::getInt8Ty(CPA->getContext()) ||
                   ETy == Type::getInt8Ty(CPA->getContext()));
  if (isString && (CPA->getNumOperands() == 0 ||
                   !cast<Constant>(*(CPA->op_end()-1))->isNullValue()))
    isString = false;
  if (isString) {
    char *cp = (char *)malloc(len);
    for (unsigned i = 0, e = CPA->getNumOperands()-1; i != e; ++i)
        cp[i] = cast<ConstantInt>(CPA->getOperand(i))->getZExtValue();
    printString(cp, len);
    free(cp);
  } else {
    Out << '{';
    if (CPA->getNumOperands()) {
      Out << ' ';
      printConstant(cast<Constant>(CPA->getOperand(0)), Static);
      for (unsigned i = 1, e = CPA->getNumOperands(); i != e; ++i) {
        Out << ", ";
        printConstant(cast<Constant>(CPA->getOperand(i)), Static);
      }
    }
    Out << " }";
  }
}
void CWriter::printConstantDataArray(ConstantDataArray *CPA, bool Static)
{
  if (CPA->isString()) {
    StringRef value = CPA->getAsString();
    const char *cp = value.str().c_str();
    printString(cp, value.str().length());
  } else {
    Out << '{';
    if (CPA->getNumOperands()) {
      Out << ' ';
      printConstant(cast<Constant>(CPA->getOperand(0)), Static);
      for (unsigned i = 1, e = CPA->getNumOperands(); i != e; ++i) {
        Out << ", ";
        printConstant(cast<Constant>(CPA->getOperand(i)), Static);
      }
    }
    Out << " }";
  }
}
void CWriter::printConstantVector(ConstantVector *CP, bool Static)
{
  Out << '{';
  if (CP->getNumOperands()) {
    Out << ' ';
    printConstant(cast<Constant>(CP->getOperand(0)), Static);
    for (unsigned i = 1, e = CP->getNumOperands(); i != e; ++i) {
      Out << ", ";
      printConstant(cast<Constant>(CP->getOperand(i)), Static);
    }
  }
  Out << " }";
}
void CWriter::printCast(unsigned opc, Type *SrcTy, Type *DstTy)
{
  bool TypeIsSigned = false;
  switch (opc) {
    case Instruction::UIToFP: case Instruction::SIToFP: case Instruction::IntToPtr:
    case Instruction::Trunc: case Instruction::BitCast: case Instruction::FPExt:
    case Instruction::FPTrunc: // For these the DstTy sign doesn't matter
      break;
    case Instruction::ZExt: case Instruction::PtrToInt:
    case Instruction::FPToUI: // For these, make sure we get an unsigned dest
      break;
    case Instruction::SExt:
    case Instruction::FPToSI: // For these, make sure we get a signed dest
      TypeIsSigned = true;
      break;
    default:
      llvm_unreachable("Invalid cast opcode");
  }
  printType(Out, DstTy, TypeIsSigned, "", false, false, "(", ")");
  switch (opc) {
    case Instruction::Trunc: case Instruction::BitCast: case Instruction::FPExt:
    case Instruction::FPTrunc: case Instruction::FPToSI: case Instruction::FPToUI:
      return; // These don't need a source cast.
  }
  switch (opc) {
    case Instruction::IntToPtr: case Instruction::PtrToInt:
      Out << "(unsigned long)";
      return;
  }
  printType(Out, SrcTy, TypeIsSigned, "", false, false, "(", ")");
}
void CWriter::printConstant(Constant *CPV, bool Static)
{
  /* handle expressions */
  if (const ConstantExpr *CE = dyn_cast<ConstantExpr>(CPV)) {
    Out << "(";
    switch (CE->getOpcode()) {
    case Instruction::Trunc: case Instruction::ZExt: case Instruction::SExt:
    case Instruction::FPTrunc: case Instruction::FPExt: case Instruction::UIToFP:
    case Instruction::SIToFP: case Instruction::FPToUI: case Instruction::FPToSI:
    case Instruction::PtrToInt: case Instruction::IntToPtr: case Instruction::BitCast:
      printCast(CE->getOpcode(), CE->getOperand(0)->getType(), CE->getType());
      if (CE->getOpcode() == Instruction::SExt &&
          CE->getOperand(0)->getType() == Type::getInt1Ty(CPV->getContext())) {
        Out << "0-";
      }
      printConstant(CE->getOperand(0), Static);
      if (CE->getType() == Type::getInt1Ty(CPV->getContext()) &&
          (CE->getOpcode() == Instruction::Trunc ||
           CE->getOpcode() == Instruction::FPToUI ||
           CE->getOpcode() == Instruction::FPToSI ||
           CE->getOpcode() == Instruction::PtrToInt)) {
        Out << "&1u";
      }
      break;
    case Instruction::GetElementPtr:
      printGEPExpression(CE->getOperand(0), gep_type_begin(CPV), gep_type_end(CPV), Static);
      break;
    case Instruction::Select:
      printConstant(CE->getOperand(0), Static);
      Out << '?';
      printConstant(CE->getOperand(1), Static);
      Out << ':';
      printConstant(CE->getOperand(2), Static);
      break;
    case Instruction::Add: case Instruction::FAdd: case Instruction::Sub:
    case Instruction::FSub: case Instruction::Mul: case Instruction::FMul:
    case Instruction::SDiv: case Instruction::UDiv: case Instruction::FDiv:
    case Instruction::URem: case Instruction::SRem: case Instruction::FRem:
    case Instruction::And: case Instruction::Or: case Instruction::Xor:
    case Instruction::ICmp: case Instruction::Shl: case Instruction::LShr:
    case Instruction::AShr:
    {
      printConstantWithCast(CE->getOperand(0), CE->getOpcode());
      Out << " ";
      if (CE->getOpcode() == Instruction::ICmp)
        Out << intmap_lookup(predText, CE->getPredicate());
      else
        Out << intmap_lookup(opcodeMap, CE->getOpcode());
      Out << " ";
      printConstantWithCast(CE->getOperand(1), CE->getOpcode());
      printConstExprCast(CE, Static);
      break;
    }
    case Instruction::FCmp: {
      if (CE->getPredicate() == FCmpInst::FCMP_FALSE)
        Out << "0";
      else if (CE->getPredicate() == FCmpInst::FCMP_TRUE)
        Out << "1";
      else {
        Out << "llvm_fcmp_" << intmap_lookup(predText, CE->getPredicate()) << "(";
        printConstantWithCast(CE->getOperand(0), CE->getOpcode());
        Out << ", ";
        printConstantWithCast(CE->getOperand(1), CE->getOpcode());
        Out << ")";
      }
      printConstExprCast(CE, Static);
      break;
    }
    default:
      errs() << "CWriter Error: Unhandled constant expression: " << *CE << "\n";
      llvm_unreachable(0);
    }
    Out << ')';
    return;
  }
  /* handle 'undefined' */
  if (isa<UndefValue>(CPV) && CPV->getType()->isSingleValueType()) {
    printType(Out, CPV->getType(), false, "", false, false, "((", ")/*UNDEF*/"); // sign doesn't matter
    if (!CPV->getType()->isVectorTy()) {
      Out << "0)";
    } else {
      Out << "{})";
    }
    return;
  }
  /* handle int */
  if (ConstantInt *CI = dyn_cast<ConstantInt>(CPV)) {
    Type* Ty = CI->getType();
    if (Ty == Type::getInt1Ty(CPV->getContext()))
      Out << (CI->getZExtValue() ? '1' : '0');
    else if (Ty == Type::getInt32Ty(CPV->getContext()))
      Out << CI->getZExtValue();// << 'u';
    else if (Ty->getPrimitiveSizeInBits() > 32)
      Out << CI->getZExtValue();// << "ull";
    else {
      printType(Out, Ty, false, "", false, false, "((", ")");
      if (CI->isMinValue(true))
        Out << CI->getZExtValue();// << 'u';
      else
        Out << CI->getSExtValue();
      Out << ')';
    }
    return;
  }
  int tid = CPV->getType()->getTypeID();
  if (tid != Type::PointerTyID && !Static)
      printType(Out, CPV->getType(), false, "", false, false, "(", ")");
  /* handle structured types */
  switch (tid) {
  case Type::ArrayTyID:
    Out << "{ "; // Arrays are wrapped in struct types.
    if (ConstantArray *CA = dyn_cast<ConstantArray>(CPV))
      printConstantArray(CA, Static);
    else if (ConstantDataArray *CA = dyn_cast<ConstantDataArray>(CPV))
      printConstantDataArray(CA, Static);
    else if(!isa<ConstantAggregateZero>(CPV) && !isa<UndefValue>(CPV)) {
      printf("[%s:%d] %d\n", __FUNCTION__, __LINE__, CPV->getValueID());
      CPV->dump();
      exit(1);
    } else {
      ArrayType *AT = cast<ArrayType>(CPV->getType());
      Out << '{';
      if (AT->getNumElements()) {
        Out << ' ';
        Constant *CZ = Constant::getNullValue(AT->getElementType());
        printConstant(CZ, Static);
        for (unsigned i = 1, e = AT->getNumElements(); i != e; ++i) {
          Out << ", ";
          printConstant(CZ, Static);
        }
      }
      Out << " }";
    }
    Out << " }"; // Arrays are wrapped in struct types.
    break;
  case Type::VectorTyID:
    if (ConstantVector *CV = dyn_cast<ConstantVector>(CPV))
      printConstantVector(CV, Static);
    else {
      assert(isa<ConstantAggregateZero>(CPV) || isa<UndefValue>(CPV));
      VectorType *VT = cast<VectorType>(CPV->getType());
      Out << "{ ";
      Constant *CZ = Constant::getNullValue(VT->getElementType());
      printConstant(CZ, Static);
      for (unsigned i = 1, e = VT->getNumElements(); i != e; ++i) {
        Out << ", ";
        printConstant(CZ, Static);
      }
      Out << " }";
    }
    break;
  case Type::StructTyID:
    Out << '{';
    if (isa<ConstantAggregateZero>(CPV) || isa<UndefValue>(CPV)) {
      StructType *ST = cast<StructType>(CPV->getType());
      if (ST->getNumElements()) {
        Out << ' ';
        printConstant(Constant::getNullValue(ST->getElementType(0)), Static);
        for (unsigned i = 1, e = ST->getNumElements(); i != e; ++i) {
          Out << ", ";
          printConstant(Constant::getNullValue(ST->getElementType(i)), Static);
        }
      }
    } else if (CPV->getNumOperands()) {
        Out << ' ';
        printConstant(cast<Constant>(CPV->getOperand(0)), Static);
        for (unsigned i = 1, e = CPV->getNumOperands(); i != e; ++i) {
          Out << ", ";
          printConstant(cast<Constant>(CPV->getOperand(i)), Static);
        }
    }
    Out << " }";
    break;
  case Type::PointerTyID:
    if (isa<ConstantPointerNull>(CPV))
      printType(Out, CPV->getType(), false, "", false, false, "((", ")/*NULL*/0)"); // sign doesn't matter
    else if (GlobalValue *GV = dyn_cast<GlobalValue>(CPV))
      writeOperand(GV, false, Static);
    break;
  default:
    errs() << "Unknown constant type: " << *CPV << "\n";
    llvm_unreachable(0);
  }
}
bool CWriter::printConstExprCast(const ConstantExpr* CE, bool Static)
{
  Type *Ty = CE->getOperand(0)->getType();
  bool TypeIsSigned = false;
  switch (CE->getOpcode()) {
  case Instruction::Add: case Instruction::Sub: case Instruction::Mul:
  case Instruction::LShr: case Instruction::URem:
  case Instruction::UDiv:
    break;
  case Instruction::AShr: case Instruction::SRem:
  case Instruction::SDiv:
    TypeIsSigned = true;
    break;
  case Instruction::SExt:
    Ty = CE->getType();
    TypeIsSigned = true;
    break;
  case Instruction::ZExt: case Instruction::Trunc: case Instruction::FPTrunc:
  case Instruction::FPExt: case Instruction::UIToFP: case Instruction::SIToFP:
  case Instruction::FPToUI: case Instruction::FPToSI: case Instruction::PtrToInt:
  case Instruction::IntToPtr: case Instruction::BitCast:
    Ty = CE->getType();
    break;
  default: return false;
  }
  if (!Ty->isIntegerTy() || Ty == Type::getInt1Ty(Ty->getContext()))
      TypeIsSigned = false; // not integer, sign doesn't matter
  printType(Out, Ty, TypeIsSigned, "", false, false, "((", ")(");
  //Out << ")(";
  Out << "))";
  return true;
}
void CWriter::printConstantWithCast(Constant* CPV, unsigned Opcode)
{
  Type* OpTy = CPV->getType();
  bool typeIsSigned = false;
  switch (Opcode) {
    default:
      printConstant(CPV, false);
      return;
    case Instruction::Add: case Instruction::Sub: case Instruction::Mul:
    case Instruction::LShr: case Instruction::UDiv: case Instruction::URem:
      break;
    case Instruction::AShr: case Instruction::SDiv: case Instruction::SRem:
      typeIsSigned = true;
      break;
  }
  printType(Out, OpTy, typeIsSigned, "", false, false, "((", ")");
  printConstant(CPV, false);
  Out << ")";
}
std::string CWriter::GetValueName(const Value *Operand)
{
  if (const GlobalAlias *GA = dyn_cast<GlobalAlias>(Operand)) {
    if (const Value *V = GA->resolveAliasedGlobal(false))
      Operand = V;
  }
  if (const GlobalValue *GV = dyn_cast<GlobalValue>(Operand)) {
    SmallString<128> Str;
    Str = GV->getName();
    //Mang->getNameWithPrefix(Str, GV, false);
    return CBEMangle(Str.str().str());
  }
  std::string Name = Operand->getName();
  if (Name.empty()) { // Assign unique names to local temporaries.
    unsigned &No = AnonValueNumbers[Operand];
    if (No == 0)
      No = ++NextAnonValueNumber;
    Name = "tmp__" + utostr(No);
  }
  std::string VarName;
  VarName.reserve(Name.capacity());
  for (std::string::iterator I = Name.begin(), E = Name.end(); I != E; ++I) {
    char ch = *I;
    if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
          (ch >= '0' && ch <= '9') || ch == '_')) {
      char buffer[5];
      sprintf(buffer, "_%x_", ch);
      VarName += buffer;
    } else
      VarName += ch;
  }
  return "V" + VarName;
}
void CWriter::writeInstComputationInline(Instruction &I)
{
  Type *Ty = I.getType();
  if (Ty->isIntegerTy() && (Ty!=Type::getInt1Ty(I.getContext()) &&
        Ty!=Type::getInt8Ty(I.getContext()) && Ty!=Type::getInt16Ty(I.getContext()) &&
        Ty!=Type::getInt32Ty(I.getContext()) && Ty!=Type::getInt64Ty(I.getContext()))) {
      report_fatal_error("The C backend does not currently support integer "
           "types of widths other than 1, 8, 16, 32, 64.\n" "This is being tracked as PR 4158.");
  }
  bool NeedBoolTrunc = false;
  if (I.getType() == Type::getInt1Ty(I.getContext()) && !isa<ICmpInst>(I) && !isa<FCmpInst>(I))
    NeedBoolTrunc = true;
  if (NeedBoolTrunc)
    Out << "((";
  visit(I);
  if (NeedBoolTrunc)
    Out << ")&1)";
}
void CWriter::writeOperand(Value *Operand, bool Indirect, bool Static)
{
  Instruction *I = dyn_cast<Instruction>(Operand);
  bool isAddressImplicit = isAddressExposed(Operand);
  if (Indirect) {
    if (isAddressImplicit)
      isAddressImplicit = false;
    else
      Out << '*';
  }
  if (isAddressImplicit)
    Out << "(&";  // Global variables are referenced as their addresses by llvm
  if (I && isInlinableInst(*I) && !isDirectAlloca(I)) {
      Out << '(';
      writeInstComputationInline(*I);
      Out << ')';
  }
  else {
      Constant* CPV = dyn_cast<Constant>(Operand);
      if (CPV && !isa<GlobalValue>(CPV))
        printConstant(CPV, Static);
      else
        Out << GetValueName(Operand);
  }
  if (isAddressImplicit)
    Out << ')';
}
bool CWriter::writeInstructionCast(const Instruction &I)
{
  Type *Ty = I.getOperand(0)->getType();
  bool typeIsSigned = false;
  switch (I.getOpcode()) {
  case Instruction::Add: case Instruction::Sub: case Instruction::Mul:
  case Instruction::LShr: case Instruction::URem: case Instruction::UDiv:
    break;
  case Instruction::AShr: case Instruction::SRem: case Instruction::SDiv:
    typeIsSigned = true;
    break;
  default:
    return false;
  }
  printType(Out, Ty, typeIsSigned, "", false, false, "((", ")(");
  return true;
}
void CWriter::writeOperandWithCast(Value* Operand, unsigned Opcode)
{
  Type* OpTy = Operand->getType();
  bool castIsSigned = false;
  switch (Opcode) {
    default:
      writeOperand(Operand, false);
      return;
    case Instruction::Add: case Instruction::Sub: case Instruction::Mul:
    case Instruction::LShr: case Instruction::UDiv:
    case Instruction::URem: // Cast to unsigned first
      break;
    case Instruction::GetElementPtr: case Instruction::AShr: case Instruction::SDiv:
    case Instruction::SRem: // Cast to signed first
      castIsSigned = true;
      break;
  }
  printType(Out, OpTy, castIsSigned, "", false, false, "((", ")");
  writeOperand(Operand, false);
  Out << ")";
}
void CWriter::writeOperandWithCast(Value* Operand, const ICmpInst &Cmp)
{
  bool shouldCast = Cmp.isRelational();
  if (shouldCast) {
      Type* OpTy = Operand->getType();
      if (OpTy->isPointerTy()) {
        printf("[%s:%d]\n", __FUNCTION__, __LINE__);
        exit(1);
        //OpTy = TD->getIntPtrType(Operand->getContext());
      }
      printType(Out, OpTy, Cmp.isSigned(), "", false, false, "((", ")");
  }
  writeOperand(Operand, false);
  if (shouldCast)
      Out << ")";
}
static SpecialGlobalClass getGlobalVariableClass(const GlobalVariable *GV)
{
  if (GV->hasAppendingLinkage() && GV->use_empty()) {
    if (GV->getName() == "llvm.global_ctors")
      return GlobalCtors;
    else if (GV->getName() == "llvm.global_dtors")
      return GlobalDtors;
  }
  if (GV->getSection() == "llvm.metadata")
    return NotPrinted;
  return NotSpecial;
}
bool CWriter::doInitialization(Module &M)
{
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
  FunctionPass::doInitialization(M);
  if (!M.global_empty()) {
    Out << "\n\n/* Global Variable Definitions and Initialization */\n";
    for (Module::global_iterator I = M.global_begin(), E = M.global_end(); I != E; ++I)
      if (!I->isDeclaration()) {
        if (I->hasWeakLinkage() || I->hasDLLImportLinkage() || I->hasDLLExportLinkage() || I->isThreadLocal() || I->hasHiddenVisibility()) {
            printf("[%s:%d]\n", __FUNCTION__, __LINE__);
            exit(1);
        }
        if (getGlobalVariableClass(I))
          continue;
        Type *Ty = I->getType()->getElementType();
        if (Ty->getTypeID() == Type::ArrayTyID)
            if (ArrayType *ATy = cast<ArrayType>(Ty))
                if (ATy->getElementType()->getTypeID() == Type::PointerTyID)
                    continue;
        if (I->hasLocalLinkage())
          Out << "static ";
        printType(Out, Ty, false, GetValueName(I), false, false, "", "");
        if (!I->getInitializer()->isNullValue()) {
          Out << " = " ;
          writeOperand(I->getInitializer(), false, true);
        }
        Out << ";\n";
      }
    Out << "//vtables for Classes\n";
    for (Module::global_iterator I = M.global_begin(), E = M.global_end(); I != E; ++I)
      if (!I->isDeclaration()) {
        if (getGlobalVariableClass(I))
          continue;
        Type *Ty = I->getType()->getElementType();
        if (Ty->getTypeID() == Type::ArrayTyID)
            if (ArrayType *ATy = cast<ArrayType>(Ty)) {
                if (ATy->getElementType()->getTypeID() == Type::PointerTyID) {
                    if (I->hasLocalLinkage())
                      Out << "static ";
                    printType(Out, Ty, false, GetValueName(I), false, false, "", "");
                    if (!I->getInitializer()->isNullValue()) {
                      Out << " = " ;
                      writeOperand(I->getInitializer(), false, true);
                    }
                    Out << ";\n";
                }
            }
      }
  }
  return false;
}
const char *CWriter::fieldName(StructType *STy, uint64_t ind)
{
    static char temp[MAX_CHAR_BUFFER];
    if (STy->isLiteral()) { // unnamed items
        sprintf(temp, "field%d", (int)ind);
        return temp;
    }
    CLASS_META *classp = lookup_class(STy->getName().str().c_str());
    if (!classp) {
        printf("[%s:%d] class not found!!!\n", __FUNCTION__, __LINE__);
        exit(1);
    }
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
    sprintf(temp, "field%d", (int)ind);
    return temp;
}
void CWriter::printContainedStructs(Type *Ty)
{
    if (Ty->isPointerTy()) {
        PointerType *PTy = cast<PointerType>(Ty);
        printContainedStructs(PTy->getElementType());
        return;
    }
    if (Ty->isPrimitiveType() || Ty->isIntegerTy())
        return;
    std::map<Type *, int>::iterator FI = structMap.find(Ty);
    if (FI != structMap.end())
        return;
    structMap[Ty] = 1;
    for (Type::subtype_iterator I = Ty->subtype_begin(), E = Ty->subtype_end(); I != E; ++I)
        printContainedStructs(*I);
    if (StructType *STy = dyn_cast<StructType>(Ty)) {
        std::string Name = getStructName(STy);
        printType(OutHeader, STy, false, Name, true, false, "typedef ", ";\n\n");
    }
}
bool CWriter::doFinalization(Module &M)
{
    structWork_run = 1;
    while (structWork.begin() != structWork.end()) {
        printContainedStructs(*structWork.begin());
        structWork.pop_front();
    }
    if (!M.global_empty()) {
      OutHeader << "\n/* External Global Variable Declarations */\n";
      for (Module::global_iterator I = M.global_begin(), E = M.global_end(); I != E; ++I) {
        if (I->hasDLLImportLinkage() || I->hasExternalWeakLinkage() || I->isThreadLocal() || I->hasHiddenVisibility()) {
            printf("[%s:%d]\n", __FUNCTION__, __LINE__);
            exit(1);
        }
        if (I->hasExternalLinkage() || I->hasCommonLinkage())
          printType(OutHeader, I->getType()->getElementType(), false, GetValueName(I), false, false, "extern ", ";\n");
        if (!I->isDeclaration()) {
          Type *Ty = I->getType()->getElementType();
          if (!getGlobalVariableClass(I) && Ty->getTypeID() == Type::ArrayTyID)
              printType(OutHeader, Ty, false, GetValueName(I), true, true, "", ";\n");
        }
      }
    }
    OutHeader << "\n/* Function Declarations */\n";
    SmallVector<const Function*, 8> intrinsicsToDefine;
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
      if (I->hasExternalWeakLinkage() || I->hasHiddenVisibility() || (I->hasName() && I->getName()[0] == 1)) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      if (I->isIntrinsic()) {
        switch (I->getIntrinsicID()) {
          default:
            break;
          case Intrinsic::uadd_with_overflow: case Intrinsic::sadd_with_overflow:
            intrinsicsToDefine.push_back(I);
            break;
        }
        continue;
      }
      if (I->getName() == "main" || I->getName() == "atexit")
        continue;
      if (I->getName() == "printf" || I->getName() == "__cxa_pure_virtual")
        continue;
      if (I->getName() == "setjmp" || I->getName() == "longjmp" || I->getName() == "_setjmp")
        continue;
      printFunctionSignature(OutHeader, I, true);
      OutHeader << ";\n";
    }
    UnnamedStructIDs.clear();
    return false;
}
void CWriter::printFunctionSignature(raw_ostream &Out, const Function *F, bool Prototype)
{
  if (F->hasLocalLinkage()) Out << "static ";
  FunctionType *FT = cast<FunctionType>(F->getFunctionType());
  if (F->hasDLLImportLinkage() || F->hasDLLExportLinkage()
   || F->hasStructRetAttr() || FT->isVarArg()) {
    printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    exit(1);
  }
  std::string tstr;
  raw_string_ostream FunctionInnards(tstr);
  FunctionInnards << GetValueName(F) << '(';
  bool PrintedArg = false;
  if (!F->isDeclaration()) {
    if (!F->arg_empty()) {
      std::string ArgName;
      for (Function::const_arg_iterator I = F->arg_begin(), E = F->arg_end(); I != E; ++I) {
        if (PrintedArg) FunctionInnards << ", ";
        if (I->hasName() || !Prototype)
          ArgName = GetValueName(I);
        else
          ArgName = "";
        Type *ArgTy = I->getType();
        printType(FunctionInnards, ArgTy, /*isSigned=*/false, ArgName, false, false, "", "");
        PrintedArg = true;
      }
    }
  } else {
    for (FunctionType::param_iterator I = FT->param_begin(), E = FT->param_end(); I != E; ++I) {
      if (PrintedArg)
          FunctionInnards << ", ";
      printType(FunctionInnards, *I, /*isSigned=*/false, "", false, false, "", "");
      PrintedArg = true;
    }
  }
  if (!PrintedArg)
    FunctionInnards << "void"; // ret() -> ret(void) in C.
  FunctionInnards << ')';
  Type *RetTy = F->getReturnType();
  printType(Out, RetTy, /*isSigned=*/false, FunctionInnards.str(), false, false, "", "");
}
void CWriter::printFunction(Function &F)
{
  printFunctionSignature(Out, &F, false);
  Out << " {\n";
  for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB)
      printBasicBlock(BB);
  Out << "}\n\n";
}
void CWriter::printBasicBlock(BasicBlock *BB)
{
  //Out << GetValueName(BB) << ":\n";
  for (BasicBlock::iterator II = BB->begin(), E = --BB->end(); II != E; ++II) {
    if (const AllocaInst *AI = isDirectAlloca(&*II)) {
      Out << "    ";
      printType(Out, AI->getAllocatedType(), false, GetValueName(AI), false, false, "", ";    /* Address-exposed local */\n");
    } else if (!isInlinableInst(*II)) {
      Out << "    ";
      if (II->getType() != Type::getVoidTy(BB->getContext()))
          printType(Out, II->getType(), false, GetValueName(&*II), false, false, "", " = ");
      if (isa<PHINode>(*II)) {    // Print out PHI node temporaries as well...
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      writeInstComputationInline(*II);
      Out << ";\n";
    }
  }
  visit(*BB->getTerminator());
}
void CWriter::visitReturnInst(ReturnInst &I)
{
  if (I.getNumOperands() != 0 || I.getParent()->getParent()->size() != 1) {
    Out << "  return ";
    if (I.getNumOperands())
      writeOperand(I.getOperand(0), false);
    Out << ";\n";
  }
}
void CWriter::visitBinaryOperator(Instruction &I)
{
  assert(!I.getType()->isPointerTy());
  bool needsCast = false;
  if ((I.getType() == Type::getInt8Ty(I.getContext())) ||
      (I.getType() == Type::getInt16Ty(I.getContext()))
      || (I.getType() == Type::getFloatTy(I.getContext()))) {
    needsCast = true;
    printType(Out, I.getType(), false, "", false, false, "((", ")(");
  }
  if (BinaryOperator::isNeg(&I)) {
    Out << "-(";
    writeOperand(BinaryOperator::getNegArgument(cast<BinaryOperator>(&I)), false);
    Out << ")";
  } else if (BinaryOperator::isFNeg(&I)) {
    Out << "-(";
    writeOperand(BinaryOperator::getFNegArgument(cast<BinaryOperator>(&I)), false);
    Out << ")";
  } else if (I.getOpcode() == Instruction::FRem) {
    if (I.getType() == Type::getFloatTy(I.getContext()))
      Out << "fmodf(";
    else if (I.getType() == Type::getDoubleTy(I.getContext()))
      Out << "fmod(";
    else  // all 3 flavors of long double
      Out << "fmodl(";
    writeOperand(I.getOperand(0), false);
    Out << ", ";
    writeOperand(I.getOperand(1), false);
    Out << ")";
  } else {
    writeOperandWithCast(I.getOperand(0), I.getOpcode());
    Out << " ";
    Out << intmap_lookup(opcodeMap, I.getOpcode());
    Out << " ";
    writeOperandWithCast(I.getOperand(1), I.getOpcode());
    if (writeInstructionCast(I))
      Out << "))";
  }
  if (needsCast) {
    Out << "))";
  }
}
void CWriter::visitICmpInst(ICmpInst &I)
{
  bool needsCast = false;
  writeOperandWithCast(I.getOperand(0), I);
  Out << " ";
  Out << intmap_lookup(predText, I.getPredicate());
  Out << " ";
  writeOperandWithCast(I.getOperand(1), I);
  if (writeInstructionCast(I))
    Out << "))";
  if (needsCast) {
    Out << "))";
  }
}
void CWriter::visitCastInst(CastInst &I)
{
  Type *DstTy = I.getType();
  Type *SrcTy = I.getOperand(0)->getType();
  Out << '(';
  printCast(I.getOpcode(), SrcTy, DstTy);
  if (SrcTy == Type::getInt1Ty(I.getContext()) &&
      I.getOpcode() == Instruction::SExt)
    Out << "0-";
  writeOperand(I.getOperand(0), false);
  if (DstTy == Type::getInt1Ty(I.getContext()) &&
      (I.getOpcode() == Instruction::Trunc || I.getOpcode() == Instruction::FPToUI ||
       I.getOpcode() == Instruction::FPToSI || I.getOpcode() == Instruction::PtrToInt)) {
    Out << "&1u";
  }
  Out << ')';
}
void CWriter::visitCallInst(CallInst &I)
{
  bool WroteCallee = false;
  if (Function *F = I.getCalledFunction())
      if (Intrinsic::ID ID = (Intrinsic::ID)F->getIntrinsicID()) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
  Value *Callee = I.getCalledValue();
  PointerType  *PTy   = cast<PointerType>(Callee->getType());
  FunctionType *FTy   = cast<FunctionType>(PTy->getElementType());
  bool hasByVal = I.hasByValArgument();
  if (I.hasStructRetAttr()) {
      printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      exit(1);
  }
  if (I.isTailCall()) Out << " /*tail*/ ";
  if (!WroteCallee) {
    bool NeedsCast = (hasByVal ) && !isa<Function>(Callee);
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(Callee))
      if (CE->isCast())
        if (Function *RF = dyn_cast<Function>(CE->getOperand(0))) {
          NeedsCast = true;
          Callee = RF;
        }
    if (NeedsCast)
      printType(Out, I.getCalledValue()->getType(), false, "", hasByVal, false, "((", ")(void*)");
    writeOperand(Callee, false);
    if (NeedsCast) Out << ')';
  }
  Out << '(';
  if (Callee->getName() == "printf")
      Out << "(const char *) ";
  bool PrintedArg = false;
  if(FTy->isVarArg() && !FTy->getNumParams()) {
    printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    exit(1);
  }
  unsigned NumDeclaredParams = FTy->getNumParams();
  CallSite CS(&I);
  CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();
  unsigned ArgNo = 0;
  for (; AI != AE; ++AI, ++ArgNo) {
    if (PrintedArg) Out << ", ";
    if (ArgNo < NumDeclaredParams && (*AI)->getType() != FTy->getParamType(ArgNo))
        printType(Out, FTy->getParamType(ArgNo), /*isSigned=*/false, "", false, false, "(", ")");
    writeOperand(*AI, false);
    PrintedArg = true;
  }
  Out << ')';
}
void CWriter::printGEPExpression(Value *Ptr, gep_type_iterator I, gep_type_iterator E, bool Static)
{
  if (I == E) {
    writeOperand(Ptr, false);
    return;
  }
  VectorType *LastIndexIsVector = 0;
  for (gep_type_iterator TmpI = I; TmpI != E; ++TmpI)
      LastIndexIsVector = dyn_cast<VectorType>(*TmpI);
  Out << "(";
  if (LastIndexIsVector) {
    printType(Out, PointerType::getUnqual(LastIndexIsVector->getElementType()), false, "", false, false, "((", ")(");
  }
  Out << '&';
  Value *FirstOp = I.getOperand();
  if (!isa<Constant>(FirstOp) || !cast<Constant>(FirstOp)->isNullValue()) {
    writeOperand(Ptr, false);
  } else {
    ++I;  // Skip the zero index.
    if (isAddressExposed(Ptr)) {
      writeOperand(Ptr, true, Static);
    } else if (I != E && (*I)->isStructTy()) {
      writeOperand(Ptr, false);
      StructType *STy = dyn_cast<StructType>(*I);
      Out << "->" << fieldName(STy, cast<ConstantInt>(I.getOperand())->getZExtValue());
      ++I;  // eat the struct index as well.
    } else {
      Out << "(";
      writeOperand(Ptr, true);
      Out << ")";
    }
  }
  for (; I != E; ++I) {
    if ((*I)->isStructTy()) {
      StructType *STy = dyn_cast<StructType>(*I);
      Out << "." << fieldName(STy, cast<ConstantInt>(I.getOperand())->getZExtValue());
    } else if ((*I)->isArrayTy()) {
      Out << ".array[";
      writeOperand(I.getOperand(), false);
      Out << ']';
    } else if (!(*I)->isVectorTy()) {
      Out << '[';
      writeOperand(I.getOperand(), false);
      Out << ']';
    } else {
      if (!isa<Constant>(I.getOperand()) || !cast<Constant>(I.getOperand())->isNullValue()) {
        Out << ")+(";
        writeOperandWithCast(I.getOperand(), Instruction::GetElementPtr);
      }
      Out << "))";
    }
  }
  Out << ")";
}
void CWriter::writeMemoryAccess(Value *Operand, Type *OperandType, bool IsVolatile, unsigned Alignment)
{
  if (IsVolatile) {
    printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    Operand->dump();
    exit(1);
  }
  writeOperand(Operand, true);
}
void CWriter::visitLoadInst(LoadInst &I)
{
  writeMemoryAccess(I.getOperand(0), I.getType(), I.isVolatile(), I.getAlignment());
}
void CWriter::visitStoreInst(StoreInst &I)
{
  writeMemoryAccess(I.getPointerOperand(), I.getOperand(0)->getType(), I.isVolatile(), I.getAlignment());
  Out << " = ";
  Value *Operand = I.getOperand(0);
  Constant *BitMask = 0;
  if (IntegerType* ITy = dyn_cast<IntegerType>(Operand->getType()))
    if (!ITy->isPowerOf2ByteWidth())
      BitMask = ConstantInt::get(ITy, ITy->getBitMask());
  if (BitMask)
    Out << "((";
  writeOperand(Operand, false);
  if (BitMask) {
    Out << ") & ";
    printConstant(BitMask, false);
    Out << ")";
  }
}
void CWriter::visitGetElementPtrInst(GetElementPtrInst &I)
{
  printGEPExpression(I.getPointerOperand(), gep_type_begin(I), gep_type_end(I), false);
}
