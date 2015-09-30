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
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

#include "declarations.h"

static std::map<const Value *, Value *> cloneVmap;
int trace_clone;

/*
 * clone a DAG from one basic block to another
 */
static Instruction *cloneTree(const Instruction *I, Instruction *insertPoint)
{
    std::string NameSuffix = "foosuff";
    Instruction *NewInst = I->clone();

    if (I->hasName())
        NewInst->setName(I->getName()+NameSuffix);
    for (unsigned OI = 0, E = I->getNumOperands(); OI != E; ++OI) {
        const Value *oval = I->getOperand(OI);
        if (cloneVmap.find(oval) == cloneVmap.end()) {
            if (const Instruction *IC = dyn_cast<Instruction>(oval))
                cloneVmap[oval] = cloneTree(IC, insertPoint);
            else
                continue;
        }
        NewInst->setOperand(OI, cloneVmap[oval]);
    }
    NewInst->insertBefore(insertPoint);
    if (trace_clone)
printf("[%s:%d] %s %d\n", __FUNCTION__, __LINE__, NewInst->getOpcodeName(), NewInst->getNumOperands());
    return NewInst;
}

static void prepareClone(Instruction *TI, const Instruction *I)
{
    cloneVmap.clear();
    Function *TargetF = TI->getParent()->getParent();
    const Function *SourceF = I->getParent()->getParent();
    Function::arg_iterator TargetA = TargetF->arg_begin();
    for (Function::const_arg_iterator AI = SourceF->arg_begin(),
             AE = SourceF->arg_end(); AI != AE; ++AI, ++TargetA)
        cloneVmap[AI] = TargetA;
}
static Instruction *copyFunction(Instruction *TI, const Instruction *I, int methodIndex, Type *returnType)
{
    prepareClone(TI, I);
    if (!returnType)
        return cloneTree(I, TI);
    Instruction *orig_thisp = dyn_cast<Instruction>(I->getOperand(0));
    Instruction *thisp = cloneTree(orig_thisp, TI);
    Type *Params[] = {thisp->getType()};
    Type *castType = PointerType::get(
             PointerType::get(
                 PointerType::get(
                     FunctionType::get(returnType,
                           ArrayRef<Type*>(Params, 1), false),
                     0), 0), 0);
    IRBuilder<> builder(TI->getParent());
    builder.SetInsertPoint(TI);
    Value *vtabbase = builder.CreateLoad(
             builder.CreateBitCast(thisp, castType));
    Value *newCall = builder.CreateCall(
             builder.CreateLoad(
                 builder.CreateConstInBoundsGEP1_32(
                     vtabbase, methodIndex)), thisp);
    return dyn_cast<Instruction>(newCall);
}

/*
 * Perform guard(), confirm() hoisting.  Insert shadow variable access for store.
 */
const char *calculateGuardUpdate(Function ***parent_thisp, Instruction &I)
{
    int opcode = I.getOpcode();
    switch (opcode) {
    // Terminators
    case Instruction::Br:
        if (isa<BranchInst>(I) && cast<BranchInst>(I).isConditional()) {
            const BranchInst &BI(cast<BranchInst>(I));
            prepareOperand(BI.getCondition());
            int cond_item = getLocalSlot(BI.getCondition());
            char temp[MAX_CHAR_BUFFER];
            sprintf(temp, "%s" SEPARATOR "%s_cond", globalName, I.getParent()->getName().str().c_str());
            if (slotarray[cond_item].name) {
                slotarray[cond_item].name = strdup(temp);
            }
            prepareOperand(BI.getSuccessor(0));
            prepareOperand(BI.getSuccessor(1));
        } else if (isa<IndirectBrInst>(I)) {
            for (unsigned i = 0, e = I.getNumOperands(); i != e; ++i)
                prepareOperand(I.getOperand(i));
        }
        break;

    // Memory instructions...
    case Instruction::Store:
        printf("%s: STORE %s=%s\n", __FUNCTION__, getParam(2), getParam(1));
        if (operand_list[1].type == OpTypeLocalRef && !slotarray[operand_list[1].value].svalue)
            operand_list[1].type = OpTypeInt;
        if (operand_list[1].type != OpTypeLocalRef || operand_list[2].type != OpTypeLocalRef)
            printf("%s: STORE %s;", __FUNCTION__, getParam(2));
        else
            slotarray[operand_list[2].value] = slotarray[operand_list[1].value];
        break;

    // Convert instructions...
    case Instruction::Trunc:
    case Instruction::ZExt:
    case Instruction::BitCast:
        slotarray[operand_list[0].value] = slotarray[operand_list[1].value];
        break;

    // Other instructions...
    case Instruction::PHI:
        {
        char temp[MAX_CHAR_BUFFER];
        const PHINode *PN = dyn_cast<PHINode>(&I);
        I.getType()->dump();
        sprintf(temp, "%s" SEPARATOR "%s_phival", globalName, I.getParent()->getName().str().c_str());
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
            }
            if (slotarray[valuein].name)
                sprintf(temp, "%s %s", slotarray[valuein].name, trailch);
            else
                sprintf(temp, "%lld %s", (long long)slotarray[valuein].offset, trailch);
        }
        }
        break;
    case Instruction::Call:
        {
        int guardName = -1, updateName = -1, parentGuardName = -1, parentUpdateName = -1;
        Value *called = I.getOperand(I.getNumOperands()-1);
        const char *cp = called->getName().str().c_str();
        const Function *CF = dyn_cast<Function>(called);
        if (CF && CF->isDeclaration() && !strncmp(cp, "_Z14PIPELINEMARKER", 18)) {
            cloneVmap.clear();
            /* for now, just remove the Call.  Later we will push processing of I.getOperand(0) into another block */
            Function *F = I.getParent()->getParent();
            Module *Mod = F->getParent();
            std::string Fname = F->getName().str();
            std::string otherName = Fname.substr(0, Fname.length() - 8) + "2" + "4bodyEv";
            Function *otherBody = Mod->getFunction(otherName);
            TerminatorInst *TI = otherBody->begin()->getTerminator();
            prepareClone(TI, &I);
            Instruction *IT = dyn_cast<Instruction>(I.getOperand(1));
            Instruction *IC = dyn_cast<Instruction>(I.getOperand(0));
            Instruction *newIC = cloneTree(IC, TI);
            Instruction *newIT = cloneTree(IT, TI);
            printf("[%s:%d] other %s %p\n", __FUNCTION__, __LINE__, otherName.c_str(), otherBody);
            IRBuilder<> builder(TI->getParent());
            builder.SetInsertPoint(TI);
            builder.CreateStore(newIC, newIT);
            IRBuilder<> oldbuilder(I.getParent());
            oldbuilder.SetInsertPoint(&I);
            Value *newLoad = oldbuilder.CreateLoad(IT);
            I.replaceAllUsesWith(newLoad);
            I.eraseFromParent();
            break;
        }
        int tcall = operand_list[operand_list_index-1].value; // Callee is _last_ operand
        Function *f = (Function *)slotarray[tcall].svalue;
        Function ***thisp = (Function ***)slotarray[operand_list[1].value].svalue;
        if (!f) {
            printf("[%s:%d] not an instantiable call!!!!\n", __FUNCTION__, __LINE__);
        }
        else {
            if (trace_translate)
                printf("%s: CALL %d %s %p\n", __FUNCTION__, I.getType()->getTypeID(),
                      f->getName().str().c_str(), thisp);
            const Instruction *IC = dyn_cast<Instruction>(I.getOperand(0));
            const Type *p1 = IC->getOperand(0)->getType()->getPointerElementType();
            const StructType *STy = cast<StructType>(p1->getPointerElementType());
            const char *tname = strdup(STy->getName().str().c_str());
            guardName = lookup_method(tname, "guard");
            updateName = lookup_method(tname, "update");
        }
        printf("[%s:%d] guard %d update %d\n", __FUNCTION__, __LINE__, guardName, updateName);
        const GlobalValue *g = EE->getGlobalValueAtAddress(parent_thisp[0] - 2);
        printf("[%s:%d] %p g %p\n", __FUNCTION__, __LINE__, parent_thisp, g);
        if (g) {
            char temp[MAX_CHAR_BUFFER];
            int status;
            const char *ret = abi::__cxa_demangle(g->getName().str().c_str(), 0, 0, &status);
            sprintf(temp, "class.%s", ret+11);
            parentGuardName = lookup_method(temp, "guard");
            parentUpdateName = lookup_method(temp, "update");
        }
        if (guardName >= 0 && parentGuardName >= 0) {
            Function *peer_guard = parent_thisp[0][parentGuardName];
            TerminatorInst *TI = peer_guard->begin()->getTerminator();
            Instruction *newI = copyFunction(TI, &I, guardName, Type::getInt1Ty(TI->getContext()));
            if (CallInst *nc = dyn_cast<CallInst>(newI))
                nc->addAttribute(AttributeSet::ReturnIndex, Attribute::ZExt);
            Value *cond = TI->getOperand(0);
            const ConstantInt *CI = dyn_cast<ConstantInt>(cond);
            if (CI && CI->getType()->isIntegerTy(1) && CI->getZExtValue())
                TI->setOperand(0, newI);
            else {
                // 'And' return value into condition
                Instruction *newBool = BinaryOperator::Create(Instruction::And, newI, newI, "newand", TI);
                cond->replaceAllUsesWith(newBool);
                // we must set this after the 'replaceAllUsesWith'
                newBool->setOperand(0, cond);
            }
        }
        if (parentUpdateName >= 0) {
            Function *peer_update = parent_thisp[0][parentUpdateName];
            TerminatorInst *TI = peer_update->begin()->getTerminator();
            if (updateName >= 0)
                copyFunction(TI, &I, updateName, Type::getVoidTy(TI->getContext()));
            else if (I.use_empty()) {
                copyFunction(TI, &I, 0, NULL); // Move this call to the 'update()' method
                I.eraseFromParent(); // delete "Call" instruction
            }
        }
//if (operand_list_index <= 3)
        if (f && thisp) {
            vtablework.push_back(VTABLE_WORK(thisp[0][slotarray[tcall].offset/sizeof(uint64_t)],
                thisp,
                (operand_list_index > 3) ? slotarray[operand_list[2].value] : SLOTARRAY_TYPE()));
            slotarray[operand_list[0].value].name = strdup(f->getName().str().c_str());
        }
        }
        break;
    default:
        printf("Other opcode %d.=%s\n", opcode, I.getOpcodeName());
        exit(1);
    case Instruction::Ret: case Instruction::Unreachable:
    case Instruction::Add: case Instruction::FAdd:
    case Instruction::Sub: case Instruction::FSub:
    case Instruction::Mul: case Instruction::FMul:
    case Instruction::UDiv: case Instruction::SDiv: case Instruction::FDiv:
    case Instruction::URem: case Instruction::SRem: case Instruction::FRem:
    case Instruction::And: case Instruction::Or: case Instruction::Xor:
    case Instruction::ICmp:
        break;
    }
    return NULL;
}
