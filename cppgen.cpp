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

static int printout_initialization = 1;

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
//printf("[%s:%d] in %s out %s\n", __FUNCTION__, __LINE__, S.c_str(), Result.c_str());
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
raw_ostream & CWriter::printSimpleType(raw_ostream &Out, Type *Ty, bool isSigned, const std::string &NameSoFar)
{
  assert((Ty->isPrimitiveType() || Ty->isIntegerTy() || Ty->isVectorTy()) &&
         "Invalid type for printSimpleType");
  switch (Ty->getTypeID()) {
  case Type::VoidTyID:   return Out << "void " << NameSoFar;
  case Type::IntegerTyID: {
    unsigned NumBits = cast<IntegerType>(Ty)->getBitWidth();
    if (NumBits == 1)
      return Out << "bool " << NameSoFar;
    else if (NumBits <= 8)
      return Out << (isSigned?"signed":"unsigned") << " char " << NameSoFar;
    else if (NumBits <= 16)
      return Out << (isSigned?"signed":"unsigned") << " short " << NameSoFar;
    else if (NumBits <= 32)
      return Out << (isSigned?"signed":"unsigned") << " int " << NameSoFar;
    else if (NumBits <= 64)
      return Out << (isSigned?"signed":"unsigned") << " long long "<< NameSoFar;
    else {
      assert(NumBits <= 128 && "Bit widths > 128 not implemented yet");
      return Out << (isSigned?"llvmInt128":"llvmUInt128") << " " << NameSoFar;
    }
  }
  case Type::VectorTyID: {
    VectorType *VTy = cast<VectorType>(Ty);
    return printSimpleType(Out, VTy->getElementType(), isSigned, " __attribute__((vector_size(" "utostr(TD->getTypeAllocSize(VTy))" " ))) " + NameSoFar);
  }
  case Type::MetadataTyID: {
      return Out << "MetadataTyIDSTUB\n";
  }
  default:
    errs() << "Unknown primitive type: " << *Ty << "\n";
    llvm_unreachable(0);
  }
}
void CWriter::printType(raw_ostream &Out, Type *Ty, bool isSigned, const std::string &NameSoFar, bool IgnoreName, bool isStatic)
{
  if (Ty->isPrimitiveType() || Ty->isIntegerTy() || Ty->isVectorTy()) {
    printSimpleType(Out, Ty, isSigned, NameSoFar);
    return;
  }
  switch (Ty->getTypeID()) {
  case Type::FunctionTyID: {
    FunctionType *FTy = cast<FunctionType>(Ty);
    std::string tstr;
    raw_string_ostream FunctionInnards(tstr);
    FunctionInnards << " (" << NameSoFar << ") (";
    unsigned Idx = 1;
    for (FunctionType::param_iterator I = FTy->param_begin(), E = FTy->param_end(); I != E; ++I) {
      Type *ArgTy = *I;
      //if (PAL.paramHasAttr(Idx, Attribute::ByVal)) {
        //assert(ArgTy->isPointerTy());
        //ArgTy = cast<PointerType>(ArgTy)->getElementType();
      //}
      if (I != FTy->param_begin())
        FunctionInnards << ", ";
      printType(FunctionInnards, ArgTy, /*isSigned=*/false/*PAL.paramHasAttr(Idx, Attribute::SExt)*/, "", false, 0);
      ++Idx;
    }
    if (FTy->isVarArg()) {
      if (!FTy->getNumParams())
        FunctionInnards << " int"; //dummy argument for empty vaarg functs
      FunctionInnards << ", ...";
    } else if (!FTy->getNumParams()) {
      FunctionInnards << "void";
    }
    FunctionInnards << ')';
    printType(Out, FTy->getReturnType(), /*isSigned=*/false/*PAL.paramHasAttr(0, Attribute::SExt)*/, FunctionInnards.str(), false, 0);
    break;
  }
  case Type::StructTyID: {
    StructType *STy = cast<StructType>(Ty);
    if (!structWork_run)
        structWork.push_back(STy);
    if (!IgnoreName || (!strncmp(getStructName(STy).c_str(), "l_", 2) && getStructName(STy) != NameSoFar)) {
      Out << "struct " << getStructName(STy) << ' ' << NameSoFar;
      break;
    }
    std::string lname = strdup(NameSoFar.c_str());
    Out << "struct " << getStructName(STy) << " {\n";
    unsigned Idx = 0;
    for (StructType::element_iterator I = STy->element_begin(), E = STy->element_end(); I != E; ++I) {
      Out << "  ";
      printType(Out, *I, false, "field" + utostr(Idx++), false, 0);
      Out << ";\n";
    }
    Out << "} ";
    if (isStatic)
        break;
    Out << lname;
    break;
  }
  case Type::PointerTyID: {
    PointerType *PTy = cast<PointerType>(Ty);
    std::string ptrName = "*" + NameSoFar;
    if (PTy->getElementType()->isArrayTy() || PTy->getElementType()->isVectorTy())
      ptrName = "(" + ptrName + ")";
    printType(Out, PTy->getElementType(), false, ptrName, false, 0);
    break;
  }
  case Type::ArrayTyID: {
    ArrayType *ATy = cast<ArrayType>(Ty);
    unsigned NumElements = ATy->getNumElements();
    if (NumElements == 0) NumElements = 1;
    if (!IgnoreName) {
      Out << "struct " /*<< getStructName(STy) << ' '*/ << NameSoFar << " " << NameSoFar;
      break;
    }
    std::string lname = strdup(NameSoFar.c_str());
    Out << "struct " << lname << " {\n";
    printType(Out, ATy->getElementType(), false, "array[" + utostr(NumElements) + "]", false, 0);
    Out << "; } ";
    if (isStatic)
        break;
    Out << lname;
    break;
  }
  default:
    llvm_unreachable("Unhandled case in getTypeProps!");
  }
}
void CWriter::printConstantArray(ConstantArray *CPA, bool Static)
{
  Type *ETy = CPA->getType()->getElementType();
  bool isString = (ETy == Type::getInt8Ty(CPA->getContext()) ||
                   ETy == Type::getInt8Ty(CPA->getContext()));
  if (isString && (CPA->getNumOperands() == 0 ||
                   !cast<Constant>(*(CPA->op_end()-1))->isNullValue()))
    isString = false;
  if (isString) {
    Out << '\"';
    bool LastWasHex = false;
    for (unsigned i = 0, e = CPA->getNumOperands()-1; i != e; ++i) {
      unsigned char C = cast<ConstantInt>(CPA->getOperand(i))->getZExtValue();
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
    Out << '\"';
    bool LastWasHex = false;
    int len = value.str().length();
    if (!cp[len-1])
        len--;
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
  switch (opc) {
    case Instruction::UIToFP: case Instruction::SIToFP: case Instruction::IntToPtr:
    case Instruction::Trunc: case Instruction::BitCast: case Instruction::FPExt:
    case Instruction::FPTrunc: // For these the DstTy sign doesn't matter
      Out << '(';
      printType(Out, DstTy, false, "", false, 0);
      Out << ')';
      break;
    case Instruction::ZExt: case Instruction::PtrToInt:
    case Instruction::FPToUI: // For these, make sure we get an unsigned dest
      Out << '(';
      printSimpleType(Out, DstTy, false);
      Out << ')';
      break;
    case Instruction::SExt:
    case Instruction::FPToSI: // For these, make sure we get a signed dest
      Out << '(';
      printSimpleType(Out, DstTy, true);
      Out << ')';
      break;
    default:
      llvm_unreachable("Invalid cast opcode");
  }
  switch (opc) {
    case Instruction::UIToFP: case Instruction::ZExt:
      Out << '(';
      printSimpleType(Out, SrcTy, false);
      Out << ')';
      break;
    case Instruction::SIToFP: case Instruction::SExt:
      Out << '(';
      printSimpleType(Out, SrcTy, true);
      Out << ')';
      break;
    case Instruction::IntToPtr: case Instruction::PtrToInt:
      Out << "(unsigned long)";
      break;
    case Instruction::Trunc: case Instruction::BitCast: case Instruction::FPExt:
    case Instruction::FPTrunc: case Instruction::FPToSI: case Instruction::FPToUI:
      break; // These don't need a source cast.
    default:
      llvm_unreachable("Invalid cast opcode");
      break;
  }
}
void CWriter::printConstant(Constant *CPV, bool Static)
{
  if (const ConstantExpr *CE = dyn_cast<ConstantExpr>(CPV)) {
    switch (CE->getOpcode()) {
    case Instruction::Trunc: case Instruction::ZExt: case Instruction::SExt:
    case Instruction::FPTrunc: case Instruction::FPExt: case Instruction::UIToFP:
    case Instruction::SIToFP: case Instruction::FPToUI: case Instruction::FPToSI:
    case Instruction::PtrToInt: case Instruction::IntToPtr: case Instruction::BitCast:
      Out << "(";
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
      Out << ')';
      return;
    case Instruction::GetElementPtr:
      Out << "(";
      printGEPExpression(CE->getOperand(0), gep_type_begin(CPV), gep_type_end(CPV), Static);
      Out << ")";
      return;
    case Instruction::Select:
      Out << '(';
      printConstant(CE->getOperand(0), Static);
      Out << '?';
      printConstant(CE->getOperand(1), Static);
      Out << ':';
      printConstant(CE->getOperand(2), Static);
      Out << ')';
      return;
    case Instruction::Add: case Instruction::FAdd: case Instruction::Sub:
    case Instruction::FSub: case Instruction::Mul: case Instruction::FMul:
    case Instruction::SDiv: case Instruction::UDiv: case Instruction::FDiv:
    case Instruction::URem: case Instruction::SRem: case Instruction::FRem:
    case Instruction::And: case Instruction::Or: case Instruction::Xor:
    case Instruction::ICmp: case Instruction::Shl: case Instruction::LShr:
    case Instruction::AShr:
    {
      Out << '(';
      bool NeedsClosingParens = printConstExprCast(CE, Static);
      printConstantWithCast(CE->getOperand(0), CE->getOpcode());
      switch (CE->getOpcode()) {
      case Instruction::Add: case Instruction::FAdd: Out << " + "; break;
      case Instruction::Sub: case Instruction::FSub: Out << " - "; break;
      case Instruction::Mul: case Instruction::FMul: Out << " * "; break;
      case Instruction::URem: case Instruction::SRem: case Instruction::FRem: Out << " % "; break;
      case Instruction::UDiv: case Instruction::SDiv: case Instruction::FDiv: Out << " / "; break;
      case Instruction::And: Out << " & "; break;
      case Instruction::Or:  Out << " | "; break;
      case Instruction::Xor: Out << " ^ "; break;
      case Instruction::Shl: Out << " << "; break;
      case Instruction::LShr: case Instruction::AShr: Out << " >> "; break;
      case Instruction::ICmp:
        switch (CE->getPredicate()) {
          case ICmpInst::ICMP_EQ: Out << " == "; break;
          case ICmpInst::ICMP_NE: Out << " != "; break;
          case ICmpInst::ICMP_SLT: case ICmpInst::ICMP_ULT: Out << " < "; break;
          case ICmpInst::ICMP_SLE: case ICmpInst::ICMP_ULE: Out << " <= "; break;
          case ICmpInst::ICMP_SGT: case ICmpInst::ICMP_UGT: Out << " > "; break;
          case ICmpInst::ICMP_SGE: case ICmpInst::ICMP_UGE: Out << " >= "; break;
          default: llvm_unreachable("Illegal ICmp predicate");
        }
        break;
      default: llvm_unreachable("Illegal opcode here!");
      }
      printConstantWithCast(CE->getOperand(1), CE->getOpcode());
      if (NeedsClosingParens)
        Out << "))";
      Out << ')';
      return;
    }
    case Instruction::FCmp: {
      Out << '(';
      bool NeedsClosingParens = printConstExprCast(CE, Static);
      if (CE->getPredicate() == FCmpInst::FCMP_FALSE)
        Out << "0";
      else if (CE->getPredicate() == FCmpInst::FCMP_TRUE)
        Out << "1";
      else {
        const char* op = 0;
        switch (CE->getPredicate()) {
        default: llvm_unreachable("Illegal FCmp predicate");
        case FCmpInst::FCMP_ORD: op = "ord"; break;
        case FCmpInst::FCMP_UNO: op = "uno"; break;
        case FCmpInst::FCMP_UEQ: op = "ueq"; break;
        case FCmpInst::FCMP_UNE: op = "une"; break;
        case FCmpInst::FCMP_ULT: op = "ult"; break;
        case FCmpInst::FCMP_ULE: op = "ule"; break;
        case FCmpInst::FCMP_UGT: op = "ugt"; break;
        case FCmpInst::FCMP_UGE: op = "uge"; break;
        case FCmpInst::FCMP_OEQ: op = "oeq"; break;
        case FCmpInst::FCMP_ONE: op = "one"; break;
        case FCmpInst::FCMP_OLT: op = "olt"; break;
        case FCmpInst::FCMP_OLE: op = "ole"; break;
        case FCmpInst::FCMP_OGT: op = "ogt"; break;
        case FCmpInst::FCMP_OGE: op = "oge"; break;
        }
        Out << "llvm_fcmp_" << op << "(";
        printConstantWithCast(CE->getOperand(0), CE->getOpcode());
        Out << ", ";
        printConstantWithCast(CE->getOperand(1), CE->getOpcode());
        Out << ")";
      }
      if (NeedsClosingParens)
        Out << "))";
      Out << ')';
      return;
    }
    default:
      errs() << "CWriter Error: Unhandled constant expression: " << *CE << "\n";
      llvm_unreachable(0);
    }
  } else if (isa<UndefValue>(CPV) && CPV->getType()->isSingleValueType()) {
    Out << "((";
    printType(Out, CPV->getType(), false, "", false, 0); // sign doesn't matter
    Out << ")/*UNDEF*/";
    if (!CPV->getType()->isVectorTy()) {
      Out << "0)";
    } else {
      Out << "{})";
    }
    return;
  }
  if (ConstantInt *CI = dyn_cast<ConstantInt>(CPV)) {
    Type* Ty = CI->getType();
    if (Ty == Type::getInt1Ty(CPV->getContext()))
      Out << (CI->getZExtValue() ? '1' : '0');
    else if (Ty == Type::getInt32Ty(CPV->getContext()))
      Out << CI->getZExtValue();// << 'u';
    else if (Ty->getPrimitiveSizeInBits() > 32)
      Out << CI->getZExtValue();// << "ull";
    else {
      Out << "((";
      printSimpleType(Out, Ty, false) << ')';
      if (CI->isMinValue(true))
        Out << CI->getZExtValue();// << 'u';
      else
        Out << CI->getSExtValue();
      Out << ')';
    }
    return;
  }
  switch (CPV->getType()->getTypeID()) {
  case Type::ArrayTyID:
    if (!Static) {
      Out << "(";
      printType(Out, CPV->getType(), false, "", false, 0);
      Out << ")";
    }
    Out << "{ "; // Arrays are wrapped in struct types.
    if (ConstantArray *CA = dyn_cast<ConstantArray>(CPV)) {
      printConstantArray(CA, Static);
    }
    else if (ConstantDataArray *CA = dyn_cast<ConstantDataArray>(CPV)) {
      printConstantDataArray(CA, Static);
    }
    else if(!isa<ConstantAggregateZero>(CPV) && !isa<UndefValue>(CPV)) {
      printf("[%s:%d] %d\n", __FUNCTION__, __LINE__, CPV->getValueID());
      CPV->dump();
      exit(1);
    } else {
      assert(isa<ConstantAggregateZero>(CPV) || isa<UndefValue>(CPV));
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
    if (!Static) {
      Out << "(";
      printType(Out, CPV->getType(), false, "", false, 0);
      Out << ")";
    }
    if (ConstantVector *CV = dyn_cast<ConstantVector>(CPV)) {
      printConstantVector(CV, Static);
    } else {
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
    if (!Static) {
      Out << "(";
      printType(Out, CPV->getType(), false, "", false, 0);
      Out << ")";
    }
    if (isa<ConstantAggregateZero>(CPV) || isa<UndefValue>(CPV)) {
      StructType *ST = cast<StructType>(CPV->getType());
      Out << '{';
      if (ST->getNumElements()) {
        Out << ' ';
        printConstant(Constant::getNullValue(ST->getElementType(0)), Static);
        for (unsigned i = 1, e = ST->getNumElements(); i != e; ++i) {
          Out << ", ";
          printConstant(Constant::getNullValue(ST->getElementType(i)), Static);
        }
      }
      Out << " }";
    } else {
      Out << '{';
      if (CPV->getNumOperands()) {
        Out << ' ';
        printConstant(cast<Constant>(CPV->getOperand(0)), Static);
        for (unsigned i = 1, e = CPV->getNumOperands(); i != e; ++i) {
          Out << ", ";
          printConstant(cast<Constant>(CPV->getOperand(i)), Static);
        }
      }
      Out << " }";
    }
    break;
  case Type::PointerTyID:
    if (isa<ConstantPointerNull>(CPV)) {
      Out << "((";
      printType(Out, CPV->getType(), false, "", false, 0); // sign doesn't matter
      Out << ")/*NULL*/0)";
      break;
    } else if (GlobalValue *GV = dyn_cast<GlobalValue>(CPV)) {
      writeOperand(GV, false, Static);
      break;
    }
  default:
    errs() << "Unknown constant type: " << *CPV << "\n";
    llvm_unreachable(0);
  }
}
bool CWriter::printConstExprCast(const ConstantExpr* CE, bool Static)
{
  bool NeedsExplicitCast = false;
  Type *Ty = CE->getOperand(0)->getType();
  bool TypeIsSigned = false;
  switch (CE->getOpcode()) {
  case Instruction::Add: case Instruction::Sub: case Instruction::Mul:
  case Instruction::LShr: case Instruction::URem:
  case Instruction::UDiv: NeedsExplicitCast = true; break;
  case Instruction::AShr: case Instruction::SRem:
  case Instruction::SDiv: NeedsExplicitCast = true; TypeIsSigned = true; break;
  case Instruction::SExt:
    Ty = CE->getType();
    NeedsExplicitCast = true;
    TypeIsSigned = true;
    break;
  case Instruction::ZExt: case Instruction::Trunc: case Instruction::FPTrunc:
  case Instruction::FPExt: case Instruction::UIToFP: case Instruction::SIToFP:
  case Instruction::FPToUI: case Instruction::FPToSI: case Instruction::PtrToInt:
  case Instruction::IntToPtr: case Instruction::BitCast:
    Ty = CE->getType();
    NeedsExplicitCast = true;
    break;
  default: break;
  }
  if (NeedsExplicitCast) {
    Out << "((";
    if (Ty->isIntegerTy() && Ty != Type::getInt1Ty(Ty->getContext()))
      printSimpleType(Out, Ty, TypeIsSigned);
    else
      printType(Out, Ty, false, "", false, 0); // not integer, sign doesn't matter
    Out << ")(";
  }
  return NeedsExplicitCast;
}
void CWriter::printConstantWithCast(Constant* CPV, unsigned Opcode)
{
  Type* OpTy = CPV->getType();
  bool shouldCast = false;
  bool typeIsSigned = false;
  switch (Opcode) {
    default:
      break;
    case Instruction::Add: case Instruction::Sub: case Instruction::Mul:
    case Instruction::LShr: case Instruction::UDiv: case Instruction::URem:
      shouldCast = true;
      break;
    case Instruction::AShr: case Instruction::SDiv: case Instruction::SRem:
      shouldCast = true;
      typeIsSigned = true;
      break;
  }
  if (shouldCast) {
    Out << "((";
    printSimpleType(Out, OpTy, typeIsSigned);
    Out << ")";
    printConstant(CPV, false);
    Out << ")";
  } else
    printConstant(CPV, false);
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
  //return "llvm_cbe_" + VarName;
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
void CWriter::writeOperandInternal(Value *Operand, bool Static)
{
  if (Instruction *I = dyn_cast<Instruction>(Operand))
    if (isInlinableInst(*I) && !isDirectAlloca(I)) {
      Out << '(';
      writeInstComputationInline(*I);
      Out << ')';
      return;
    }
  Constant* CPV = dyn_cast<Constant>(Operand);
  if (CPV && !isa<GlobalValue>(CPV))
    printConstant(CPV, Static);
  else
    Out << GetValueName(Operand);
}
void CWriter::writeOperand(Value *Operand, bool Indirect, bool Static)
{
  bool isAddressImplicit = isAddressExposed(Operand);
  if (Indirect) {
    if (isAddressImplicit)
      isAddressImplicit = false;
    else
      Out << '*';
  }
  if (isAddressImplicit)
    Out << "(&";  // Global variables are referenced as their addresses by llvm
  writeOperandInternal(Operand, Static);
  if (isAddressImplicit)
    Out << ')';
}
bool CWriter::writeInstructionCast(const Instruction &I)
{
  Type *Ty = I.getOperand(0)->getType();
  switch (I.getOpcode()) {
  case Instruction::Add: case Instruction::Sub: case Instruction::Mul:
  case Instruction::LShr: case Instruction::URem: case Instruction::UDiv:
    Out << "((";
    printSimpleType(Out, Ty, false);
    Out << ")(";
    return true;
  case Instruction::AShr: case Instruction::SRem: case Instruction::SDiv:
    Out << "((";
    printSimpleType(Out, Ty, true);
    Out << ")(";
    return true;
  default: break;
  }
  return false;
}
void CWriter::writeOperandWithCast(Value* Operand, unsigned Opcode)
{
  Type* OpTy = Operand->getType();
  bool shouldCast = false;
  bool castIsSigned = false;
  switch (Opcode) {
    default:
      break;
    case Instruction::Add: case Instruction::Sub: case Instruction::Mul:
    case Instruction::LShr: case Instruction::UDiv:
    case Instruction::URem: // Cast to unsigned first
      shouldCast = true;
      castIsSigned = false;
      break;
    case Instruction::GetElementPtr: case Instruction::AShr: case Instruction::SDiv:
    case Instruction::SRem: // Cast to signed first
      shouldCast = true;
      castIsSigned = true;
      break;
  }
  if (shouldCast) {
    Out << "((";
    printSimpleType(Out, OpTy, castIsSigned);
    Out << ")";
    writeOperand(Operand, false);
    Out << ")";
  } else
    writeOperand(Operand, false);
}
void CWriter::writeOperandWithCast(Value* Operand, const ICmpInst &Cmp)
{
  bool shouldCast = Cmp.isRelational();
  if (!shouldCast) {
    writeOperand(Operand, false);
    return;
  }
  bool castIsSigned = Cmp.isSigned();
  Type* OpTy = Operand->getType();
  if (OpTy->isPointerTy()) {
    printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    exit(1);
    //OpTy = TD->getIntPtrType(Operand->getContext());
  }
  Out << "((";
  printSimpleType(Out, OpTy, castIsSigned);
  Out << ")";
  writeOperand(Operand, false);
  Out << ")";
}
static void FindStaticTors(GlobalVariable *GV, std::set<Function*> &StaticTors){
  ConstantArray *InitList = dyn_cast<ConstantArray>(GV->getInitializer());
  if (!InitList) return;
  for (unsigned i = 0, e = InitList->getNumOperands(); i != e; ++i)
    if (ConstantStruct *CS = dyn_cast<ConstantStruct>(InitList->getOperand(i))){
      if (CS->getNumOperands() != 2) return;  // Not array of 2-element structs.
      if (CS->getOperand(1)->isNullValue())
        return;  // Found a null terminator, exit printing.
      Constant *FP = CS->getOperand(1);
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(FP))
        if (CE->isCast())
          FP = CE->getOperand(0);
      if (Function *F = dyn_cast<Function>(FP))
        StaticTors.insert(F);
    }
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
  std::set<Function*> StaticCtors, StaticDtors;
  for (Module::global_iterator I = M.global_begin(), E = M.global_end(); I != E; ++I) {
    switch (getGlobalVariableClass(I)) {
    default: break;
    case GlobalCtors:
      FindStaticTors(I, StaticCtors);
      break;
    case GlobalDtors:
      FindStaticTors(I, StaticDtors);
      break;
    }
  }
  if (!M.global_empty()) {
    Out << "\n/* External Global Variable Declarations */\n";
    for (Module::global_iterator I = M.global_begin(), E = M.global_end(); I != E; ++I) {
      if (I->hasExternalLinkage() || I->hasExternalWeakLinkage() ||
          I->hasCommonLinkage())
        Out << "extern ";
      else if (I->hasDLLImportLinkage())
        Out << "__declspec(dllimport) ";
      else
        continue; // Internal Global
      if (I->isThreadLocal())
        Out << "__thread ";
      printType(Out, I->getType()->getElementType(), false, GetValueName(I), false, 0);
      if (I->hasExternalWeakLinkage())
         Out << " __EXTERNAL_WEAK__";
      Out << ";\n";
    }
  }
  Out << "\n/* Function Declarations */\n";
  SmallVector<const Function*, 8> intrinsicsToDefine;
  if (printout_initialization)
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
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
    if (I->hasExternalWeakLinkage())
      Out << "extern ";
    printFunctionSignature(I, true);
    if (I->hasExternalWeakLinkage())
      Out << " __EXTERNAL_WEAK__";
    //if (StaticCtors.count(I))
      //Out << " __ATTRIBUTE_CTOR__";
    if (StaticDtors.count(I))
      Out << " __ATTRIBUTE_DTOR__";
    if (I->hasHiddenVisibility())
      Out << " __HIDDEN__";
    if (I->hasName() && I->getName()[0] == 1)
      Out << " LLVM_ASM(\"" << I->getName().substr(1) << "\")";
    Out << ";\n";
  }
  if (!M.global_empty()) {
    Out << "\n\n/* Global Variable Declarations */\n";
    if (printout_initialization)
    for (Module::global_iterator I = M.global_begin(), E = M.global_end(); I != E; ++I)
      if (!I->isDeclaration()) {
        if (getGlobalVariableClass(I))
          continue;
        if (I->hasLocalLinkage())
          {} //Out << "static ";
        else
          Out << "extern ";
        if (I->isThreadLocal())
          Out << "__thread ";
        printType(Out, I->getType()->getElementType(), false, GetValueName(I), true, I->hasLocalLinkage());
        if (I->hasExternalWeakLinkage())
          Out << " __EXTERNAL_WEAK__";
        if (I->hasHiddenVisibility())
          Out << " __HIDDEN__";
        Out << ";\n";
      }
  }
  if (!M.global_empty()) {
    Out << "\n\n/* Global Variable Definitions and Initialization */\n";
    for (Module::global_iterator I = M.global_begin(), E = M.global_end(); I != E; ++I)
      if (!I->isDeclaration()) {
        if (getGlobalVariableClass(I))
          continue;
        if (I->hasLocalLinkage())
          Out << "static ";
        else if (I->hasDLLImportLinkage())
          Out << "__declspec(dllimport) ";
        else if (I->hasDLLExportLinkage())
          Out << "__declspec(dllexport) ";
        if (I->isThreadLocal())
          Out << "__thread ";
        printType(Out, I->getType()->getElementType(), false, GetValueName(I), false, 0);
        if (I->hasHiddenVisibility())
          Out << " __HIDDEN__";
        if (!I->getInitializer()->isNullValue()) {
          Out << " = " ;
          writeOperand(I->getInitializer(), false, true);
        } else if (I->hasWeakLinkage()) {
          Out << " = " ;
          if (I->getInitializer()->getType()->isStructTy() || I->getInitializer()->getType()->isVectorTy()) {
            Out << "{ 0 }";
          } else if (I->getInitializer()->getType()->isArrayTy()) {
            Out << "{ { 0 } }";
          } else {
            writeOperand(I->getInitializer(), false, true);
          }
        }
        Out << ";\n";
      }
  }
  if (!M.empty())
    Out << "\n\n/* Function Bodies */\n";
  return false;
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
            OutHeader << "typedef ";
            printType(OutHeader, STy, false, Name, true, 0);
            OutHeader << ";\n\n";
    }
}
void CWriter::flushStruct(void)
{
    structWork_run = 1;
    while (structWork.begin() != structWork.end()) {
        printContainedStructs(*structWork.begin());
        structWork.pop_front();
    }
}
void CWriter::printFunctionSignature(const Function *F, bool Prototype)
{
  bool isStructReturn = F->hasStructRetAttr();
  if (F->hasLocalLinkage()) Out << "static ";
  if (F->hasDLLImportLinkage()) Out << "__declspec(dllimport) ";
  if (F->hasDLLExportLinkage()) Out << "__declspec(dllexport) ";
  FunctionType *FT = cast<FunctionType>(F->getFunctionType());
  //const AttributeSet &PAL = F->getAttributes();
  std::string tstr;
  raw_string_ostream FunctionInnards(tstr);
  FunctionInnards << GetValueName(F) << '(';
  bool PrintedArg = false;
  if (!F->isDeclaration()) {
    if (!F->arg_empty()) {
      Function::const_arg_iterator I = F->arg_begin(), E = F->arg_end();
      unsigned Idx = 1;
      if (isStructReturn) {
        assert(I != E && "Invalid struct return function!");
        ++I;
        ++Idx;
      }
      std::string ArgName;
      for (; I != E; ++I) {
        if (PrintedArg) FunctionInnards << ", ";
        if (I->hasName() || !Prototype)
          ArgName = GetValueName(I);
        else
          ArgName = "";
        Type *ArgTy = I->getType();
        //if (PAL.paramHasAttr(Idx, Attribute::ByVal)) {
          //ArgTy = cast<PointerType>(ArgTy)->getElementType();
          //ByValParams.insert(I);
        //}
        printType(FunctionInnards, ArgTy, /*isSigned=*/false/*PAL.paramHasAttr(Idx, Attribute::SExt)*/, ArgName, false, 0);
        PrintedArg = true;
        ++Idx;
      }
    }
  } else {
    FunctionType::param_iterator I = FT->param_begin(), E = FT->param_end();
    unsigned Idx = 1;
    if (isStructReturn) {
      assert(I != E && "Invalid struct return function!");
      ++I;
      ++Idx;
    }
    for (; I != E; ++I) {
      if (PrintedArg) FunctionInnards << ", ";
      Type *ArgTy = *I;
      printType(FunctionInnards, ArgTy, /*isSigned=*/false/*PAL.paramHasAttr(Idx, Attribute::SExt)*/, "", false, 0);
      PrintedArg = true;
      ++Idx;
    }
  }
  if (!PrintedArg && FT->isVarArg()) {
    FunctionInnards << "int vararg_dummy_arg";
    PrintedArg = true;
  }
  if (FT->isVarArg() && PrintedArg) {
    FunctionInnards << ",...";  // Output varargs portion of signature!
  } else if (!FT->isVarArg() && !PrintedArg) {
    FunctionInnards << "void"; // ret() -> ret(void) in C.
  }
  FunctionInnards << ')';
  Type *RetTy;
  if (!isStructReturn)
    RetTy = F->getReturnType();
  else {
    RetTy = cast<PointerType>(FT->getParamType(0))->getElementType();
  }
  printType(Out, RetTy, /*isSigned=*/false, //PAL.paramHasAttr(0, Attribute::SExt),
            FunctionInnards.str(), false, 0);
}
void CWriter::printFunction(Function &F)
{
  bool isStructReturn = F.hasStructRetAttr();
  printFunctionSignature(&F, false);
  Out << " {\n";
  if (isStructReturn) {
    Type *StructTy =
      cast<PointerType>(F.arg_begin()->getType())->getElementType();
    Out << "  ";
    printType(Out, StructTy, false, "StructReturn", false, 0);
    Out << ";  /* Struct return temporary */\n";
    Out << "  ";
    printType(Out, F.arg_begin()->getType(), false, GetValueName(F.arg_begin()), false, 0);
    Out << " = &StructReturn;\n";
  }
  bool PrintedVar = false;
  for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; ++I) {
    if (const AllocaInst *AI = isDirectAlloca(&*I)) {
      Out << "  ";
      printType(Out, AI->getAllocatedType(), false, GetValueName(AI), false, 0);
      Out << ";    /* Address-exposed local */\n";
      PrintedVar = true;
    } else if (I->getType() != Type::getVoidTy(F.getContext()) && !isInlinableInst(*I)) {
      Out << "  ";
      printType(Out, I->getType(), false, GetValueName(&*I), false, 0);
      Out << ";\n";
      if (isa<PHINode>(*I)) {  // Print out PHI node temporaries as well...
        Out << "  ";
        printType(Out, I->getType(), false, GetValueName(&*I)+"__PHI_TEMPORARY", false, 0);
        Out << ";\n";
      }
      PrintedVar = true;
    }
  }
  if (PrintedVar)
    Out << '\n';
  if (F.hasExternalLinkage() && F.getName() == "main")
    Out << "  CODE_FOR_MAIN();\n";
  for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB) {
      printBasicBlock(BB);
  }
  Out << "}\n\n";
}
void CWriter::printBasicBlock(BasicBlock *BB)
{
  bool NeedsLabel = true; //false;
  if (NeedsLabel) Out << GetValueName(BB) << ":\n";
  for (BasicBlock::iterator II = BB->begin(), E = --BB->end(); II != E; ++II) {
    if (!isInlinableInst(*II) && !isDirectAlloca(II)) {
      if (II->getType() != Type::getVoidTy(BB->getContext()))
        Out << "  " << GetValueName(II) << " = ";
      else
        Out << "  ";
      writeInstComputationInline(*II);
      Out << ";\n";
    }
  }
  visit(*BB->getTerminator());
}
void CWriter::visitReturnInst(ReturnInst &I)
{
  bool isStructReturn = I.getParent()->getParent()->hasStructRetAttr();
  if (isStructReturn) {
    Out << "  return StructReturn;\n";
    return;
  }
  if (I.getNumOperands() == 0 &&
      &*--I.getParent()->getParent()->end() == I.getParent() &&
      !I.getParent()->size() == 1) {
    return;
  }
  Out << "  return";
  if (I.getNumOperands()) {
    Out << ' ';
    writeOperand(I.getOperand(0), false);
  }
  Out << ";\n";
}
void CWriter::visitIndirectBrInst(IndirectBrInst &IBI)
{
  Out << "  goto *(void*)(";
  writeOperand(IBI.getOperand(0), false);
  Out << ");\n";
}
void CWriter::visitUnreachableInst(UnreachableInst &I)
{
  Out << "  /*UNREACHABLE*/;\n";
}
bool CWriter::isGotoCodeNecessary(BasicBlock *From, BasicBlock *To)
{
  if (llvm::next(Function::iterator(From)) != Function::iterator(To))
    return true;  // Not the direct successor, we need a goto.
  return false;
}
void CWriter::printPHICopiesForSuccessor (BasicBlock *CurBlock, BasicBlock *Successor, unsigned Indent)
{
  for (BasicBlock::iterator I = Successor->begin(); isa<PHINode>(I); ++I) {
    PHINode *PN = cast<PHINode>(I);
    Value *IV = PN->getIncomingValueForBlock(CurBlock);
    if (!isa<UndefValue>(IV)) {
      Out << std::string(Indent, ' ');
      Out << "  " << GetValueName(I) << "__PHI_TEMPORARY = ";
      writeOperand(IV, false);
      Out << ";   /* for PHI node */\n";
    }
  }
  if (isGotoCodeNecessary(CurBlock, Successor)) {
    Out << std::string(Indent, ' ') << "  goto ";
    writeOperand(Successor, false);
    Out << ";\n";
  }
}
void CWriter::visitBranchInst(BranchInst &I)
{
  if (I.isConditional()) {
    if (isGotoCodeNecessary(I.getParent(), I.getSuccessor(0))) {
      Out << "  if (";
      writeOperand(I.getCondition(), false);
      Out << ") {\n";
      printPHICopiesForSuccessor (I.getParent(), I.getSuccessor(0), 2);
      if (isGotoCodeNecessary(I.getParent(), I.getSuccessor(1))) {
        Out << "  } else {\n";
        printPHICopiesForSuccessor (I.getParent(), I.getSuccessor(1), 2);
      }
    } else {
      Out << "  if (!";
      writeOperand(I.getCondition(), false);
      Out << ") {\n";
      printPHICopiesForSuccessor (I.getParent(), I.getSuccessor(1), 2);
    }
    Out << "  }\n";
  } else
    printPHICopiesForSuccessor (I.getParent(), I.getSuccessor(0), 0);
  Out << "\n";
}
void CWriter::visitPHINode(PHINode &I)
{
  writeOperand(&I, false);
  Out << "__PHI_TEMPORARY";
}
void CWriter::visitBinaryOperator(Instruction &I)
{
  assert(!I.getType()->isPointerTy());
  bool needsCast = false;
  if ((I.getType() == Type::getInt8Ty(I.getContext())) ||
      (I.getType() == Type::getInt16Ty(I.getContext()))
      || (I.getType() == Type::getFloatTy(I.getContext()))) {
    needsCast = true;
    Out << "((";
    printType(Out, I.getType(), false, "", false, 0);
    Out << ")(";
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
    bool NeedsClosingParens = writeInstructionCast(I);
    writeOperandWithCast(I.getOperand(0), I.getOpcode());
    switch (I.getOpcode()) {
    case Instruction::Add: case Instruction::FAdd: Out << " + "; break;
    case Instruction::Sub: case Instruction::FSub: Out << " - "; break;
    case Instruction::Mul: case Instruction::FMul: Out << " * "; break;
    case Instruction::URem: case Instruction::SRem: case Instruction::FRem: Out << " % "; break;
    case Instruction::UDiv: case Instruction::SDiv: case Instruction::FDiv: Out << " / "; break;
    case Instruction::And:  Out << " & "; break;
    case Instruction::Or:   Out << " | "; break;
    case Instruction::Xor:  Out << " ^ "; break;
    case Instruction::Shl : Out << " << "; break;
    case Instruction::LShr: case Instruction::AShr: Out << " >> "; break;
    default:
       errs() << "Invalid operator type!" << I;
       llvm_unreachable(0);
    }
    writeOperandWithCast(I.getOperand(1), I.getOpcode());
    if (NeedsClosingParens)
      Out << "))";
  }
  if (needsCast) {
    Out << "))";
  }
}
void CWriter::visitICmpInst(ICmpInst &I)
{
  bool needsCast = false;
  bool NeedsClosingParens = writeInstructionCast(I);
  writeOperandWithCast(I.getOperand(0), I);
  switch (I.getPredicate()) {
  case ICmpInst::ICMP_EQ:  Out << " == "; break;
  case ICmpInst::ICMP_NE:  Out << " != "; break;
  case ICmpInst::ICMP_ULE: case ICmpInst::ICMP_SLE: Out << " <= "; break;
  case ICmpInst::ICMP_UGE: case ICmpInst::ICMP_SGE: Out << " >= "; break;
  case ICmpInst::ICMP_ULT: case ICmpInst::ICMP_SLT: Out << " < "; break;
  case ICmpInst::ICMP_UGT: case ICmpInst::ICMP_SGT: Out << " > "; break;
  default:
    errs() << "Invalid icmp predicate!" << I;
    llvm_unreachable(0);
  }
  writeOperandWithCast(I.getOperand(1), I);
  if (NeedsClosingParens)
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
      if (Intrinsic::ID ID = (Intrinsic::ID)F->getIntrinsicID())
          if (visitBuiltinCall(I, ID, WroteCallee))
              return;
  Value *Callee = I.getCalledValue();
  PointerType  *PTy   = cast<PointerType>(Callee->getType());
  FunctionType *FTy   = cast<FunctionType>(PTy->getElementType());
  //const AttributeSet &PAL = I.getAttributes();
  bool hasByVal = I.hasByValArgument();
  bool isStructRet = I.hasStructRetAttr();
  if (isStructRet) {
      Value *Operand = I.getArgOperand(0);
      if (isAddressExposed(Operand))
          writeOperandInternal(Operand);
      else {
          Out << "*(";
          writeOperand(Operand, false);
          Out << ")";
      }
      Out << " = ";
  }
  if (I.isTailCall()) Out << " /*tail*/ ";
  if (!WroteCallee) {
    bool NeedsCast = (hasByVal || isStructRet) && !isa<Function>(Callee);
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(Callee))
      if (CE->isCast())
        if (Function *RF = dyn_cast<Function>(CE->getOperand(0))) {
          NeedsCast = true;
          Callee = RF;
        }
    if (NeedsCast) {
      Out << "((";
      if (isStructRet) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      else if (hasByVal)
        printType(Out, I.getCalledValue()->getType(), false, "", true, 0);
      else
        printType(Out, I.getCalledValue()->getType(), false, "", false, 0);
      Out << ")(void*)";
    }
    writeOperand(Callee, false);
    if (NeedsCast) Out << ')';
  }
  Out << '(';
  if (Callee->getName() == "printf")
      Out << "(const char *) ";
  bool PrintedArg = false;
  if(FTy->isVarArg() && !FTy->getNumParams()) {
    Out << "0 /*dummy arg*/";
    PrintedArg = true;
  }
  unsigned NumDeclaredParams = FTy->getNumParams();
  CallSite CS(&I);
  CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();
  unsigned ArgNo = 0;
  if (isStructRet) {   // Skip struct return argument.
    ++AI;
    ++ArgNo;
  }
  for (; AI != AE; ++AI, ++ArgNo) {
    if (PrintedArg) Out << ", ";
    if (ArgNo < NumDeclaredParams &&
        (*AI)->getType() != FTy->getParamType(ArgNo)) {
      Out << '(';
      printType(Out, FTy->getParamType(ArgNo), /*isSigned=*/false, "", false, 0);//PAL.paramHasAttr(ArgNo+1, Attribute::SExt), "", false, 0);
      Out << ')';
    }
      writeOperand(*AI, false);
    PrintedArg = true;
  }
  Out << ')';
}
bool CWriter::visitBuiltinCall(CallInst &I, Intrinsic::ID ID, bool &WroteCallee)
{
    Function *F = I.getCalledFunction();
    Out << F->getName() + "BUILTINSTUB";
    WroteCallee = true;
    return 0;
}
void CWriter::visitAllocaInst(AllocaInst &I)
{
  Out << '(';
  printType(Out, I.getType(), false, "", false, 0);
  Out << ") alloca(sizeof(";
  printType(Out, I.getType()->getElementType(), false, "", false, 0);
  Out << ')';
  if (I.isArrayAllocation()) {
    Out << " * " ;
    writeOperand(I.getOperand(0), false);
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
  {
    for (gep_type_iterator TmpI = I; TmpI != E; ++TmpI)
      LastIndexIsVector = dyn_cast<VectorType>(*TmpI);
  }
  Out << "(";
  if (LastIndexIsVector) {
    Out << "((";
    printType(Out, PointerType::getUnqual(LastIndexIsVector->getElementType()), false, "", false, 0);
    Out << ")(";
  }
  Out << '&';
  Value *FirstOp = I.getOperand();
  if (!isa<Constant>(FirstOp) || !cast<Constant>(FirstOp)->isNullValue()) {
    writeOperand(Ptr, false);
  } else {
    ++I;  // Skip the zero index.
    if (isAddressExposed(Ptr)) {
      writeOperandInternal(Ptr, Static);
    } else if (I != E && (*I)->isStructTy()) {
      writeOperand(Ptr, false);
      Out << "->field" << cast<ConstantInt>(I.getOperand())->getZExtValue();
      ++I;  // eat the struct index as well.
    } else {
      Out << "(*";
      writeOperand(Ptr, false);
      Out << ")";
    }
  }
  for (; I != E; ++I) {
    if ((*I)->isStructTy()) {
      Out << ".field" << cast<ConstantInt>(I.getOperand())->getZExtValue();
    } else if ((*I)->isArrayTy()) {
      Out << ".array[";
      //writeOperandWithCast(I.getOperand(), Instruction::GetElementPtr);
      writeOperand(I.getOperand(), false);
      Out << ']';
    } else if (!(*I)->isVectorTy()) {
      Out << '[';
      //writeOperandWithCast(I.getOperand(), Instruction::GetElementPtr);
      writeOperand(I.getOperand(), false);
      Out << ']';
    } else {
      if (isa<Constant>(I.getOperand()) &&
          cast<Constant>(I.getOperand())->isNullValue()) {
        Out << "))";  // avoid "+0".
      } else {
        Out << ")+(";
        writeOperandWithCast(I.getOperand(), Instruction::GetElementPtr);
        Out << "))";
      }
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
