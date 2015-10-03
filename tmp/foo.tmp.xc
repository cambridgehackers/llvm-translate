

/* Global Variable Definitions and Initialization */
struct l_class_OC_EchoTest echoTest;
unsigned int stop_main_program;
struct l_class_OC_Module *_ZN6Module5firstE;


//******************** vtables for Classes *******************
unsigned char *_ZTVN8EchoTest5driveE[5] = { ((unsigned char *)_ZN8EchoTest5drive5guardEv), ((unsigned char *)_ZN8EchoTest5drive4bodyEv), ((unsigned char *)_ZN8EchoTest5drive6updateEv) };
unsigned char *_ZTV4Rule[5] = {0 };
unsigned char *_ZTVN4Echo7respond8respond2E[5] = { ((unsigned char *)_ZN4Echo7respond8respond25guardEv), ((unsigned char *)_ZN4Echo7respond8respond24bodyEv), ((unsigned char *)_ZN4Echo7respond8respond26updateEv) };
unsigned char *_ZTVN4Echo7respond8respond1E[5] = { ((unsigned char *)_ZN4Echo7respond8respond15guardEv), ((unsigned char *)_ZN4Echo7respond8respond14bodyEv), ((unsigned char *)_ZN4Echo7respond8respond16updateEv) };
unsigned char *_ZTV5Fifo1IiE[6] = {0 };
unsigned char *_ZTVN5Fifo1IiE10FirstValueE[4] = {0 };
unsigned char *_ZTV12GuardedValueIiE[4] = {0 };
unsigned char *_ZTVN5Fifo1IiE9DeqActionE[6] = {0 };
unsigned char *_ZTV6ActionIiE[6] = {0 };
unsigned char *_ZTVN5Fifo1IiE9EnqActionE[6] = {0 };
unsigned char *_ZTV4FifoIiE[6] = {0 };
void _ZN14EchoIndication4echoEi(unsigned int Vv) {
unsigned int Vcall =         printf((("Heard an echo: %d\n")), Vv);
        stop_main_program = 1;
}

static void __cxx_global_var_init(void) {
        _ZN8EchoTestC1Ev((&echoTest));
}

void _ZN8EchoTestC1Ev(struct l_class_OC_EchoTest *Vthis) {
            struct l_class_OC_EchoTest *Vthis_2e_addr_2e_i;    /* Address-exposed local */
;
        Vthis_2e_addr_2e_i = Vthis;
struct l_class_OC_EchoTest *Vthis1_2e_i =         Vthis_2e_addr_2e_i;
        _ZN6ModuleC2Em((((struct l_class_OC_Module *)Vthis1_2e_i)), 136);
unsigned char *Vcall_2e_i =         llvm_translate_malloc(136);
struct l_class_OC_Echo *Vtmp__1 =         ((struct l_class_OC_Echo *)Vcall_2e_i);
unsigned char *Vcall2_2e_i =         llvm_translate_malloc(1);
struct l_class_OC_EchoIndication *Vtmp__2 =         ((struct l_class_OC_EchoIndication *)Vcall2_2e_i);
        _ZN14EchoIndicationC1Ev(Vtmp__2);
        _ZN4EchoC1EP14EchoIndication(Vtmp__1, Vtmp__2);
        (Vthis1_2e_i->echo) = Vtmp__1;
        (Vthis1_2e_i->x) = 7;
        _ZN8EchoTest5driveC1EPS_(((&Vthis1_2e_i->driveRule)), Vthis1_2e_i);
unsigned int Vcall3_2e_i =         printf((("EchoTest: addr %p size 0x%lx csize 0x%lx\n")), Vthis1_2e_i, 72, 72);
}

void _ZN8EchoTestC2Ev(struct l_class_OC_EchoTest *Vthis) {
        _ZN6ModuleC2Em((((struct l_class_OC_Module *)Vthis)), 136);
unsigned char *Vcall =         llvm_translate_malloc(136);
struct l_class_OC_Echo *Vtmp__1 =         ((struct l_class_OC_Echo *)Vcall);
unsigned char *Vcall2 =         llvm_translate_malloc(1);
struct l_class_OC_EchoIndication *Vtmp__2 =         ((struct l_class_OC_EchoIndication *)Vcall2);
        _ZN14EchoIndicationC1Ev(Vtmp__2);
        _ZN4EchoC1EP14EchoIndication(Vtmp__1, Vtmp__2);
        (Vthis->echo) = Vtmp__1;
        (Vthis->x) = 7;
        _ZN8EchoTest5driveC1EPS_(((&Vthis->driveRule)), Vthis);
unsigned int Vcall3 =         printf((("EchoTest: addr %p size 0x%lx csize 0x%lx\n")), Vthis, 72, 72);
}

void _ZN6ModuleC2Em(struct l_class_OC_Module *Vthis, unsigned long long Vsize) {
            unsigned char *Vtemp;    /* Address-exposed local */
;
        (Vthis->rfirst) = ((struct l_class_OC_Rule *)/*NULL*/0);
struct l_class_OC_Module *Vtmp__1 =         _ZN6Module5firstE;
        (Vthis->next) = Vtmp__1;
        (Vthis->size) = Vsize;
struct l_class_OC_Module *Vtmp__2 =         _ZN6Module5firstE;
unsigned int Vcall =         printf((("[%s] add module to list first %p this %p\n")), (("Module")), Vtmp__2, Vthis);
        _ZN6Module5firstE = Vthis;
unsigned char *Vcall3 =         llvm_translate_malloc(Vsize);
        Vtemp = Vcall3;
unsigned char *Vtmp__3 =         Vtemp;
unsigned char *Vtmp__4 =         memset(Vtmp__3, 0, Vsize);
unsigned char *Vtmp__5 =         Vtemp;
        (Vthis->shadow) = (((struct l_class_OC_Module *)Vtmp__5));
}

void _ZN4EchoC1EP14EchoIndication(struct l_class_OC_Echo *Vthis, struct l_class_OC_EchoIndication *Vind) {
            struct l_class_OC_Echo *Vthis_2e_addr_2e_i;    /* Address-exposed local */
;
            struct l_class_OC_EchoIndication *Vind_2e_addr_2e_i;    /* Address-exposed local */
;
        Vthis_2e_addr_2e_i = Vthis;
        Vind_2e_addr_2e_i = Vind;
struct l_class_OC_Echo *Vthis1_2e_i =         Vthis_2e_addr_2e_i;
        _ZN6ModuleC2Em((((struct l_class_OC_Module *)Vthis1_2e_i)), 136);
unsigned char *Vcall_2e_i =         llvm_translate_malloc(128);
struct l_class_OC_Fifo1 *Vtmp__1 =         ((struct l_class_OC_Fifo1 *)Vcall_2e_i);
        _ZN5Fifo1IiEC1Ev(Vtmp__1);
        (Vthis1_2e_i->fifo) = (((struct l_class_OC_Fifo *)Vtmp__1));
struct l_class_OC_EchoIndication *Vtmp__2 =         Vind_2e_addr_2e_i;
        (Vthis1_2e_i->ind) = Vtmp__2;
        _ZN4Echo7respondC1EPS_(((&Vthis1_2e_i->respondRule)), Vthis1_2e_i);
struct l_class_OC_Fifo *Vtmp__3 =         (Vthis1_2e_i->fifo);
unsigned int Vcall4_2e_i =         printf((("Echo: addr %p size 0x%lx fifo %p csize 0x%lx\n")), Vthis1_2e_i, 136, Vtmp__3, 136);
struct l_class_OC_Fifo *Vtmp__4 =         (Vthis1_2e_i->fifo);
struct l_class_OC_Action *Vtmp__5 =         (Vtmp__4->enq);
        (Vthis1_2e_i->echoreq) = Vtmp__5;
struct l_class_OC_Fifo *Vtmp__6 =         (Vthis1_2e_i->fifo);
struct l_class_OC_GuardedValue *Vtmp__7 =         (Vtmp__6->first);
        (Vthis1_2e_i->firstreq) = Vtmp__7;
struct l_class_OC_Fifo *Vtmp__8 =         (Vthis1_2e_i->fifo);
struct l_class_OC_Action *Vtmp__9 =         (Vtmp__8->deq);
        (Vthis1_2e_i->deqreq) = Vtmp__9;
}

void _ZN14EchoIndicationC1Ev(struct l_class_OC_EchoIndication *Vthis) {
            struct l_class_OC_EchoIndication *Vthis_2e_addr_2e_i;    /* Address-exposed local */
;
        Vthis_2e_addr_2e_i = Vthis;
struct l_class_OC_EchoIndication *Vthis1_2e_i =         Vthis_2e_addr_2e_i;
}

void _ZN8EchoTest5driveC1EPS_(struct l_class_OC_EchoTest_KD__KD_drive *Vthis, struct l_class_OC_EchoTest *Vmodule) {
            struct l_class_OC_EchoTest_KD__KD_drive *Vthis_2e_addr_2e_i;    /* Address-exposed local */
;
            struct l_class_OC_EchoTest *Vmodule_2e_addr_2e_i;    /* Address-exposed local */
;
        Vthis_2e_addr_2e_i = Vthis;
        Vmodule_2e_addr_2e_i = Vmodule;
struct l_class_OC_EchoTest_KD__KD_drive *Vthis1_2e_i =         Vthis_2e_addr_2e_i;
        _ZN4RuleC2Ev((((struct l_class_OC_Rule *)Vthis1_2e_i)));
        *(((unsigned char ***)Vthis1_2e_i)) = ((_ZTVN8EchoTest5driveE));
struct l_class_OC_EchoTest *Vtmp__1 =         Vmodule_2e_addr_2e_i;
        (Vthis1_2e_i->module) = Vtmp__1;
struct l_class_OC_EchoTest *Vtmp__2 =         Vmodule_2e_addr_2e_i;
        _ZN6Module7addRuleEP4Rule((((struct l_class_OC_Module *)Vtmp__2)), (((struct l_class_OC_Rule *)Vthis1_2e_i)));
}

void _ZN8EchoTest5driveC2EPS_(struct l_class_OC_EchoTest_KD__KD_drive *Vthis, struct l_class_OC_EchoTest *Vmodule) {
        _ZN4RuleC2Ev((((struct l_class_OC_Rule *)Vthis)));
        *(((unsigned char ***)Vthis)) = ((_ZTVN8EchoTest5driveE));
        (Vthis->module) = Vmodule;
        _ZN6Module7addRuleEP4Rule((((struct l_class_OC_Module *)Vmodule)), (((struct l_class_OC_Rule *)Vthis)));
}

void _ZN4RuleC2Ev(struct l_class_OC_Rule *Vthis) {
        *(((unsigned char ***)Vthis)) = ((_ZTV4Rule));
}

void _ZN6Module7addRuleEP4Rule(struct l_class_OC_Module *Vthis, struct l_class_OC_Rule *Vrule) {
struct l_class_OC_Rule *Vtmp__1 =         (Vthis->rfirst);
unsigned int Vcall =         printf((("[%s] add rule to module list rfirst %p this %p\n")), (("addRule")), Vtmp__1, Vthis);
struct l_class_OC_Rule *Vtmp__2 =         (Vthis->rfirst);
        (Vrule->next) = Vtmp__2;
        (Vthis->rfirst) = Vrule;
}

bool _ZN8EchoTest5drive5guardEv(struct l_class_OC_EchoTest_KD__KD_drive *Vthis) {
struct l_class_OC_EchoTest *Vtmp__1 =         (Vthis->module);
struct l_class_OC_Echo *Vtmp__2 =         (Vtmp__1->echo);
struct l_class_OC_Action *Vtmp__3 =         (Vtmp__2->echoreq);
bool Vtmp__4 =         _ZN5Fifo1IiE9EnqAction5guardEv((struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *)Vtmp__3);
          return Vtmp__4;
;
}

void _ZN8EchoTest5drive4bodyEv(struct l_class_OC_EchoTest_KD__KD_drive *Vthis) {
struct l_class_OC_EchoTest *Vtmp__1 =         (Vthis->module);
struct l_class_OC_Echo *Vtmp__2 =         (Vtmp__1->echo);
struct l_class_OC_Action *Vtmp__3 =         (Vtmp__2->echoreq);
        _ZN5Fifo1IiE9EnqAction4bodyEi((struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *)Vtmp__3, 22);
}

void _ZN8EchoTest5drive6updateEv(struct l_class_OC_EchoTest_KD__KD_drive *Vthis) {
struct l_class_OC_EchoTest *Vtmp__1 =         (Vthis->module);
struct l_class_OC_Echo *Vtmp__2 =         (Vtmp__1->echo);
struct l_class_OC_Action *Vtmp__3 =         (Vtmp__2->echoreq);
        _ZN5Fifo1IiE9EnqAction6updateEv((struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *)Vtmp__3);
}

void _ZN14EchoIndicationC2Ev(struct l_class_OC_EchoIndication *Vthis) {
}

void _ZN4EchoC2EP14EchoIndication(struct l_class_OC_Echo *Vthis, struct l_class_OC_EchoIndication *Vind) {
        _ZN6ModuleC2Em((((struct l_class_OC_Module *)Vthis)), 136);
unsigned char *Vcall =         llvm_translate_malloc(128);
struct l_class_OC_Fifo1 *Vtmp__1 =         ((struct l_class_OC_Fifo1 *)Vcall);
        _ZN5Fifo1IiEC1Ev(Vtmp__1);
        (Vthis->fifo) = (((struct l_class_OC_Fifo *)Vtmp__1));
        (Vthis->ind) = Vind;
        _ZN4Echo7respondC1EPS_(((&Vthis->respondRule)), Vthis);
struct l_class_OC_Fifo *Vtmp__2 =         (Vthis->fifo);
unsigned int Vcall4 =         printf((("Echo: addr %p size 0x%lx fifo %p csize 0x%lx\n")), Vthis, 136, Vtmp__2, 136);
struct l_class_OC_Fifo *Vtmp__3 =         (Vthis->fifo);
struct l_class_OC_Action *Vtmp__4 =         (Vtmp__3->enq);
        (Vthis->echoreq) = Vtmp__4;
struct l_class_OC_Fifo *Vtmp__5 =         (Vthis->fifo);
struct l_class_OC_GuardedValue *Vtmp__6 =         (Vtmp__5->first);
        (Vthis->firstreq) = Vtmp__6;
struct l_class_OC_Fifo *Vtmp__7 =         (Vthis->fifo);
struct l_class_OC_Action *Vtmp__8 =         (Vtmp__7->deq);
        (Vthis->deqreq) = Vtmp__8;
}

void _ZN5Fifo1IiEC1Ev(struct l_class_OC_Fifo1 *Vthis) {
            struct l_class_OC_Fifo1 *Vthis_2e_addr_2e_i;    /* Address-exposed local */
;
        Vthis_2e_addr_2e_i = Vthis;
struct l_class_OC_Fifo1 *Vthis1_2e_i =         Vthis_2e_addr_2e_i;
        *(((unsigned char ***)Vthis1_2e_i)) = ((_ZTV5Fifo1IiE));
        _ZN4FifoIiEC2EmP6ActionIiEP12GuardedValueIiES3_((((struct l_class_OC_Fifo *)Vthis1_2e_i)), 128, (((struct l_class_OC_Action *)((&Vthis1_2e_i->enqAction)))), (((struct l_class_OC_GuardedValue *)((&Vthis1_2e_i->firstValue)))), (((struct l_class_OC_Action *)((&Vthis1_2e_i->deqAction)))));
        *(((unsigned char ***)Vthis1_2e_i)) = ((_ZTV5Fifo1IiE));
        (Vthis1_2e_i->full) = ((unsigned char )0);
        _ZN5Fifo1IiE9EnqActionC1EPS0_(((&Vthis1_2e_i->enqAction)), Vthis1_2e_i);
        _ZN5Fifo1IiE9DeqActionC1EPS0_(((&Vthis1_2e_i->deqAction)), Vthis1_2e_i);
        _ZN5Fifo1IiE10FirstValueC1EPS0_(((&Vthis1_2e_i->firstValue)), Vthis1_2e_i);
unsigned int Vcall_2e_i =         printf((("Fifo1: addr %p size 0x%lx\n")), Vthis1_2e_i, 128);
}

void _ZN4Echo7respondC1EPS_(struct l_class_OC_Echo_KD__KD_respond *Vthis, struct l_class_OC_Echo *Vmodule) {
            struct l_class_OC_Echo_KD__KD_respond *Vthis_2e_addr_2e_i;    /* Address-exposed local */
;
            struct l_class_OC_Echo *Vmodule_2e_addr_2e_i;    /* Address-exposed local */
;
        Vthis_2e_addr_2e_i = Vthis;
        Vmodule_2e_addr_2e_i = Vmodule;
struct l_class_OC_Echo_KD__KD_respond *Vthis1_2e_i =         Vthis_2e_addr_2e_i;
struct l_class_OC_Echo *Vtmp__1 =         Vmodule_2e_addr_2e_i;
        _ZN4Echo7respond8respond1C1EPS_(((&Vthis1_2e_i->respond1Rule)), Vtmp__1);
struct l_class_OC_Echo *Vtmp__2 =         Vmodule_2e_addr_2e_i;
        _ZN4Echo7respond8respond2C1EPS_(((&Vthis1_2e_i->respond2Rule)), Vtmp__2);
}

void _ZN4Echo7respondC2EPS_(struct l_class_OC_Echo_KD__KD_respond *Vthis, struct l_class_OC_Echo *Vmodule) {
        _ZN4Echo7respond8respond1C1EPS_(((&Vthis->respond1Rule)), Vmodule);
        _ZN4Echo7respond8respond2C1EPS_(((&Vthis->respond2Rule)), Vmodule);
}

void _ZN4Echo7respond8respond1C1EPS_(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 *Vthis, struct l_class_OC_Echo *Vmodule) {
            struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 *Vthis_2e_addr_2e_i;    /* Address-exposed local */
;
            struct l_class_OC_Echo *Vmodule_2e_addr_2e_i;    /* Address-exposed local */
;
        Vthis_2e_addr_2e_i = Vthis;
        Vmodule_2e_addr_2e_i = Vmodule;
struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 *Vthis1_2e_i =         Vthis_2e_addr_2e_i;
        _ZN4RuleC2Ev((((struct l_class_OC_Rule *)Vthis1_2e_i)));
        *(((unsigned char ***)Vthis1_2e_i)) = ((_ZTVN4Echo7respond8respond1E));
struct l_class_OC_Echo *Vtmp__1 =         Vmodule_2e_addr_2e_i;
        (Vthis1_2e_i->module) = Vtmp__1;
struct l_class_OC_Echo *Vtmp__2 =         Vmodule_2e_addr_2e_i;
        _ZN6Module7addRuleEP4Rule((((struct l_class_OC_Module *)Vtmp__2)), (((struct l_class_OC_Rule *)Vthis1_2e_i)));
}

void _ZN4Echo7respond8respond2C1EPS_(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 *Vthis, struct l_class_OC_Echo *Vmodule) {
            struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 *Vthis_2e_addr_2e_i;    /* Address-exposed local */
;
            struct l_class_OC_Echo *Vmodule_2e_addr_2e_i;    /* Address-exposed local */
;
        Vthis_2e_addr_2e_i = Vthis;
        Vmodule_2e_addr_2e_i = Vmodule;
struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 *Vthis1_2e_i =         Vthis_2e_addr_2e_i;
        _ZN4RuleC2Ev((((struct l_class_OC_Rule *)Vthis1_2e_i)));
        *(((unsigned char ***)Vthis1_2e_i)) = ((_ZTVN4Echo7respond8respond2E));
struct l_class_OC_Echo *Vtmp__1 =         Vmodule_2e_addr_2e_i;
        (Vthis1_2e_i->module) = Vtmp__1;
struct l_class_OC_Echo *Vtmp__2 =         Vmodule_2e_addr_2e_i;
        _ZN6Module7addRuleEP4Rule((((struct l_class_OC_Module *)Vtmp__2)), (((struct l_class_OC_Rule *)Vthis1_2e_i)));
}

void _ZN4Echo7respond8respond2C2EPS_(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 *Vthis, struct l_class_OC_Echo *Vmodule) {
        _ZN4RuleC2Ev((((struct l_class_OC_Rule *)Vthis)));
        *(((unsigned char ***)Vthis)) = ((_ZTVN4Echo7respond8respond2E));
        (Vthis->module) = Vmodule;
        _ZN6Module7addRuleEP4Rule((((struct l_class_OC_Module *)Vmodule)), (((struct l_class_OC_Rule *)Vthis)));
}

bool _ZN4Echo7respond8respond25guardEv(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 *Vthis) {
          return 1;
;
}

void _ZN4Echo7respond8respond24bodyEv(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 *Vthis) {
struct l_class_OC_Echo *Vtmp__1 =         (Vthis->module);
struct l_class_OC_GuardedValue *Vtmp__2 =         (Vtmp__1->firstreq);
unsigned int Vcallfoosuff =         _ZN5Fifo1IiE10FirstValue5valueEv((struct l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue *)Vtmp__2);
struct l_class_OC_Echo *Vtmp__3 =         (Vthis->module);
        (Vtmp__3->pipetemp) = Vcallfoosuff;
}

void _ZN4Echo7respond8respond26updateEv(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 *Vthis) {
}

void _ZN4Echo7respond8respond1C2EPS_(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 *Vthis, struct l_class_OC_Echo *Vmodule) {
        _ZN4RuleC2Ev((((struct l_class_OC_Rule *)Vthis)));
        *(((unsigned char ***)Vthis)) = ((_ZTVN4Echo7respond8respond1E));
        (Vthis->module) = Vmodule;
        _ZN6Module7addRuleEP4Rule((((struct l_class_OC_Module *)Vmodule)), (((struct l_class_OC_Rule *)Vthis)));
}

bool _ZN4Echo7respond8respond15guardEv(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 *Vthis) {
struct l_class_OC_Echo *Vtmp__1 =         (Vthis->module);
struct l_class_OC_GuardedValue *Vtmp__2 =         (Vtmp__1->firstreq);
bool Vtmp__3 =         _ZN5Fifo1IiE10FirstValue5guardEv((struct l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue *)Vtmp__2);
struct l_class_OC_Echo *Vtmp__4 =         (Vthis->module);
struct l_class_OC_Action *Vtmp__5 =         (Vtmp__4->deqreq);
bool Vtmp__6 =         _ZN5Fifo1IiE9DeqAction5guardEv((struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *)Vtmp__5);
          return (Vtmp__3 & Vtmp__6);
;
}

void _ZN4Echo7respond8respond14bodyEv(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 *Vthis) {
struct l_class_OC_Echo *Vtmp__1 =         (Vthis->module);
struct l_class_OC_GuardedValue *Vtmp__2 =         (Vtmp__1->firstreq);
unsigned int Vcall =         _ZN5Fifo1IiE10FirstValue5valueEv((struct l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue *)Vtmp__2);
struct l_class_OC_Echo *Vtmp__3 =         (Vthis->module);
unsigned int Vtmp__4 =         (Vtmp__3->pipetemp);
struct l_class_OC_Echo *Vtmp__5 =         (Vthis->module);
        (Vtmp__5->response) = Vtmp__4;
struct l_class_OC_Echo *Vtmp__6 =         (Vthis->module);
struct l_class_OC_Action *Vtmp__7 =         (Vtmp__6->deqreq);
        _ZN5Fifo1IiE9DeqAction4bodyEv((struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *)Vtmp__7);
struct l_class_OC_Echo *Vtmp__8 =         (Vthis->module);
struct l_class_OC_EchoIndication *Vtmp__9 =         (Vtmp__8->ind);
struct l_class_OC_Echo *Vtmp__10 =         (Vthis->module);
unsigned int Vtmp__11 =         (Vtmp__10->response);
}

void _ZN4Echo7respond8respond16updateEv(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 *Vthis) {
struct l_class_OC_Echo *Vtmp__1 =         (Vthis->module);
struct l_class_OC_Action *Vtmp__2 =         (Vtmp__1->deqreq);
        _ZN5Fifo1IiE9DeqAction6updateEv((struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *)Vtmp__2);
struct l_class_OC_Echo *Vtmp__3 =         (Vthis->module);
unsigned int Vtmp__4 =         (Vtmp__3->response);
        _ZN14EchoIndication4echoEi(Vtmp__4);
}

void _ZN5Fifo1IiEC2Ev(struct l_class_OC_Fifo1 *Vthis) {
        *(((unsigned char ***)Vthis)) = ((_ZTV5Fifo1IiE));
        _ZN4FifoIiEC2EmP6ActionIiEP12GuardedValueIiES3_((((struct l_class_OC_Fifo *)Vthis)), 128, (((struct l_class_OC_Action *)((&Vthis->enqAction)))), (((struct l_class_OC_GuardedValue *)((&Vthis->firstValue)))), (((struct l_class_OC_Action *)((&Vthis->deqAction)))));
        *(((unsigned char ***)Vthis)) = ((_ZTV5Fifo1IiE));
        (Vthis->full) = ((unsigned char )0);
        _ZN5Fifo1IiE9EnqActionC1EPS0_(((&Vthis->enqAction)), Vthis);
        _ZN5Fifo1IiE9DeqActionC1EPS0_(((&Vthis->deqAction)), Vthis);
        _ZN5Fifo1IiE10FirstValueC1EPS0_(((&Vthis->firstValue)), Vthis);
unsigned int Vcall =         printf((("Fifo1: addr %p size 0x%lx\n")), Vthis, 128);
}

void _ZN4FifoIiEC2EmP6ActionIiEP12GuardedValueIiES3_(struct l_class_OC_Fifo *Vthis, unsigned long long Vsize, struct l_class_OC_Action *Venq, struct l_class_OC_GuardedValue *Vfirst, struct l_class_OC_Action *Vdeq) {
        _ZN6ModuleC2Em((((struct l_class_OC_Module *)((&(((unsigned char *)Vthis))[8])))), Vsize);
        *(((unsigned char ***)Vthis)) = ((_ZTV4FifoIiE));
        (Vthis->enq) = Venq;
        (Vthis->first) = Vfirst;
        (Vthis->deq) = Vdeq;
}

void _ZN5Fifo1IiE9EnqActionC1EPS0_(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *Vthis, struct l_class_OC_Fifo1 *Vfifo) {
            struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *Vthis_2e_addr_2e_i;    /* Address-exposed local */
;
            struct l_class_OC_Fifo1 *Vfifo_2e_addr_2e_i;    /* Address-exposed local */
;
        Vthis_2e_addr_2e_i = Vthis;
        Vfifo_2e_addr_2e_i = Vfifo;
struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *Vthis1_2e_i =         Vthis_2e_addr_2e_i;
        _ZN6ActionIiEC2Ev((((struct l_class_OC_Action *)Vthis1_2e_i)));
        *(((unsigned char ***)Vthis1_2e_i)) = ((_ZTVN5Fifo1IiE9EnqActionE));
struct l_class_OC_Fifo1 *Vtmp__1 =         Vfifo_2e_addr_2e_i;
        (Vthis1_2e_i->fifo) = Vtmp__1;
        (Vthis1_2e_i->elt) = 0;
struct l_class_OC_Fifo1 *Vtmp__2 =         Vfifo_2e_addr_2e_i;
unsigned int Vcall_2e_i =         printf((("[%s:%d] fifo %p\n")), (("EnqAction")), 30, Vtmp__2);
}

void _ZN5Fifo1IiE9DeqActionC1EPS0_(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *Vthis, struct l_class_OC_Fifo1 *Vfifo) {
            struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *Vthis_2e_addr_2e_i;    /* Address-exposed local */
;
            struct l_class_OC_Fifo1 *Vfifo_2e_addr_2e_i;    /* Address-exposed local */
;
        Vthis_2e_addr_2e_i = Vthis;
        Vfifo_2e_addr_2e_i = Vfifo;
struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *Vthis1_2e_i =         Vthis_2e_addr_2e_i;
        _ZN6ActionIiEC2Ev((((struct l_class_OC_Action *)Vthis1_2e_i)));
        *(((unsigned char ***)Vthis1_2e_i)) = ((_ZTVN5Fifo1IiE9DeqActionE));
struct l_class_OC_Fifo1 *Vtmp__1 =         Vfifo_2e_addr_2e_i;
        (Vthis1_2e_i->fifo) = Vtmp__1;
struct l_class_OC_Fifo1 *Vtmp__2 =         Vfifo_2e_addr_2e_i;
unsigned int Vcall_2e_i =         printf((("[%s:%d] fifo %p\n")), (("DeqAction")), 44, Vtmp__2);
}

void _ZN5Fifo1IiE10FirstValueC1EPS0_(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue *Vthis, struct l_class_OC_Fifo1 *Vfifo) {
            struct l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue *Vthis_2e_addr_2e_i;    /* Address-exposed local */
;
            struct l_class_OC_Fifo1 *Vfifo_2e_addr_2e_i;    /* Address-exposed local */
;
        Vthis_2e_addr_2e_i = Vthis;
        Vfifo_2e_addr_2e_i = Vfifo;
struct l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue *Vthis1_2e_i =         Vthis_2e_addr_2e_i;
        _ZN12GuardedValueIiEC2Ev((((struct l_class_OC_GuardedValue *)Vthis1_2e_i)));
        *(((unsigned char ***)Vthis1_2e_i)) = ((_ZTVN5Fifo1IiE10FirstValueE));
struct l_class_OC_Fifo1 *Vtmp__1 =         Vfifo_2e_addr_2e_i;
        (Vthis1_2e_i->fifo) = Vtmp__1;
}

bool _ZNK5Fifo1IiE8notEmptyEv(struct l_class_OC_Fifo1 *Vthis) {
unsigned char Vtmp__1 =         (Vthis->full);
          return (((bool )Vtmp__1&1u));
;
}

bool _ZNK5Fifo1IiE7notFullEv(struct l_class_OC_Fifo1 *Vthis) {
unsigned char Vtmp__1 =         (Vthis->full);
          return ((((bool )Vtmp__1&1u)) ^ 1);
;
}

void _ZN5Fifo1IiE10FirstValueC2EPS0_(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue *Vthis, struct l_class_OC_Fifo1 *Vfifo) {
        _ZN12GuardedValueIiEC2Ev((((struct l_class_OC_GuardedValue *)Vthis)));
        *(((unsigned char ***)Vthis)) = ((_ZTVN5Fifo1IiE10FirstValueE));
        (Vthis->fifo) = Vfifo;
}

void _ZN12GuardedValueIiEC2Ev(struct l_class_OC_GuardedValue *Vthis) {
        *(((unsigned char ***)Vthis)) = ((_ZTV12GuardedValueIiE));
}

bool _ZN5Fifo1IiE10FirstValue5guardEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue *Vthis) {
struct l_class_OC_Fifo1 *Vtmp__1 =         (Vthis->fifo);
bool Vcall =         _ZNK5Fifo1IiE8notEmptyEv(Vtmp__1);
          return Vcall;
;
}

unsigned int _ZN5Fifo1IiE10FirstValue5valueEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue *Vthis) {
struct l_class_OC_Fifo1 *Vtmp__1 =         (Vthis->fifo);
unsigned int Vtmp__2 =         (Vtmp__1->element);
          return Vtmp__2;
;
}

void _ZN5Fifo1IiE9DeqActionC2EPS0_(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *Vthis, struct l_class_OC_Fifo1 *Vfifo) {
        _ZN6ActionIiEC2Ev((((struct l_class_OC_Action *)Vthis)));
        *(((unsigned char ***)Vthis)) = ((_ZTVN5Fifo1IiE9DeqActionE));
        (Vthis->fifo) = Vfifo;
unsigned int Vcall =         printf((("[%s:%d] fifo %p\n")), (("DeqAction")), 44, Vfifo);
}

void _ZN6ActionIiEC2Ev(struct l_class_OC_Action *Vthis) {
        *(((unsigned char ***)Vthis)) = ((_ZTV6ActionIiE));
}

bool _ZN5Fifo1IiE9DeqAction5guardEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *Vthis) {
struct l_class_OC_Fifo1 *Vtmp__1 =         (Vthis->fifo);
bool Vcall =         _ZNK5Fifo1IiE8notEmptyEv(Vtmp__1);
          return Vcall;
;
}

void _ZN5Fifo1IiE9DeqAction4bodyEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *Vthis) {
}

void _ZN5Fifo1IiE9DeqAction4bodyEi(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *Vthis, unsigned int Vv) {
}

void _ZN5Fifo1IiE9DeqAction6updateEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *Vthis) {
struct l_class_OC_Fifo1 *Vtmp__1 =         (Vthis->fifo);
        (Vtmp__1->full) = ((unsigned char )0);
}

bool _ZN6ActionIiE5guardEv(struct l_class_OC_Action *Vthis) {
          return 1;
;
}

void _ZN5Fifo1IiE9EnqActionC2EPS0_(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *Vthis, struct l_class_OC_Fifo1 *Vfifo) {
        _ZN6ActionIiEC2Ev((((struct l_class_OC_Action *)Vthis)));
        *(((unsigned char ***)Vthis)) = ((_ZTVN5Fifo1IiE9EnqActionE));
        (Vthis->fifo) = Vfifo;
        (Vthis->elt) = 0;
unsigned int Vcall =         printf((("[%s:%d] fifo %p\n")), (("EnqAction")), 30, Vfifo);
}

bool _ZN5Fifo1IiE9EnqAction5guardEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *Vthis) {
struct l_class_OC_Fifo1 *Vtmp__1 =         (Vthis->fifo);
bool Vcall =         _ZNK5Fifo1IiE7notFullEv(Vtmp__1);
          return Vcall;
;
}

void _ZN5Fifo1IiE9EnqAction4bodyEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *Vthis) {
}

void _ZN5Fifo1IiE9EnqAction4bodyEi(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *Vthis, unsigned int Vv) {
        (Vthis->elt) = Vv;
}

void _ZN5Fifo1IiE9EnqAction6updateEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *Vthis) {
unsigned int Vtmp__1 =         (Vthis->elt);
struct l_class_OC_Fifo1 *Vtmp__2 =         (Vthis->fifo);
        (Vtmp__2->element) = Vtmp__1;
struct l_class_OC_Fifo1 *Vtmp__3 =         (Vthis->fifo);
        (Vtmp__3->full) = ((unsigned char )1);
}

static void _GLOBAL__I_a(void) {
        _ZN8EchoTestC1Ev((&echoTest));
}

