

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
void _ZN4Echo7respond8respond26updateEv(void) {
}

bool _ZN4Echo7respond8respond25guardEv(void) {
          return 1;
;
}

void _ZN4Echo7respond8respond16updateEv(void) {
        _ZN5Fifo1IiE3deqEv((struct l_class_OC_Fifo1 *)&echoTest_ZZ_EchoTest_ZZ_echo_ZZ__ZZ_Echo_ZZ_fifo_ZZ__ZZ_Fifo1_int_);
unsigned int Vcall =         _ZN5Fifo1IiE5firstEv((struct l_class_OC_Fifo1 *)&echoTest_ZZ_EchoTest_ZZ_echo_ZZ__ZZ_Echo_ZZ_fifo_ZZ__ZZ_Fifo1_int_);
        _ZN14EchoIndication4echoEi(Vcall);
}

bool _ZN4Echo7respond8respond15guardEv(void) {
          return 1;
;
}

void _ZN8EchoTest5drive6updateEv(void) {
        _ZN5Fifo1IiE3enqEi((struct l_class_OC_Fifo1 *)&echoTest_ZZ_EchoTest_ZZ_echo_ZZ__ZZ_Echo_ZZ_fifo_ZZ__ZZ_Fifo1_int_, 22);
}

bool _ZN8EchoTest5drive5guardEv(void) {
          return 1;
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

void _ZN5Fifo1IiE3enqEi(struct l_class_OC_Fifo1 *Vthis, unsigned int Vv) {
        (Vthis->element) = Vv;
        (Vthis->full) = ((unsigned char )1);
}

