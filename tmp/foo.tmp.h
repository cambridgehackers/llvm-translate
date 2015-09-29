typedef struct l_class_OC_Module {
  struct l_class_OC_Rule *rfirst;
  struct l_class_OC_Module *next;
  struct l_class_OC_Module *shadow;
  unsigned long long size;
} l_class_OC_Module;

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

typedef struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 {
  struct l_class_OC_Rule Rule;
  struct l_class_OC_Echo *module;
} l_class_OC_Echo_KD__KD_respond_KD__KD_respond1;

typedef struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 {
  struct l_class_OC_Rule Rule;
  struct l_class_OC_Echo *module;
} l_class_OC_Echo_KD__KD_respond_KD__KD_respond2;

typedef struct l_class_OC_Echo_KD__KD_respond {
  struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 respond1Rule;
  struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 respond2Rule;
} l_class_OC_Echo_KD__KD_respond;

typedef struct l_class_OC_Echo {
  struct l_class_OC_Module Module;
  struct l_class_OC_Fifo *fifo;
  struct l_class_OC_EchoIndication *ind;
  unsigned int response;
  struct l_class_OC_GuardedValue *firstreq;
  struct l_class_OC_Action *deqreq;
  unsigned int pipetemp;
  struct l_class_OC_Echo_KD__KD_respond respondRule;
  struct l_class_OC_Action *echoreq;
} l_class_OC_Echo;

typedef struct l_class_OC_EchoIndication {
  unsigned char field0;
} l_class_OC_EchoIndication;

typedef struct l_class_OC_Fifo {
  unsigned int  (**Module) ( int, ...);
  struct l_class_OC_Module _vptr_EC_Fifo;
  struct l_class_OC_Action *enq;
  struct l_class_OC_GuardedValue *first;
  struct l_class_OC_Action *deq;
} l_class_OC_Fifo;

typedef struct l_class_OC_Action {
  unsigned int  (**_vptr_EC_Action) ( int, ...);
} l_class_OC_Action;

typedef struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction {
  struct l_class_OC_Action Action_MD_int_OD_;
  struct l_class_OC_Fifo1 *fifo;
  unsigned int elt;
} l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction;

typedef struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction {
  struct l_class_OC_Action Action_MD_int_OD_;
  struct l_class_OC_Fifo1 *fifo;
} l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction;

typedef struct l_class_OC_GuardedValue {
  unsigned int  (**_vptr_EC_GuardedValue) ( int, ...);
} l_class_OC_GuardedValue;

typedef struct l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue {
  struct l_class_OC_GuardedValue GuardedValue_MD_int_OD_;
  struct l_class_OC_Fifo1 *fifo;
} l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue;

typedef struct l_class_OC_Fifo1 {
  struct l_class_OC_Fifo Fifo_MD_int_OD_;
  unsigned int element;
  unsigned char full;
  struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction enqAction;
  struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction deqAction;
  struct l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue firstValue;
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
void _ZN8EchoTest5driveC1EPS_(struct l_class_OC_EchoTest_KD__KD_drive *Vthis, struct l_class_OC_EchoTest *Vmodule);
void _ZN8EchoTest5driveC2EPS_(struct l_class_OC_EchoTest_KD__KD_drive *Vthis, struct l_class_OC_EchoTest *Vmodule);
void _ZN4RuleC2Ev(struct l_class_OC_Rule *Vthis);
void _ZN6Module7addRuleEP4Rule(struct l_class_OC_Module *Vthis, struct l_class_OC_Rule *Vrule);
bool _ZN8EchoTest5drive5guardEv(struct l_class_OC_EchoTest_KD__KD_drive *Vthis);
void _ZN8EchoTest5drive4bodyEv(struct l_class_OC_EchoTest_KD__KD_drive *Vthis);
void _ZN8EchoTest5drive6updateEv(struct l_class_OC_EchoTest_KD__KD_drive *Vthis);
void _ZN14EchoIndicationC2Ev(struct l_class_OC_EchoIndication *Vthis);
void _ZN4EchoC2EP14EchoIndication(struct l_class_OC_Echo *Vthis, struct l_class_OC_EchoIndication *Vind);
void _ZN5Fifo1IiEC1Ev(struct l_class_OC_Fifo1 *Vthis);
void _ZN4Echo7respondC1EPS_(struct l_class_OC_Echo_KD__KD_respond *Vthis, struct l_class_OC_Echo *Vmodule);
void _ZN4Echo7respondC2EPS_(struct l_class_OC_Echo_KD__KD_respond *Vthis, struct l_class_OC_Echo *Vmodule);
void _ZN4Echo7respond8respond1C1EPS_(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 *Vthis, struct l_class_OC_Echo *Vmodule);
void _ZN4Echo7respond8respond2C1EPS_(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 *Vthis, struct l_class_OC_Echo *Vmodule);
void _ZN4Echo7respond8respond2C2EPS_(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 *Vthis, struct l_class_OC_Echo *Vmodule);
bool _ZN4Echo7respond8respond25guardEv(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 *Vthis);
void _ZN4Echo7respond8respond24bodyEv(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 *Vthis);
void _ZN4Echo7respond8respond26updateEv(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 *Vthis);
void _ZN4Echo7respond8respond1C2EPS_(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 *Vthis, struct l_class_OC_Echo *Vmodule);
bool _ZN4Echo7respond8respond15guardEv(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 *Vthis);
void _ZN4Echo7respond8respond14bodyEv(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 *Vthis);
void _ZN4Echo7respond8respond16updateEv(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 *Vthis);
unsigned int _Z14PIPELINEMARKERIiET_S0_RS0_(unsigned int , unsigned int *);
void _ZN5Fifo1IiEC2Ev(struct l_class_OC_Fifo1 *Vthis);
void _ZN4FifoIiEC2EmP6ActionIiEP12GuardedValueIiES3_(struct l_class_OC_Fifo *Vthis, unsigned long long Vsize, struct l_class_OC_Action *Venq, struct l_class_OC_GuardedValue *Vfirst, struct l_class_OC_Action *Vdeq);
void _ZN5Fifo1IiE9EnqActionC1EPS0_(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *Vthis, struct l_class_OC_Fifo1 *Vfifo);
void _ZN5Fifo1IiE9DeqActionC1EPS0_(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *Vthis, struct l_class_OC_Fifo1 *Vfifo);
void _ZN5Fifo1IiE10FirstValueC1EPS0_(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue *Vthis, struct l_class_OC_Fifo1 *Vfifo);
void _ZN5Fifo1IiED1Ev(struct l_class_OC_Fifo1 *Vthis);
void _ZN5Fifo1IiED0Ev(struct l_class_OC_Fifo1 *Vthis);
bool _ZNK5Fifo1IiE8notEmptyEv(struct l_class_OC_Fifo1 *Vthis);
bool _ZNK5Fifo1IiE7notFullEv(struct l_class_OC_Fifo1 *Vthis);
void _ZdlPv(unsigned char *);
void _ZN5Fifo1IiED2Ev(struct l_class_OC_Fifo1 *Vthis);
void _ZN4FifoIiED2Ev(struct l_class_OC_Fifo *Vthis);
void _ZN5Fifo1IiE10FirstValueC2EPS0_(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue *Vthis, struct l_class_OC_Fifo1 *Vfifo);
void _ZN12GuardedValueIiEC2Ev(struct l_class_OC_GuardedValue *Vthis);
bool _ZN5Fifo1IiE10FirstValue5guardEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue *Vthis);
unsigned int _ZN5Fifo1IiE10FirstValue5valueEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue *Vthis);
void _ZN5Fifo1IiE9DeqActionC2EPS0_(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *Vthis, struct l_class_OC_Fifo1 *Vfifo);
void _ZN6ActionIiEC2Ev(struct l_class_OC_Action *Vthis);
bool _ZN5Fifo1IiE9DeqAction5guardEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *Vthis);
void _ZN5Fifo1IiE9DeqAction4bodyEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *Vthis);
void _ZN5Fifo1IiE9DeqAction4bodyEi(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *Vthis, unsigned int Vv);
void _ZN5Fifo1IiE9DeqAction6updateEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *Vthis);
bool _ZN6ActionIiE5guardEv(struct l_class_OC_Action *Vthis);
void _ZN5Fifo1IiE9EnqActionC2EPS0_(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *Vthis, struct l_class_OC_Fifo1 *Vfifo);
bool _ZN5Fifo1IiE9EnqAction5guardEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *Vthis);
void _ZN5Fifo1IiE9EnqAction4bodyEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *Vthis);
void _ZN5Fifo1IiE9EnqAction4bodyEi(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *Vthis, unsigned int Vv);
void _ZN5Fifo1IiE9EnqAction6updateEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *Vthis);
void _ZN4FifoIiED1Ev(struct l_class_OC_Fifo *Vthis);
void _ZN4FifoIiED0Ev(struct l_class_OC_Fifo *Vthis);
unsigned char *malloc(unsigned long long );
static void _GLOBAL__I_a(void);
void _Z16run_main_programv(void);
unsigned char *llvm_translate_malloc(unsigned long long );
unsigned char *memset(unsigned char *, unsigned int , unsigned long long );
