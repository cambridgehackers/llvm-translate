typedef struct l_class_OC_Module {
  struct l_class_OC_Rule *rfirst;
  struct l_class_OC_Module *next;
  struct l_class_OC_Module *shadow;
  unsigned long long size;
} l_class_OC_Module;

typedef struct l_class_OC_Fifo {
  unsigned int  (**Module) ( int, ...);
  struct l_class_OC_Module _vptr_EC_Fifo;
} l_class_OC_Fifo;

typedef struct l_class_OC_Rule {
  unsigned int  (**_vptr_EC_Rule) ( int, ...);
  struct l_class_OC_Rule *next;
} l_class_OC_Rule;

typedef struct l_class_OC_EchoTest_KD__KD_drive {
  struct l_class_OC_Rule Rule;
  struct l_class_OC_EchoTest *module;
} l_class_OC_EchoTest_KD__KD_drive;

typedef struct l_class_OC_EchoTest {
  struct l_class_OC_Module Module;
  struct l_class_OC_Echo *echo;
  unsigned int x;
  struct l_class_OC_EchoTest_KD__KD_drive driveRule;
} l_class_OC_EchoTest;

typedef struct l_class_OC_Fifo1 {
  struct l_class_OC_Fifo Fifo_MD_int_OD_;
  unsigned int element;
  unsigned char full;
} l_class_OC_Fifo1;


/* External Global Variable Declarations */
extern struct l_class_OC_EchoTest echoTest;
extern unsigned char *_ZTVN10__cxxabiv120__si_class_type_infoE;
extern unsigned char *_ZTVN10__cxxabiv117__class_type_infoE;
extern unsigned char *_ZTVN10__cxxabiv121__vmi_class_type_infoE;
extern unsigned int stop_main_program;
extern struct l_class_OC_Module *_ZN6Module5firstE;

/* Function Declarations */
void _ZN14EchoIndication4echoEi(unsigned int Vv);
static void __cxx_global_var_init(void);
void _ZN8EchoTestC1Ev(struct l_class_OC_EchoTest *Vthis);
void _ZN8EchoTestD1Ev(struct l_class_OC_EchoTest *Vthis);
static void __dtor_echoTest(void);
void _ZN8EchoTestD2Ev(struct l_class_OC_EchoTest *Vthis);
void _ZN8EchoTestC2Ev(struct l_class_OC_EchoTest *Vthis);
void _ZN6ModuleC2Em(struct l_class_OC_Module *Vthis, unsigned long long Vsize);
unsigned char *_Znwm(unsigned long long );
void _ZN4EchoC1EP14EchoIndication(struct l_class_OC_Echo *Vthis, struct l_class_OC_EchoIndication *Vind);
void _ZN14EchoIndicationC1Ev(struct l_class_OC_EchoIndication *Vthis);
void _ZN8EchoTest5driveC1EPS_(struct l_class_OC_EchoTest *Vmodule);
void _ZN8EchoTest5driveC2EPS_(struct l_class_OC_EchoTest *Vmodule);
void _ZN4RuleC2Ev(struct l_class_OC_Rule *Vthis);
void _ZN6Module7addRuleEP4Rule(struct l_class_OC_Module *Vthis, struct l_class_OC_Rule *Vrule);
bool _ZN8EchoTest5drive5guardEv(void);
void _ZN8EchoTest5drive6updateEv(void);
void _ZN14EchoIndicationC2Ev(struct l_class_OC_EchoIndication *Vthis);
void _ZN4EchoC2EP14EchoIndication(struct l_class_OC_Echo *Vthis, struct l_class_OC_EchoIndication *Vind);
void _ZN5Fifo1IiEC1Ev(struct l_class_OC_Fifo1 *Vthis);
void _ZN4Echo7respondC1EPS_(struct l_class_OC_Echo_KD__KD_respond *Vthis, struct l_class_OC_Echo *Vmodule);
void _ZN4Echo7respondC2EPS_(struct l_class_OC_Echo_KD__KD_respond *Vthis, struct l_class_OC_Echo *Vmodule);
void _ZN4Echo7respond8respond1C1EPS_(struct l_class_OC_Echo *Vmodule);
void _ZN4Echo7respond8respond2C1EPS_(struct l_class_OC_Echo *Vmodule);
void _ZN4Echo7respond8respond2C2EPS_(struct l_class_OC_Echo *Vmodule);
bool _ZN4Echo7respond8respond25guardEv(void);
void _ZN4Echo7respond8respond26updateEv(void);
void _ZN4Echo7respond8respond1C2EPS_(struct l_class_OC_Echo *Vmodule);
bool _ZN4Echo7respond8respond15guardEv(void);
void _ZN4Echo7respond8respond16updateEv(void);
void _ZN5Fifo1IiEC2Ev(struct l_class_OC_Fifo1 *Vthis);
void _ZN4FifoIiEC2Em(struct l_class_OC_Fifo *Vthis, unsigned long long Vsize);
void _ZN5Fifo1IiED1Ev(struct l_class_OC_Fifo1 *Vthis);
void _ZN5Fifo1IiED0Ev(struct l_class_OC_Fifo1 *Vthis);
bool _ZN5Fifo1IiE10enq__guardEv(struct l_class_OC_Fifo1 *Vthis);
void _ZN5Fifo1IiE3enqEi(struct l_class_OC_Fifo1 *Vthis, unsigned int Vv);
bool _ZN5Fifo1IiE10deq__guardEv(struct l_class_OC_Fifo1 *Vthis);
void _ZN5Fifo1IiE3deqEv(struct l_class_OC_Fifo1 *Vthis);
bool _ZN5Fifo1IiE12first__guardEv(struct l_class_OC_Fifo1 *Vthis);
unsigned int _ZN5Fifo1IiE5firstEv(struct l_class_OC_Fifo1 *Vthis);
bool _ZNK5Fifo1IiE8notEmptyEv(struct l_class_OC_Fifo1 *Vthis);
bool _ZNK5Fifo1IiE7notFullEv(struct l_class_OC_Fifo1 *Vthis);
void _ZdlPv(unsigned char *);
void _ZN5Fifo1IiED2Ev(struct l_class_OC_Fifo1 *Vthis);
void _ZN4FifoIiED2Ev(struct l_class_OC_Fifo *Vthis);
void _ZN4FifoIiED1Ev(struct l_class_OC_Fifo *Vthis);
void _ZN4FifoIiED0Ev(struct l_class_OC_Fifo *Vthis);
bool _ZN4FifoIiE10enq__guardEv(struct l_class_OC_Fifo *Vthis);
bool _ZN4FifoIiE10deq__guardEv(struct l_class_OC_Fifo *Vthis);
bool _ZN4FifoIiE12first__guardEv(struct l_class_OC_Fifo *Vthis);
unsigned char *malloc(unsigned long long );
static void _GLOBAL__I_a(void);
void _Z16run_main_programv(void);
unsigned char *llvm_translate_malloc(unsigned long long );
unsigned char *memset(unsigned char *, unsigned int , unsigned long long );
