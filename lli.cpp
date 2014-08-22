//===- lli.cpp - LLVM Interpreter / Dynamic compiler ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This utility provides a simple wrapper around the LLVM Execution Engines,
// which allow the direct execution of LLVM programs through a Just-In-Time
// compiler, or through an interpreter if no JIT is available for this platform.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "lli"
#include "llvm/IR/LLVMContext.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/Memory.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Instrumentation.h"
#include <cerrno>

int dump_ir;// = 1;
int dump_interpret;// = 1;

using namespace llvm;

namespace {
  cl::opt<std::string>
  InputFile(cl::desc("<input bitcode>"), cl::Positional, cl::init("-"));

  cl::list<std::string>
  InputArgv(cl::ConsumeAfter, cl::desc("<program arguments>..."));

  cl::opt<std::string>
  MArch("march",
        cl::desc("Architecture to generate assembly for (see --version)"));

  cl::list<std::string>
  MAttrs("mattr",
         cl::CommaSeparated,
         cl::desc("Target specific attributes (-mattr=help for details)"),
         cl::value_desc("a1,+a2,-a3,..."));

  cl::list<std::string>
  ExtraModules("extra-module",
         cl::desc("Extra modules to be loaded"),
         cl::value_desc("input bitcode"));
}

static ExecutionEngine *EE = 0;

static void memdump(void *p, int len, const char *title);

void dump_vtab(uint64_t **vtab)
{
int arr_size = 0;
    const GlobalValue *g = EE->getGlobalValueAtAddress(vtab-2);
    printf("[%s:%d] vtabbase %p g %p:\n", __FUNCTION__, __LINE__, vtab-2, g);
    if (g) {
        if (g->getType()->getTypeID() == Type::PointerTyID) {
           Type *ty = g->getType()->getElementType();
           if (ty->getTypeID() == Type::ArrayTyID) {
               ArrayType *aty = cast<ArrayType>(ty);
               arr_size = aty->getNumElements();
           }
        }
        //g->getType()->dump();
    }
    for (int i = 0; i < arr_size-2; i++) {
       Function *f = (Function *)(vtab[i]);
       printf("[%s:%d] [%d] p %p: %s\n", __FUNCTION__, __LINE__, i, vtab[i], f->getName().str().c_str());
    }
}

void dump_type(Module *Mod, const char *p)
{
    StructType *tgv = Mod->getTypeByName(p);
    printf("%s:] ", __FUNCTION__);
    tgv->dump();
    printf(" tgv %p\n", tgv);
}

static void memdump(void *p, int len, const char *title)
{
int i;

    i = 0;
    while (len > 0) {
        if (!(i & 0xf)) {
            if (i > 0)
                printf("\n");
            printf("%s: ",title);
        }
        printf("%02x ", *(unsigned char *)p);
        p = 1 + (char *)p;
        i++;
        len--;
    }
    printf("\n");
}

int main(int argc, char **argv, char * const *envp)
{
  SMDiagnostic Err;
  std::string ErrorMsg;
  int Result;

printf("[%s:%d] start\n", __FUNCTION__, __LINE__);
  if (dump_interpret)
      DebugFlag = true;
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);

  LLVMContext &Context = getGlobalContext();

  cl::ParseCommandLineOptions(argc, argv, "llvm interpreter & dynamic compiler\n");

  // Load the bitcode...
  Module *Mod = ParseIRFile(InputFile, Err, Context);
  if (!Mod) {
    Err.print(argv[0], errs());
    return 1;
  }

  // If not jitting lazily, load the whole bitcode file eagerly too.
    if (Mod->MaterializeAllPermanently(&ErrorMsg)) {
      errs() << argv[0] << ": bitcode didn't read correctly.\n";
      errs() << "Reason: " << ErrorMsg << "\n";
      exit(1);
    }

  if (dump_ir) {
    ModulePass *DebugIRPass = createDebugIRPass();
    DebugIRPass->runOnModule(*Mod);
  }

  EngineBuilder builder(Mod);
  builder.setMArch(MArch);
  builder.setMCPU("");
  builder.setMAttrs(MAttrs);
  builder.setRelocationModel(Reloc::Default);
  builder.setCodeModel(CodeModel::JITDefault);
  builder.setErrorStr(&ErrorMsg);
  builder.setEngineKind(EngineKind::Interpreter);
  builder.setJITMemoryManager(0);
  builder.setOptLevel(CodeGenOpt::None);

  TargetOptions Options;
  Options.UseSoftFloat = false;
  Options.JITEmitDebugInfo = true;
  Options.JITEmitDebugInfoToDisk = false;

  builder.setTargetOptions(Options);

  EE = builder.create();
  if (!EE) {
    if (!ErrorMsg.empty())
      errs() << argv[0] << ": error creating EE: " << ErrorMsg << "\n";
    else
      errs() << argv[0] << ": unknown error creating EE!\n";
    exit(1);
  }

  // Load any additional modules specified on the command line.
  for (unsigned i = 0, e = ExtraModules.size(); i != e; ++i) {
    Module *XMod = ParseIRFile(ExtraModules[i], Err, Context);
    if (!XMod) {
      Err.print(argv[0], errs());
      return 1;
    }
    EE->addModule(XMod);
  }

  EE->DisableLazyCompilation(true); //NoLazyCompilation);

  // Add the module's name to the start of the vector of arguments to main().
  InputArgv.insert(InputArgv.begin(), InputFile);

  // Call the main function from M as if its signature were:
  //   int main (int argc, char **argv, const char **envp)
  // using the contents of Args to determine argc & argv, and the contents of
  // EnvVars to determine envp.
  //
  Function *EntryFn = Mod->getFunction("main");
  if (!EntryFn) {
    errs() << "\'main\' function not found in module.\n";
    return -1;
  }

  // Reset errno to zero on entry to main.
  errno = 0;

    // If the program doesn't explicitly call exit, we will need the Exit
    // function later on to make an explicit call, so get the function now.
    Constant *Exit = Mod->getOrInsertFunction("exit", Type::getVoidTy(Context),
                                                      Type::getInt32Ty(Context),
                                                      NULL);

    // Run static constructors.
    EE->runStaticConstructorsDestructors(false);
printf("[%s:%d] after staric constructors\n", __FUNCTION__, __LINE__);

      for (Module::iterator I = Mod->begin(), E = Mod->end(); I != E; ++I) {
        Function *Fn = &*I;
        if (Fn != EntryFn && !Fn->isDeclaration())
          EE->getPointerToFunction(Fn);
      }

    // Trigger compilation separately so code regions that need to be 
    // invalidated will be known.
    (void)EE->getPointerToFunction(EntryFn);

    // Run main.
uint64_t ***t;
    Result = EE->runFunctionAsMain(EntryFn, InputArgv, envp);
std::string Name = "_ZN6Module5firstE";
GlobalValue *gv = Mod->getNamedValue(Name);
printf("\n\n");
gv->dump();
printf("[%s:%d] gv %p\n", __FUNCTION__, __LINE__, gv);
printf("[%s:%d] gvname %s\n", __FUNCTION__, __LINE__, gv->getName().str().c_str());
printf("[%s:%d] gvtype %p\n", __FUNCTION__, __LINE__, gv->getType());
GenericValue *Ptr = (GenericValue *)EE->getPointerToGlobal(gv);
printf("[%s:%d] ptr %p\n", __FUNCTION__, __LINE__, Ptr);
uint64_t **modfirst = (uint64_t **)*(PointerTy*)Ptr;
printf("[%s:%d] value of Module::first %p\n", __FUNCTION__, __LINE__, modfirst);
dump_type(Mod, "class.Module");
printf("Module vtab %p rfirst %p next %p\n\n", modfirst[0], modfirst[1], modfirst[2]);
dump_vtab((uint64_t **)modfirst[0]);

dump_type(Mod, "class.Rule");
t = (uint64_t ***)modfirst[1];
printf("Rule %p: vtab %p next %p\n", t, t[0], t[1]);
dump_vtab(t[0]);
t = (uint64_t ***)t[1];
printf("Rule %p: vtab %p next %p\n", t, t[0], t[1]);
dump_vtab(t[0]);

  Function *guard = Mod->getFunction("_ZN5Count5count5guardEv"); //Count::done::guard");
printf("[%s:%d] guard %p\n", __FUNCTION__, __LINE__, guard);
if (guard) guard->dump();

#if 0
    // Run static destructors.
    EE->runStaticConstructorsDestructors(true);

    // If the program didn't call exit explicitly, we should call it now.
    // This ensures that any atexit handlers get called correctly.
    if (Function *ExitF = dyn_cast<Function>(Exit)) {
      std::vector<GenericValue> Args;
      GenericValue ResultGV;
      ResultGV.IntVal = APInt(32, Result);
      Args.push_back(ResultGV);
      EE->runFunction(ExitF, Args);
      errs() << "ERROR: exit(" << Result << ") returned!\n";
      abort();
    } else {
      errs() << "ERROR: exit defined with wrong prototype!\n";
      abort();
    }
#endif

printf("[%s:%d] end\n", __FUNCTION__, __LINE__);
  return Result;
}

void printArgument(const Argument *Arg, AttributeSet Attrs, unsigned Idx) {
  //TypePrinter.print(Arg->getType());
  //if (Attrs.hasAttributes(Idx))
    //Attrs.getAsString(Idx);
  //if (Arg->hasName()) {
    //PrintLLVMName(Arg);
  //}
}

void writeOperand(const Value *Operand, bool PrintType) {
  if (Operand == 0) {
    return;
  }
  if (PrintType) {
    //TypePrinter.print(Operand->getType());
  }
  //WriteAsOperandInternal(Operand, &TypePrinter, &Machine, TheModule);
}

// This member is called for each Instruction in a function..
void printInstruction(const Instruction &I) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
  // Print out indentation for an instruction.
  // Print out name if it exists...
  if (I.hasName()) {
    //PrintLLVMName(&I);
  } else if (!I.getType()->isVoidTy()) {
    // Print out the def slot taken.
    //int SlotNum = Machine.getLocalSlot(&I);
    //if (SlotNum == -1)
      //printf("<badref> = ");
    //else
      //printf("%d =",  SlotNum);
  }
  if (isa<CallInst>(I) && cast<CallInst>(I).isTailCall())
    printf("tail ");
  //printf(I.getOpcodeName());
  // If this is an atomic load or store, print out the atomic marker.
  if ((isa<LoadInst>(I)  && cast<LoadInst>(I).isAtomic()) ||
      (isa<StoreInst>(I) && cast<StoreInst>(I).isAtomic()))
    printf(" atomic");
  // If this is a volatile operation, print out the volatile marker.
  if ((isa<LoadInst>(I)  && cast<LoadInst>(I).isVolatile()) ||
      (isa<StoreInst>(I) && cast<StoreInst>(I).isVolatile()) ||
      (isa<AtomicCmpXchgInst>(I) && cast<AtomicCmpXchgInst>(I).isVolatile()) ||
      (isa<AtomicRMWInst>(I) && cast<AtomicRMWInst>(I).isVolatile()))
    printf(" volatile");
  // Print out optimization information.
  //WriteOptimizationInfo(&I);
  // Print out the compare instruction predicates
  //if (const CmpInst *CI = dyn_cast<CmpInst>(&I))
    //printf(' ' << getPredicateText(CI->getPredicate()));
  // Print out the atomicrmw operation
  //if (const AtomicRMWInst *RMWI = dyn_cast<AtomicRMWInst>(&I))
    //writeAtomicRMWOperation(RMWI->getOperation());
  // Print out the type of the operands...
  const Value *Operand = I.getNumOperands() ? I.getOperand(0) : 0;
  // Special case conditional branches to swizzle the condition out to the front
  if (isa<BranchInst>(I) && cast<BranchInst>(I).isConditional()) {
    const BranchInst &BI(cast<BranchInst>(I));
    writeOperand(BI.getCondition(), true);
    writeOperand(BI.getSuccessor(0), true);
    writeOperand(BI.getSuccessor(1), true);
  } else if (isa<SwitchInst>(I)) {
    const SwitchInst& SI(cast<SwitchInst>(I));
    // Special case switch instruction to get formatting nice and correct.
    writeOperand(SI.getCondition(), true);
    writeOperand(SI.getDefaultDest(), true);
    printf(" [");
    for (SwitchInst::ConstCaseIt i = SI.case_begin(), e = SI.case_end(); i != e; ++i) {
      writeOperand(i.getCaseValue(), true);
      writeOperand(i.getCaseSuccessor(), true);
    }
  } else if (isa<IndirectBrInst>(I)) {
    // Special case indirectbr instruction to get formatting nice and correct.
    writeOperand(Operand, true);
    for (unsigned i = 1, e = I.getNumOperands(); i != e; ++i) {
      writeOperand(I.getOperand(i), true);
    }
  } else if (const PHINode *PN = dyn_cast<PHINode>(&I)) {
    //TypePrinter.print(I.getType());
    for (unsigned op = 0, Eop = PN->getNumIncomingValues(); op < Eop; ++op) {
      writeOperand(PN->getIncomingValue(op), false);
      writeOperand(PN->getIncomingBlock(op), false);
    }
  //} else if (const ExtractValueInst *EVI = dyn_cast<ExtractValueInst>(&I)) {
    //writeOperand(I.getOperand(0), true);
    //for (const unsigned *i = EVI->idx_begin(), *e = EVI->idx_end(); i != e; ++i)
      //printf(", " << *i);
  //} else if (const InsertValueInst *IVI = dyn_cast<InsertValueInst>(&I)) {
    //writeOperand(I.getOperand(0), true);
    //writeOperand(I.getOperand(1), true);
    //for (const unsigned *i = IVI->idx_begin(), *e = IVI->idx_end(); i != e; ++i)
      //printf(", " << *i);
  } else if (const LandingPadInst *LPI = dyn_cast<LandingPadInst>(&I)) {
    //TypePrinter.print(I.getType());
    printf(" personality ");
    writeOperand(I.getOperand(0), true);
    if (LPI->isCleanup())
      printf("          cleanup");
    for (unsigned i = 0, e = LPI->getNumClauses(); i != e; ++i) {
      if (i != 0 || LPI->isCleanup()) printf("\n");
      if (LPI->isCatch(i))
        printf("          catch ");
      else
        printf("          filter ");
      writeOperand(LPI->getClause(i), true);
    }
  } else if (isa<ReturnInst>(I) && !Operand) {
    printf(" void");
  } else if (const CallInst *CI = dyn_cast<CallInst>(&I)) {
    // Print the calling convention being used.
    if (CI->getCallingConv() != CallingConv::C) {
      printf(" ");
      //PrintCallingConv(CI->getCallingConv());
    }
    Operand = CI->getCalledValue();
    PointerType *PTy = cast<PointerType>(Operand->getType());
    FunctionType *FTy = cast<FunctionType>(PTy->getElementType());
    Type *RetTy = FTy->getReturnType();
    //const AttributeSet &PAL = CI->getAttributes();
    //if (PAL.hasAttributes(AttributeSet::ReturnIndex))
      //printf(' ' << PAL.getAsString(AttributeSet::ReturnIndex));
    // If possible, print out the short form of the call instruction.  We can
    // only do this if the first argument is a pointer to a nonvararg function,
    // and if the return type is not a pointer to a function.
    //
    if (!FTy->isVarArg() &&
        (!RetTy->isPointerTy() ||
         !cast<PointerType>(RetTy)->getElementType()->isFunctionTy())) {
      //TypePrinter.print(RetTy);
      writeOperand(Operand, false);
    } else {
      writeOperand(Operand, true);
    }
    for (unsigned op = 0, Eop = CI->getNumArgOperands(); op < Eop; ++op) {
      ///writeParamOperand(CI->getArgOperand(op), PAL, op + 1);
    }
    //if (PAL.hasAttributes(AttributeSet::FunctionIndex))
      //printf(" #" < < Machine.getAttributeGroupSlot(PAL.getFnAttributes()));
  } else if (const InvokeInst *II = dyn_cast<InvokeInst>(&I)) {
    Operand = II->getCalledValue();
    PointerType *PTy = cast<PointerType>(Operand->getType());
    FunctionType *FTy = cast<FunctionType>(PTy->getElementType());
    Type *RetTy = FTy->getReturnType();
    //const AttributeSet &PAL = II->getAttributes();
    // Print the calling convention being used.
    if (II->getCallingConv() != CallingConv::C) {
      //PrintCallingConv(II->getCallingConv());
    }
    //if (PAL.hasAttributes(AttributeSet::ReturnIndex))
      //printf(' ' < < PAL.getAsString(AttributeSet::ReturnIndex));
    // If possible, print out the short form of the invoke instruction. We can
    // only do this if the first argument is a pointer to a nonvararg function,
    // and if the return type is not a pointer to a function.
    //
    if (!FTy->isVarArg() &&
        (!RetTy->isPointerTy() ||
         !cast<PointerType>(RetTy)->getElementType()->isFunctionTy())) {
      //TypePrinter.print(RetTy);
      writeOperand(Operand, false);
    } else {
      writeOperand(Operand, true);
    }
    for (unsigned op = 0, Eop = II->getNumArgOperands(); op < Eop; ++op) {
      //writeParamOperand(II->getArgOperand(op), PAL, op + 1);
    }
    //if (PAL.hasAttributes(AttributeSet::FunctionIndex))
      //printf(" #" < < Machine.getAttributeGroupSlot(PAL.getFnAttributes()));
    writeOperand(II->getNormalDest(), true);
    writeOperand(II->getUnwindDest(), true);
  } else if (const AllocaInst *AI = dyn_cast<AllocaInst>(&I)) {
    //TypePrinter.print(AI->getAllocatedType());
    if (!AI->getArraySize() || AI->isArrayAllocation()) {
      writeOperand(AI->getArraySize(), true);
    }
    //if (AI->getAlignment()) {
      //printf(", align " << AI->getAlignment());
    //}
  } else if (isa<CastInst>(I)) {
    if (Operand) {
      writeOperand(Operand, true);   // Work with broken code
    }
    //TypePrinter.print(I.getType());
  } else if (isa<VAArgInst>(I)) {
    if (Operand) {
      writeOperand(Operand, true);   // Work with broken code
    }
    //TypePrinter.print(I.getType());
  } else if (Operand) {   // Print the normal way.
    bool PrintAllTypes = false;
    Type *TheType = Operand->getType();
    // Select, Store and ShuffleVector always print all types.
    if (isa<SelectInst>(I) || isa<StoreInst>(I) || isa<ShuffleVectorInst>(I) || isa<ReturnInst>(I)) {
      PrintAllTypes = true;
    } else {
      for (unsigned i = 1, E = I.getNumOperands(); i != E; ++i) {
        Operand = I.getOperand(i);
        // note that Operand shouldn't be null, but the test helps make dump()
        // more tolerant of malformed IR
        if (Operand && Operand->getType() != TheType) {
          PrintAllTypes = true;    // We have differing types!  Print them all!
          break;
        }
      }
    }
    if (!PrintAllTypes) {
      //TypePrinter.print(TheType);
    }
    for (unsigned i = 0, E = I.getNumOperands(); i != E; ++i) {
      writeOperand(I.getOperand(i), PrintAllTypes);
    }
  }
  // Print atomic ordering/alignment for memory operations
  //if (const LoadInst *LI = dyn_cast<LoadInst>(&I)) {
    //if (LI->isAtomic())
      //writeAtomic(LI->getOrdering(), LI->getSynchScope());
    //if (LI->getAlignment())
      //printf(", align " << LI->getAlignment());
  //} else if (const StoreInst *SI = dyn_cast<StoreInst>(&I)) {
    //if (SI->isAtomic())
      //writeAtomic(SI->getOrdering(), SI->getSynchScope());
    //if (SI->getAlignment())
      //printf(", align " << SI->getAlignment());
  //} else if (const AtomicCmpXchgInst *CXI = dyn_cast<AtomicCmpXchgInst>(&I)) {
    //writeAtomic(CXI->getOrdering(), CXI->getSynchScope());
  //} else if (const AtomicRMWInst *RMWI = dyn_cast<AtomicRMWInst>(&I)) {
    //writeAtomic(RMWI->getOrdering(), RMWI->getSynchScope());
  //} else if (const FenceInst *FI = dyn_cast<FenceInst>(&I)) {
    //writeAtomic(FI->getOrdering(), FI->getSynchScope());
  //}
  // Print Metadata info.
  SmallVector<std::pair<unsigned, MDNode*>, 4> InstMD;
  I.getAllMetadata(InstMD);
  if (!InstMD.empty()) {
    SmallVector<StringRef, 8> MDNames;
    I.getType()->getContext().getMDKindNames(MDNames);
    for (unsigned i = 0, e = InstMD.size(); i != e; ++i) {
      //unsigned Kind = InstMD[i].first;
       //if (Kind < MDNames.size()) {
         //printf(", !" << MDNames[Kind]);
       //} else {
         //printf(", !<unknown kind #" << Kind << ">");
       //}
      //WriteAsOperandInternal(InstMD[i].second, &TypePrinter, &Machine, TheModule);
    }
  }
  //printInfoComment(I);
}

void printBasicBlock(const BasicBlock *BB) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
  if (BB->hasName()) {              // Print out the label if it exists...
    //PrintLLVMName(BB->getName(), LabelPrefix);
  } else if (!BB->use_empty()) {      // Don't print block # of no uses...
    //printf("\n); <label>:";
    //int Slot = Machine.getLocalSlot(BB);
    //if (Slot != -1)
      //printf(Slot);
    //else
      //printf("<badref>");
  }
  //if (BB->getParent() == 0) {
    //printf("); Error: Block without parent!");
  //} else if (BB != &BB->getParent()->getEntryBlock()) {  // Not the entry block?
    //const_pred_iterator PI = pred_begin(BB), PE = pred_end(BB);
    //if (PI == PE) {
    //} else {
      //writeOperand(*PI, false);
      //for (++PI; PI != PE; ++PI) {
        //writeOperand(*PI, false);
      //}
    //}
  //}
  for (BasicBlock::const_iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
    printInstruction(*I);
  }
}

void printFunction(const Function *F) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
  //if (F->isMaterializable())
    //printf("); Materializable\n";
  const AttributeSet &Attrs = F->getAttributes();
  if (Attrs.hasAttributes(AttributeSet::FunctionIndex)) {
    AttributeSet AS = Attrs.getFnAttributes();
    std::string AttrStr;
    unsigned Idx = 0;
    for (unsigned E = AS.getNumSlots(); Idx != E; ++Idx)
      if (AS.getSlotIndex(Idx) == AttributeSet::FunctionIndex)
        break;
    for (AttributeSet::iterator I = AS.begin(Idx), E = AS.end(Idx); I != E; ++I) {
      Attribute Attr = *I;
      if (!Attr.isStringAttribute()) {
        if (!AttrStr.empty()) AttrStr += ' ';
        AttrStr += Attr.getAsString();
      }
    }
    //if (!AttrStr.empty())
      //printf("); Function Attrs: " << AttrStr << '\n';
  }
  if (F->isDeclaration())
    printf("declare ");
  else
    printf("define ");
  //PrintLinkage(F->getLinkage());
  //PrintVisibility(F->getVisibility());
  // Print the calling convention.
  if (F->getCallingConv() != CallingConv::C) {
    //PrintCallingConv(F->getCallingConv());
  }
  FunctionType *FT = F->getFunctionType();
  //if (Attrs.hasAttributes(AttributeSet::ReturnIndex))
    //printf( Attrs.getAsString(AttributeSet::ReturnIndex) << ' ');
  //TypePrinter.print(F->getReturnType());
  //WriteAsOperandInternal(F, &TypePrinter, &Machine, F->getParent());
  //Machine.incorporateFunction(F);
  // Loop over the arguments, printing them...
  unsigned Idx = 1;
  if (!F->isDeclaration()) {
    // If this isn't a declaration, print the argument names as well.
    for (Function::const_arg_iterator I = F->arg_begin(), E = F->arg_end(); I != E; ++I) {
      printArgument(I, Attrs, Idx);
      Idx++;
    }
  } else {
    // Otherwise, print the types from the function type.
    for (unsigned i = 0, e = FT->getNumParams(); i != e; ++i) {
      //TypePrinter.print(FT->getParamType(i));
      //if (Attrs.hasAttributes(i+1))
        //printf(' ' << Attrs.getAsString(i+1));
    }
  }
  // Finish printing arguments...
  if (FT->isVarArg()) {
  }
  if (F->hasUnnamedAddr())
    printf(" unnamed_addr");
  //if (Attrs.hasAttributes(AttributeSet::FunctionIndex))
    //printf(" #" << Machine.getAttributeGroupSlot(Attrs.getFnAttributes()));
  if (F->hasSection()) {
    //PrintEscapedString(F->getSection());
  }
  //if (F->getAlignment())
    //printf(" align " << F->getAlignment());
  //if (F->hasGC())
    //printf(" gc \"" << F->getGC() << '"');
  if (F->hasPrefixData()) {
    printf(" prefix ");
    writeOperand(F->getPrefixData(), true);
  }
  if (F->isDeclaration()) {
  } else {
    for (Function::const_iterator I = F->begin(), E = F->end(); I != E; ++I)
      printBasicBlock(I);
  }
  //Machine.purgeFunction();
}
