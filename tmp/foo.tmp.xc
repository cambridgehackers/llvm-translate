

/* Global Variable Definitions and Initialization */
struct l_class_OC_EchoTest echoTest;
unsigned int stop_main_program;
struct l_class_OC_Module *_ZN6Module5firstE;


//******************** vtables for Classes *******************
unsigned char *_ZTVN8EchoTest5driveE[4] = { ((unsigned char *)_ZN8EchoTest5drive5guardEv), ((unsigned char *)_ZN8EchoTest5drive6updateEv) };
unsigned char *_ZTV4Rule[4] = {0 };
unsigned char *_ZTVN4Echo7respond8respond2E[4] = { ((unsigned char *)_ZN4Echo7respond8respond25guardEv), ((unsigned char *)_ZN4Echo7respond8respond26updateEv) };
unsigned char *_ZTVN4Echo7respond8respond1E[4] = { ((unsigned char *)_ZN4Echo7respond8respond15guardEv), ((unsigned char *)_ZN4Echo7respond8respond16updateEv) };
unsigned char *_ZTV5Fifo1IiE[12] = {0 };
unsigned char *_ZTV4FifoIiE[12] = {0 };
typedef unsigned char **RuleVTab;//Rules:
const RuleVTab ruleList[] = {
    _ZTVN8EchoTest5driveE, _ZTVN4Echo7respond8respond2E, _ZTVN4Echo7respond8respond1E, NULL};
void _ZN4Echo7respond8respond26updateEv(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 *Vthis) {
}

bool _ZN4Echo7respond8respond25guardEv(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 *Vthis) {
          return 1;
;
}

void _ZN4Echo7respond8respond16updateEv(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 *Vthis) {
        _ZN5Fifo1IiE3deqEv((struct l_class_OC_Fifo1 *)&echoTest_ZZ_EchoTest_ZZ_echo_ZZ__ZZ_Echo_ZZ_fifo_ZZ__ZZ_Fifo1_int_);
unsigned int Vcall =         _ZN5Fifo1IiE5firstEv((struct l_class_OC_Fifo1 *)&echoTest_ZZ_EchoTest_ZZ_echo_ZZ__ZZ_Echo_ZZ_fifo_ZZ__ZZ_Fifo1_int_);
        _ZN14EchoIndication4echoEi(Vcall);
}

bool _ZN4Echo7respond8respond15guardEv(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 *Vthis) {
bool Vtmp__1 =         _ZN5Fifo1IiE10deq__guardEv((struct l_class_OC_Fifo1 *)&echoTest_ZZ_EchoTest_ZZ_echo_ZZ__ZZ_Echo_ZZ_fifo_ZZ__ZZ_Fifo1_int_);
bool Vtmp__2 =         _ZN5Fifo1IiE12first__guardEv((struct l_class_OC_Fifo1 *)&echoTest_ZZ_EchoTest_ZZ_echo_ZZ__ZZ_Echo_ZZ_fifo_ZZ__ZZ_Fifo1_int_);
          return (Vtmp__1 & Vtmp__2);
;
}

void _ZN8EchoTest5drive6updateEv(struct l_class_OC_EchoTest_KD__KD_drive *Vthis) {
        _ZN5Fifo1IiE3enqEi((struct l_class_OC_Fifo1 *)&echoTest_ZZ_EchoTest_ZZ_echo_ZZ__ZZ_Echo_ZZ_fifo_ZZ__ZZ_Fifo1_int_, 22);
}

bool _ZN8EchoTest5drive5guardEv(struct l_class_OC_EchoTest_KD__KD_drive *Vthis) {
bool Vtmp__1 =         _ZN5Fifo1IiE10enq__guardEv((struct l_class_OC_Fifo1 *)&echoTest_ZZ_EchoTest_ZZ_echo_ZZ__ZZ_Echo_ZZ_fifo_ZZ__ZZ_Fifo1_int_);
          return Vtmp__1;
;
}

void _ZN5Fifo1IiE3deqEv(struct l_class_OC_Fifo1 *Vthis) {
        (Vthis->full) = ((unsigned char )0);
}

unsigned int _ZN5Fifo1IiE5firstEv(struct l_class_OC_Fifo1 *Vthis) {
          return (Vthis->element);
;
}

void _ZN14EchoIndication4echoEi(unsigned int Vv) {
        printf((("Heard an echo: %d\n")), Vv);
        stop_main_program = 1;
}

bool _ZN5Fifo1IiE10deq__guardEv(struct l_class_OC_Fifo1 *Vthis) {
bool Vcall =         _ZNK5Fifo1IiE8notEmptyEv(Vthis);
          return Vcall;
;
}

bool _ZN5Fifo1IiE12first__guardEv(struct l_class_OC_Fifo1 *Vthis) {
bool Vcall =         _ZNK5Fifo1IiE8notEmptyEv(Vthis);
          return Vcall;
;
}

void _ZN5Fifo1IiE3enqEi(struct l_class_OC_Fifo1 *Vthis, unsigned int Vv) {
        (Vthis->element) = Vv;
        (Vthis->full) = ((unsigned char )1);
}

bool _ZN5Fifo1IiE10enq__guardEv(struct l_class_OC_Fifo1 *Vthis) {
bool Vcall =         _ZNK5Fifo1IiE7notFullEv(Vthis);
          return Vcall;
;
}

bool _ZNK5Fifo1IiE8notEmptyEv(struct l_class_OC_Fifo1 *Vthis) {
          return ((bool )(Vthis->full)&1u);
;
}

bool _ZNK5Fifo1IiE7notFullEv(struct l_class_OC_Fifo1 *Vthis) {
          return (((bool )(Vthis->full)&1u) ^ 1);
;
}

