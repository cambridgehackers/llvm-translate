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

static std::list<StructType *> structWork;
static std::map<Type *, int> structMap;
static int structWork_run;
char CWriter::ID = 0;

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
std::string CWriter::getStructName(StructType *ST)
{
    if (!ST->isLiteral() && !ST->getName().empty())
        return CBEMangle("l_"+ST->getName().str());
    if (!UnnamedStructIDs[ST])
        UnnamedStructIDs[ST] = NextTypeID++;
    return "l_unnamed_" + utostr(UnnamedStructIDs[ST]);
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
/*
 * Output types
 */
void CWriter::printType(raw_ostream &Out, Type *Ty, bool isSigned, std::string NameSoFar, bool IgnoreName, std::string prefix, std::string postfix)
{
  const char *sp = (isSigned?"signed":"unsigned");
  Out << prefix;
restart_label:
  switch (Ty->getTypeID()) {
  case Type::VoidTyID:
      Out << "void " << NameSoFar;
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
      printType(FunctionInnards, *I, /*isSigned=*/false, "", false, "", "");
      PrintedArg = true;
    }
    if (FTy->isVarArg()) {
      if (!FTy->getNumParams())
        FunctionInnards << " int"; //dummy argument for empty vaarg functs
      FunctionInnards << ", ...";
    } else if (!FTy->getNumParams())
      FunctionInnards << "void";
    FunctionInnards << ')';
    printType(Out, FTy->getReturnType(), /*isSigned=*/false, FunctionInnards.str(), false, "", "");
    break;
  }
  case Type::StructTyID: {
    StructType *STy = cast<StructType>(Ty);
    std::string name = getStructName(STy);
    if (!structWork_run)
        structWork.push_back(STy);
    Out << "struct " << name << " ";
    if (IgnoreName && (strncmp(name.c_str(), "l_", 2) || name == NameSoFar)) {
        Out << "{\n";
        unsigned Idx = 0;
        for (StructType::element_iterator I = STy->element_begin(), E = STy->element_end(); I != E; ++I) {
          printType(Out, *I, false, fieldName(STy, Idx++), false, "  ", ";\n");
        }
        Out << "} ";
    }
    Out << NameSoFar;
    break;
  }
  case Type::ArrayTyID: {
    ArrayType *ATy = cast<ArrayType>(Ty);
    printType(Out, ATy->getElementType(), false, "", false, "", "");
    unsigned NumElements = ATy->getNumElements();
    if (NumElements == 0) NumElements = 1;
    Out << NameSoFar << "[" + utostr(NumElements) + "]";
    break;
  }
  case Type::PointerTyID: {
    PointerType *PTy = cast<PointerType>(Ty);
    std::string ptrName = "*" + NameSoFar;
    if (PTy->getElementType()->isArrayTy() || PTy->getElementType()->isVectorTy())
      ptrName = "(" + ptrName + ")";
    printType(Out, PTy->getElementType(), false, ptrName, false, "", "");
    break;
  }
  default:
    llvm_unreachable("Unhandled case in getTypeProps!");
  }
  Out << postfix;
}

/*
 * Output expressions
 */
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
  bool isString = (ETy == Type::getInt8Ty(CPA->getContext()) || ETy == Type::getInt8Ty(CPA->getContext()));
  if (isString && (CPA->getNumOperands() == 0 || !cast<Constant>(*(CPA->op_end()-1))->isNullValue()))
    isString = false;
  if (isString) {
    char *cp = (char *)malloc(len);
    for (unsigned i = 0, e = CPA->getNumOperands()-1; i != e; ++i)
        cp[i] = cast<ConstantInt>(CPA->getOperand(i))->getZExtValue();
    printString(cp, len);
    free(cp);
  } else {
    const char *sep = " ";
    Out << '{';
    for (unsigned i = 0, e = CPA->getNumOperands(); i != e; ++i) {
        Out << sep;
        printConstant(cast<Constant>(CPA->getOperand(i)), Static);
        sep = ", ";
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
    const char *sep = " ";
    for (unsigned i = 0, e = CPA->getNumOperands(); i != e; ++i) {
        Out << sep;
        printConstant(cast<Constant>(CPA->getOperand(i)), Static);
        sep = ", ";
    }
    Out << " }";
  }
}
void CWriter::printConstantVector(ConstantVector *CP, bool Static)
{
  Out << '{';
  const char *sep = " ";
  for (unsigned i = 0, e = CP->getNumOperands(); i != e; ++i) {
      Out << sep;
      printConstant(cast<Constant>(CP->getOperand(i)), Static);
      sep = ", ";
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
  printType(Out, DstTy, TypeIsSigned, "", false, "(", ")");
  switch (opc) {
    case Instruction::Trunc: case Instruction::BitCast: case Instruction::FPExt:
    case Instruction::FPTrunc: case Instruction::FPToSI: case Instruction::FPToUI:
      return; // These don't need a source cast.
    case Instruction::IntToPtr: case Instruction::PtrToInt:
      Out << "(unsigned long)";
      return;
  }
  printType(Out, SrcTy, TypeIsSigned, "", false, "(", ")");
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
  if (LastIndexIsVector)
    printType(Out, PointerType::getUnqual(LastIndexIsVector->getElementType()), false, "", false, "((", ")(");
  Value *FirstOp = I.getOperand();
  if (!isa<Constant>(FirstOp) || !cast<Constant>(FirstOp)->isNullValue()) {
    Out << "&";
    writeOperand(Ptr, false);
  } else {
    ++I;  // Skip the zero index.
    if (isAddressExposed(Ptr) && I != E && (*I)->isArrayTy()) {
      if (ConstantInt *CI = dyn_cast<ConstantInt>(I.getOperand())) {
        uint64_t val = CI->getZExtValue();
        ++I;     // we processed this index
        GlobalVariable *gv = dyn_cast<GlobalVariable>(Ptr);
        if (gv && !gv->getInitializer()->isNullValue()) {
            Constant* CPV = dyn_cast<Constant>(gv->getInitializer());
            if (ConstantArray *CA = dyn_cast<ConstantArray>(CPV)) {
                (void)CA;
                if (val != 2) {
                    printf("[%s:%d]\n", __FUNCTION__, __LINE__);
                    exit(1);
                }
                val = 0;
            } else 
            if (ConstantDataArray *CA = dyn_cast<ConstantDataArray>(CPV)) {
                if (val) {
                    printf("[%s:%d]\n", __FUNCTION__, __LINE__);
                    exit(1);
                }
                printConstantDataArray(CA, Static);
                goto next;
            }
        }
        writeOperand(Ptr, true, Static);
next:
        if (val)
            Out << "+" << val;
        goto done;
      }
    }
    Out << "&";
    if (isAddressExposed(Ptr)) {
      writeOperand(Ptr, true, Static);
    } else if (I != E && (*I)->isStructTy()) {
      writeOperand(Ptr, false);
      Out << "->" << fieldName(dyn_cast<StructType>(*I), cast<ConstantInt>(I.getOperand())->getZExtValue());
      ++I;  // eat the struct index as well.
    } else {
      Out << "(";
      writeOperand(Ptr, true);
      Out << ")";
    }
  }
done:
  for (; I != E; ++I) {
    if ((*I)->isStructTy()) {
      StructType *STy = dyn_cast<StructType>(*I);
      Out << "." << fieldName(STy, cast<ConstantInt>(I.getOperand())->getZExtValue());
    } else if ((*I)->isArrayTy() || !(*I)->isVectorTy()) {
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
void CWriter::printConstant(Constant *CPV, bool Static)
{
  /* handle expressions */
  if (const ConstantExpr *CE = dyn_cast<ConstantExpr>(CPV)) {
    Out << "(";
    int op = CE->getOpcode();
    switch (op) {
    case Instruction::Trunc: case Instruction::ZExt: case Instruction::SExt:
    case Instruction::FPTrunc: case Instruction::FPExt: case Instruction::UIToFP:
    case Instruction::SIToFP: case Instruction::FPToUI: case Instruction::FPToSI:
    case Instruction::PtrToInt: case Instruction::IntToPtr: case Instruction::BitCast:
      printCast(op, CE->getOperand(0)->getType(), CE->getType());
      if (op == Instruction::SExt &&
          CE->getOperand(0)->getType() == Type::getInt1Ty(CPV->getContext())) {
        Out << "0-";
      }
      printConstant(CE->getOperand(0), Static);
      if (CE->getType() == Type::getInt1Ty(CPV->getContext()) &&
          (op == Instruction::Trunc || op == Instruction::FPToUI ||
           op == Instruction::FPToSI || op == Instruction::PtrToInt)) {
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
      printConstantWithCast(CE->getOperand(0), op);
      Out << " ";
      if (op == Instruction::ICmp)
        Out << intmapLookup(predText, CE->getPredicate());
      else
        Out << intmapLookup(opcodeMap, op);
      Out << " ";
      printConstantWithCast(CE->getOperand(1), op);
      printConstExprCast(CE);
      break;
    }
    case Instruction::FCmp: {
      if (CE->getPredicate() == FCmpInst::FCMP_FALSE)
        Out << "0";
      else if (CE->getPredicate() == FCmpInst::FCMP_TRUE)
        Out << "1";
      else {
        Out << "llvm_fcmp_" << intmapLookup(predText, CE->getPredicate()) << "(";
        printConstantWithCast(CE->getOperand(0), op);
        Out << ", ";
        printConstantWithCast(CE->getOperand(1), op);
        Out << ")";
      }
      printConstExprCast(CE);
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
    printType(Out, CPV->getType(), false, "", false, "((", ")/*UNDEF*/"); // sign doesn't matter
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
    else if (Ty == Type::getInt32Ty(CPV->getContext()) || Ty->getPrimitiveSizeInBits() > 32)
      Out << CI->getZExtValue();
    else {
      printType(Out, Ty, false, "", false, "((", ")");
      if (CI->isMinValue(true))
        Out << CI->getZExtValue();// << 'u';
      else
        Out << CI->getSExtValue();
      Out << ')';
    }
    return;
  }
  int tid = CPV->getType()->getTypeID();
  if (tid != Type::PointerTyID && !Static) {
      printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      exit(1);
  }
  /* handle structured types */
  switch (tid) {
  case Type::ArrayTyID:
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
      const char *sep = " ";
      Constant *CZ = Constant::getNullValue(AT->getElementType());
      for (unsigned i = 0, e = AT->getNumElements(); i != e; ++i) {
          Out << sep;
          printConstant(CZ, Static);
          sep = ", ";
      }
      Out << " }";
    }
    break;
  case Type::VectorTyID:
    if (ConstantVector *CV = dyn_cast<ConstantVector>(CPV))
      printConstantVector(CV, Static);
    else {
      assert(isa<ConstantAggregateZero>(CPV) || isa<UndefValue>(CPV));
      VectorType *VT = cast<VectorType>(CPV->getType());
      Out << "{";
      const char *sep = " ";
      Constant *CZ = Constant::getNullValue(VT->getElementType());
      for (unsigned i = 0, e = VT->getNumElements(); i != e; ++i) {
        Out << sep;
        printConstant(CZ, Static);
        sep = ", ";
      }
      Out << " }";
    }
    break;
  case Type::StructTyID:
    Out << '{';
    if (isa<ConstantAggregateZero>(CPV) || isa<UndefValue>(CPV)) {
      StructType *ST = cast<StructType>(CPV->getType());
      const char *sep = " ";
      for (unsigned i = 0, e = ST->getNumElements(); i != e; ++i) {
          Out << sep;
          printConstant(Constant::getNullValue(ST->getElementType(i)), Static);
          sep = ", ";
      }
    } else {
        const char *sep = " ";
        for (unsigned i = 0, e = CPV->getNumOperands(); i != e; ++i) {
            Out << sep;
            printConstant(cast<Constant>(CPV->getOperand(i)), Static);
            sep = ", ";
        }
    }
    Out << " }";
    break;
  case Type::PointerTyID:
    if (isa<ConstantPointerNull>(CPV))
      printType(Out, CPV->getType(), false, "", false, "((", ")/*NULL*/0)"); // sign doesn't matter
    else if (GlobalValue *GV = dyn_cast<GlobalValue>(CPV))
      writeOperand(GV, false, Static);
    break;
  default:
    errs() << "Unknown constant type: " << *CPV << "\n";
    llvm_unreachable(0);
  }
}
bool CWriter::printConstExprCast(const ConstantExpr* CE)
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
  printType(Out, Ty, TypeIsSigned, "", false, "((", ")(");
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
  printType(Out, OpTy, typeIsSigned, "", false, "((", ")");
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
    return CBEMangle(GV->getName().str());
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
      visit(*I);
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
  printType(Out, OpTy, castIsSigned, "", false, "((", ")");
  writeOperand(Operand, false);
  Out << ")";
}
void CWriter::writeOperandWithCastICmp(Value* Operand, const ICmpInst &Cmp)
{
  bool shouldCast = Cmp.isRelational();
  if (shouldCast) {
      Type* OpTy = Operand->getType();
      if (OpTy->isPointerTy()) {
        printf("[%s:%d]\n", __FUNCTION__, __LINE__);
        exit(1);
        //OpTy = TD->getIntPtrType(Operand->getContext());
      }
      printType(Out, OpTy, Cmp.isSigned(), "", false, "((", ")");
  }
  writeOperand(Operand, false);
  if (shouldCast)
      Out << ")";
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
  if (F->isDeclaration()) {
    for (FunctionType::param_iterator I = FT->param_begin(), E = FT->param_end(); I != E; ++I) {
      if (PrintedArg)
          FunctionInnards << ", ";
      printType(FunctionInnards, *I, /*isSigned=*/false, "", false, "", "");
      PrintedArg = true;
    }
  } else if (!F->arg_empty()) {
    for (Function::const_arg_iterator I = F->arg_begin(), E = F->arg_end(); I != E; ++I) {
      std::string ArgName = "";
      if (PrintedArg) FunctionInnards << ", ";
      if (I->hasName() || !Prototype)
        ArgName = GetValueName(I);
      Type *ArgTy = I->getType();
      printType(FunctionInnards, ArgTy, /*isSigned=*/false, ArgName, false, "", "");
      PrintedArg = true;
    }
  }
  if (!PrintedArg)
    FunctionInnards << "void"; // ret() -> ret(void) in C.
  FunctionInnards << ')';
  printType(Out, F->getReturnType(), /*isSigned=*/false, FunctionInnards.str(), false, "", "");
}

/*
 * Output instructions
 */
void CWriter::visitReturnInst(ReturnInst &I)
{
  if (I.getNumOperands() != 0 || I.getParent()->getParent()->size() != 1) {
    Out << "  return ";
    if (I.getNumOperands())
      writeOperand(I.getOperand(0), false);
    Out << ";\n";
  }
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
  printType(Out, Ty, typeIsSigned, "", false, "((", ")(");
  Out << "))";
  return true;
}
void CWriter::visitBinaryOperator(Instruction &I)
{
  assert(!I.getType()->isPointerTy());
  bool needsCast = false;
  if (I.getType() ==  Type::getInt8Ty(I.getContext())
   || I.getType() == Type::getInt16Ty(I.getContext())
   || I.getType() == Type::getFloatTy(I.getContext())) {
    needsCast = true;
    printType(Out, I.getType(), false, "", false, "((", ")(");
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
    Out << intmapLookup(opcodeMap, I.getOpcode());
    Out << " ";
    writeOperandWithCast(I.getOperand(1), I.getOpcode());
    writeInstructionCast(I);
  }
  if (needsCast) {
    Out << "))";
  }
}
void CWriter::visitICmpInst(ICmpInst &I)
{
  bool needsCast = false;
  writeOperandWithCastICmp(I.getOperand(0), I);
  Out << " ";
  Out << intmapLookup(predText, I.getPredicate());
  Out << " ";
  writeOperandWithCastICmp(I.getOperand(1), I);
  writeInstructionCast(I);
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
  if (Function *F = I.getCalledFunction())
      if (Intrinsic::ID ID = (Intrinsic::ID)F->getIntrinsicID()) {
          (void)ID;
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
  Value *Callee = I.getCalledValue();
  PointerType  *PTy   = cast<PointerType>(Callee->getType());
  FunctionType *FTy   = cast<FunctionType>(PTy->getElementType());
  if (I.hasStructRetAttr() || I.hasByValArgument() || I.isTailCall()) {
      printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      exit(1);
  }
  bool NeedsCast = false;
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(Callee))
    if (CE->isCast())
      if (Function *RF = dyn_cast<Function>(CE->getOperand(0))) {
        NeedsCast = true;
        Callee = RF;
        printType(Out, I.getCalledValue()->getType(), false, "", false, "((", ")(void*)");
      }
  writeOperand(Callee, false);
  if (NeedsCast) Out << ')';
  if(FTy->isVarArg() && !FTy->getNumParams()) {
    printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    exit(1);
  }
  unsigned NumDeclaredParams = FTy->getNumParams();
  CallSite CS(&I);
  CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();
  unsigned ArgNo = 0;
  const char *sep = "";
  Out << '(';
  for (; AI != AE; ++AI, ++ArgNo) {
    Out << sep;
    if (ArgNo < NumDeclaredParams && (*AI)->getType() != FTy->getParamType(ArgNo))
        printType(Out, FTy->getParamType(ArgNo), /*isSigned=*/false, "", false, "(", ")");
    writeOperand(*AI, false);
    sep = ", ";
  }
  Out << ')';
}
void CWriter::visitGetElementPtrInst(GetElementPtrInst &I)
{
  printGEPExpression(I.getPointerOperand(), gep_type_begin(I), gep_type_end(I), false);
}
void CWriter::visitLoadInst(LoadInst &I)
{
  if (I.isVolatile()) {
    printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    I.getOperand(0)->dump();
    exit(1);
  }
  writeOperand(I.getOperand(0), true);
}
void CWriter::visitStoreInst(StoreInst &I)
{
  if (I.isVolatile()) {
    printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    I.getPointerOperand()->dump();
    exit(1);
  }
  writeOperand(I.getPointerOperand(), true);
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

/*
 * Pass control functions
 */
static int getGlobalVariableClass(const GlobalVariable *GV)
{
  if (GV->isDeclaration())
      return 1;
  if (GV->hasAppendingLinkage() && GV->use_empty()
    && (GV->getName() == "llvm.global_ctors" || GV->getName() == "llvm.global_dtors"))
      return 1;
  if (GV->getSection() == "llvm.metadata")
      return 1;
  return 0;
}
bool CWriter::doInitialization(Module &M)
{
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
  FunctionPass::doInitialization(M);
  if (!M.global_empty()) {
    Out << "\n\n/* Global Variable Definitions and Initialization */\n";
    for (Module::global_iterator I = M.global_begin(), E = M.global_end(); I != E; ++I) {
      if (I->hasWeakLinkage() || I->hasDLLImportLinkage() || I->hasDLLExportLinkage()
      || I->isThreadLocal() || I->hasHiddenVisibility() || I->hasExternalWeakLinkage()) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      if (!getGlobalVariableClass(I)) {
        Type *Ty = I->getType()->getElementType();
        if (Ty->getTypeID() == Type::ArrayTyID)
            if (ArrayType *ATy = cast<ArrayType>(Ty))
                if (ATy->getElementType()->getTypeID() == Type::PointerTyID)
                    continue;
        if (!I->getInitializer()->isNullValue())
                continue;
        if (I->hasLocalLinkage())
          Out << "static ";
        printType(Out, Ty, false, GetValueName(I), false, "", "");
        if (!I->getInitializer()->isNullValue()) {
          Out << " = " ;
          writeOperand(I->getInitializer(), false, true);
        }
        Out << ";\n";
      }
    }
    Out << "\n\n//******************** vtables for Classes *******************\n";
    for (Module::global_iterator I = M.global_begin(), E = M.global_end(); I != E; ++I)
      if (!getGlobalVariableClass(I)) {
        Type *Ty = I->getType()->getElementType();
        ArrayType *ATy;
        PointerType *PTy, *PPTy;
        const FunctionType *FT;
        StructType *STy, *ISTy;
        const ConstantExpr *CE;
        if (Ty->getTypeID() == Type::ArrayTyID && (ATy = cast<ArrayType>(Ty))
         && ATy->getElementType()->getTypeID() == Type::PointerTyID) {
            if (I->hasLocalLinkage())
              Out << "static ";
            printType(Out, Ty, false, GetValueName(I), false, "", "");
            if (!I->getInitializer()->isNullValue()) {
              Out << " = " ;
              Constant* CPV = dyn_cast<Constant>(I->getInitializer());
              if (ConstantArray *CA = dyn_cast<ConstantArray>(CPV)) {
                  Type *ETy = CA->getType()->getElementType();
                  if (ETy == Type::getInt8Ty(CA->getContext()) || ETy == Type::getInt8Ty(CA->getContext())) {
                      printf("[%s:%d]\n", __FUNCTION__, __LINE__);
                      exit(1);
                  }
                  Out << '{';
                  const char *sep = " ";
                  if ((CE = dyn_cast<ConstantExpr>(CA->getOperand(3))) && CE->getOpcode() == Instruction::BitCast
                   && (PTy = cast<PointerType>(CE->getOperand(0)->getType())) && (FT = dyn_cast<FunctionType>(PTy->getElementType()))
                   && FT->getNumParams() >= 1 && (PPTy = cast<PointerType>(FT->getParamType(0)))
                   && (STy = cast<StructType>(PPTy->getElementType()))
                   && STy->getNumElements() > 0 && STy->getElementType(0)->getTypeID() == Type::StructTyID
                   && (ISTy = cast<StructType>(STy->getElementType(0))) && !strcmp(ISTy->getName().str().c_str(), "class.Rule"))
                      for (unsigned i = 2, e = CA->getNumOperands(); i != e; ++i) {
                        Out << sep;
                        Constant* V = dyn_cast<Constant>(CA->getOperand(i));
                        printConstant(V, true);
                        sep = ", ";
                      }
                  else
                      Out << 0;
                  Out << " }";
              }
            }
            Out << ";\n";
        }
    }
  }
  return false;
}
bool CWriter::runOnFunction(Function &F)
{
    int status;
    const char *demang = abi::__cxa_demangle(F.getName().str().c_str(), 0, 0, &status);
    if (demang && strstr(demang, "::~")) {
        //printf("[%s:%d] ignorename %s\n", __FUNCTION__, __LINE__, demang);
        return false;
    }
    NextAnonValueNumber = 0;
    if (!F.isDeclaration() && F.getName() != "_Z16run_main_programv" && F.getName() != "main"
     && F.getName() != "__dtor_echoTest") {
        printFunctionSignature(Out, &F, false);
        Out << " {\n";
        for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB) {
            for (BasicBlock::iterator II = BB->begin(), E = --BB->end(); II != E; ++II) {
              if (const AllocaInst *AI = isDirectAlloca(&*II))
                printType(Out, AI->getAllocatedType(), false, GetValueName(AI), false, "    ", ";    /* Address-exposed local */\n");
              else if (!isInlinableInst(*II)) {
                Out << "    ";
                if (II->getType() != Type::getVoidTy(BB->getContext()))
                    printType(Out, II->getType(), false, GetValueName(&*II), false, "", " = ");
                if (isa<PHINode>(*II)) {    // Print out PHI node temporaries as well...
                    printf("[%s:%d]\n", __FUNCTION__, __LINE__);
                    exit(1);
                }
                visit(*II);
                Out << ";\n";
              }
            }
            visit(*BB->getTerminator());
        }
        Out << "}\n\n";
    }
    return false;
}
void CWriter::printContainedStructs(Type *Ty)
{
    std::map<Type *, int>::iterator FI = structMap.find(Ty);
    if (FI != structMap.end() || Ty->isPointerTy() || Ty->isPrimitiveType() || Ty->isIntegerTy())
        return;
    structMap[Ty] = 1;
    for (Type::subtype_iterator I = Ty->subtype_begin(), E = Ty->subtype_end(); I != E; ++I)
        printContainedStructs(*I);
    if (StructType *STy = dyn_cast<StructType>(Ty))
        printType(OutHeader, STy, false, getStructName(STy), true, "typedef ", ";\n\n");
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
        if (I->hasExternalLinkage() || I->hasCommonLinkage())
          printType(OutHeader, I->getType()->getElementType(), false, GetValueName(I), false, "extern ", ";\n");
      }
    }
    OutHeader << "\n/* Function Declarations */\n";
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
      if (I->hasExternalWeakLinkage() || I->hasHiddenVisibility() || (I->hasName() && I->getName()[0] == 1)) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      if (I->isIntrinsic() || I->getName() == "main" || I->getName() == "atexit"
       || I->getName() == "printf" || I->getName() == "__cxa_pure_virtual"
       || I->getName() == "setjmp" || I->getName() == "longjmp" || I->getName() == "_setjmp")
        continue;
      printFunctionSignature(OutHeader, I, true);
      OutHeader << ";\n";
    }
    UnnamedStructIDs.clear();
    return false;
}
