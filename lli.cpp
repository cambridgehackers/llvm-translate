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

int dump_ir = 1;

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

//===----------------------------------------------------------------------===//
// main Driver function
//
void dump_type(Module *Mod, const char *p)
{
    StructType *tgv = Mod->getTypeByName(p);
    printf("%s:] ", __FUNCTION__);
    tgv->dump();
    printf(" tgv %p\n", tgv);
}
int main(int argc, char **argv, char * const *envp)
{
DebugFlag = true;
  SMDiagnostic Err;
  std::string ErrorMsg;
  int Result;

printf("[%s:%d] start\n", __FUNCTION__, __LINE__);
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);

  LLVMContext &Context = getGlobalContext();

  cl::ParseCommandLineOptions(argc, argv, "llvm interpreter & dynamic compiler\n");

  // If the user doesn't want core files, disable them.
    //sys::Process::PreventCoreFiles();

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

printf("[%s:%d] before EECREATE\n", __FUNCTION__, __LINE__);
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
   const GlobalValue *g = EE->getGlobalValueAtAddress(modfirst[0]-2);
if (g) g->dump(); printf("[%s:%d] p %p g %p\n", __FUNCTION__, __LINE__, modfirst[0]-2, g);

dump_type(Mod, "class.Rule");
t = (uint64_t ***)modfirst[1];
printf("Rule %p: vtab %p next %p\n", t, t[0], t[1]);
g = EE->getGlobalValueAtAddress(t[0]-2);
if (g) g->dump(); printf("[%s:%d] p %p g %p\n", __FUNCTION__, __LINE__, t[0]-2, g);
for (int i = 0; i < 9; i++) {
   g = EE->getGlobalValueAtAddress(t[0][i-2]);
   if (g) g->dump(); printf("[%s:%d] [%d] %p = %p\n", __FUNCTION__, __LINE__, i, t[0][i-2], g);
}
t = (uint64_t ***)t[1];
printf("Rule %p: vtab %p next %p\n", t, t[0], t[1]);
g = EE->getGlobalValueAtAddress(t[0]-2);
if (g) g->dump(); printf("[%s:%d] p %p g %p\n", __FUNCTION__, __LINE__, t[0]-2, g);

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
