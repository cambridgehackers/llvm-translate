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
#include "llvm/IR/Instructions.h"

using namespace llvm;

#include "declarations.h"

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
static char vout[MAX_CHAR_BUFFER];

const char *intmap_lookup(INTMAP_TYPE *map, int value)
{
    while (map->name) {
        if (map->value == value)
            return map->name;
        map++;
    }
    return "unknown";
}

/*
 * Generate Verilog output for Store and Call instructions
 */
const char *generateVerilog(Function ***thisp, Instruction &I)
{
    int dump_operands = trace_full;
    int opcode = I.getOpcode();

    vout[0] = 0;
    switch (opcode) {
    // Terminators
    case Instruction::Ret:
        if (I.getParent()->getParent()->getReturnType()->getTypeID()
               == Type::IntegerTyID && operand_list_index > 1) {
            operand_list[0].type = OpTypeString;
            operand_list[0].value = (uint64_t)getparam(1);
            sprintf(vout, "%s = %s;", globalName, getparam(1));
        }
        break;
    case Instruction::Br:
        {
        if (isa<BranchInst>(I) && cast<BranchInst>(I).isConditional()) {
          char temp[MAX_CHAR_BUFFER];
          const BranchInst &BI(cast<BranchInst>(I));
          prepareOperand(BI.getCondition());
          int cond_item = getLocalSlot(BI.getCondition());
          sprintf(temp, "%s" SEPARATOR "%s_cond", globalName, I.getParent()->getName().str().c_str());
          if (slotarray[cond_item].name) {
              sprintf(vout, "%s = %s\n", temp, slotarray[cond_item].name);
              slotarray[cond_item].name = strdup(temp);
          }
          prepareOperand(BI.getSuccessor(0));
          prepareOperand(BI.getSuccessor(1));
        } else if (isa<IndirectBrInst>(I)) {
          for (unsigned i = 0, e = I.getNumOperands(); i != e; ++i) {
            prepareOperand(I.getOperand(i));
          }
        }
        dump_operands = 1;
        }
        break;
    //case Instruction::Switch:
        //const SwitchInst& SI(cast<SwitchInst>(I));
        //prepareOperand(SI.getCondition());
        //prepareOperand(SI.getDefaultDest());
        //for (SwitchInst::ConstCaseIt i = SI.case_begin(), e = SI.case_end(); i != e; ++i) {
          //prepareOperand(i.getCaseValue());
          //prepareOperand(i.getCaseSuccessor());
        //}
    //case Instruction::IndirectBr:
    //case Instruction::Invoke:
    //case Instruction::Resume:
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
    case Instruction::And: case Instruction::Or: case Instruction::Xor:
        {
        const char *op1 = getparam(1), *op2 = getparam(2);
        char temp[MAX_CHAR_BUFFER];
        sprintf(temp, "((%s) %s (%s))", op1, intmap_lookup(opcodeMap, opcode), op2);
        if (operand_list[0].type != OpTypeLocalRef) {
            printf("[%s:%d]\n", __FUNCTION__, __LINE__);
            exit(1);
        }
        slotarray[operand_list[0].value].name = strdup(temp);
        }
        break;

    // Memory instructions...
    case Instruction::Store:
        if (operand_list[1].type == OpTypeLocalRef && !slotarray[operand_list[1].value].svalue)
            operand_list[1].type = OpTypeInt;
        if (operand_list[1].type != OpTypeLocalRef || operand_list[2].type != OpTypeLocalRef)
            sprintf(vout, "%s = %s;", getparam(2), getparam(1));
        else
            slotarray[operand_list[2].value] = slotarray[operand_list[1].value];
        break;
    //case Instruction::AtomicCmpXchg:
    //case Instruction::AtomicRMW:
    //case Instruction::Fence:

    // Convert instructions...
    //case Instruction::SExt:
    //case Instruction::FPTrunc: //case Instruction::FPExt:
    //case Instruction::FPToUI: //case Instruction::FPToSI:
    //case Instruction::UIToFP: //case Instruction::SIToFP:
    //case Instruction::IntToPtr: //case Instruction::PtrToInt:
    case Instruction::Trunc: case Instruction::ZExt: case Instruction::BitCast:
        if(operand_list[0].type != OpTypeLocalRef || operand_list[1].type != OpTypeLocalRef) {
            printf("[%s:%d]\n", __FUNCTION__, __LINE__);
            exit(1);
        }
        slotarray[operand_list[0].value] = slotarray[operand_list[1].value];
        break;
    //case Instruction::AddrSpaceCast:

    // Other instructions...
    case Instruction::ICmp: case Instruction::FCmp:
        {
        const char *op1 = getparam(1), *op2 = getparam(2);
        const CmpInst *CI;
        if (!(CI = dyn_cast<CmpInst>(&I)) || operand_list[0].type != OpTypeLocalRef) {
            printf("[%s:%d]\n", __FUNCTION__, __LINE__);
            exit(1);
        }
        char temp[MAX_CHAR_BUFFER];
        sprintf(temp, "((%s) %s (%s))", op1, intmap_lookup(predText, CI->getPredicate()), op2);
        slotarray[operand_list[0].value].name = strdup(temp);
        }
        break;
    case Instruction::PHI:
        {
        char temp[MAX_CHAR_BUFFER];
        const PHINode *PN = dyn_cast<PHINode>(&I);
        if (!PN) {
            printf("[%s:%d]\n", __FUNCTION__, __LINE__);
            exit(1);
        }
        I.getType()->dump();
        sprintf(temp, "%s" SEPARATOR "%s_phival", globalName, I.getParent()->getName().str().c_str());
        sprintf(vout, "%s = ", temp);
        slotarray[operand_list[0].value].name = strdup(temp);
        for (unsigned op = 0, Eop = PN->getNumIncomingValues(); op < Eop; ++op) {
            int valuein = getLocalSlot(PN->getIncomingValue(op));
            prepareOperand(PN->getIncomingValue(op));
            prepareOperand(PN->getIncomingBlock(op));
            TerminatorInst *TI = PN->getIncomingBlock(op)->getTerminator();
            //printf("[%s:%d] terminator\n", __FUNCTION__, __LINE__);
            //TI->dump();
            const BranchInst *BI = dyn_cast<BranchInst>(TI);
            const char *trailch = "";
            if (isa<BranchInst>(TI) && cast<BranchInst>(TI)->isConditional()) {
              prepareOperand(BI->getCondition());
              int cond_item = getLocalSlot(BI->getCondition());
              sprintf(temp, "%s ?", slotarray[cond_item].name);
              trailch = ":";
              //prepareOperand(BI->getSuccessor(0));
              //prepareOperand(BI->getSuccessor(1));
              strcat(vout, temp);
            }
            if (slotarray[valuein].name)
                sprintf(temp, "%s %s", slotarray[valuein].name, trailch);
            else
                sprintf(temp, "%lld %s", (long long)slotarray[valuein].offset, trailch);
            strcat(vout, temp);
        }
        dump_operands = 1;
        }
        break;
    //case Instruction::Select:
    case Instruction::Call:
        {
        //const CallInst *CI = dyn_cast<CallInst>(&I);
        //const Value *val = CI->getCalledValue();
        int tcall = operand_list[operand_list_index-1].value; // Callee is _last_ operand
        Function *f = (Function *)slotarray[tcall].svalue;
        if (!f) {
            printf("[%s:%d] not an instantiable call!!!!\n", __FUNCTION__, __LINE__);
            break;
        }
        SLOTARRAY_TYPE arg;
        if (operand_list_index > 3)
            arg = slotarray[operand_list[2].value];
else
        vtablework.push_back(VTABLE_WORK(slotarray[tcall].offset/sizeof(uint64_t),
            (Function ***)slotarray[operand_list[1].value].svalue, arg));
        slotarray[operand_list[0].value].name = strdup(f->getName().str().c_str());
        }
        break;
    //case Instruction::VAArg:
    //case Instruction::ExtractElement:
    //case Instruction::InsertElement:
    //case Instruction::ShuffleVector:
    //case Instruction::ExtractValue:
    //case Instruction::InsertValue:
    //case Instruction::LandingPad:
    default:
        printf("Other opcode %d.=%s\n", opcode, I.getOpcodeName());
        exit(1);
        break;
    }
    if (dump_operands && trace_translate)
    for (int i = 0; i < operand_list_index; i++) {
        int t = operand_list[i].value;
        if (operand_list[i].type == OpTypeLocalRef)
            printf(" op[%d]L=%d:%p:%lld:[%p=%s];", i, t, slotarray[t].svalue, (long long)slotarray[t].offset, slotarray[t].name, slotarray[t].name);
        else if (operand_list[i].type != OpTypeNone)
            printf(" op[%d]=%s;", i, getparam(i));
    }
    if (vout[0])
        return vout;
    return NULL;
}
