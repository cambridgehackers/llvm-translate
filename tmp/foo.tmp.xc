

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
        printf((("Heard an echo: %d\n")), Vv);
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
        printf((("EchoTest: addr %p size 0x%lx csize 0x%lx\n")), Vthis1_2e_i, 72, 72);
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
        printf((("EchoTest: addr %p size 0x%lx csize 0x%lx\n")), Vthis, 72, 72);
}

void _ZN6ModuleC2Em(struct l_class_OC_Module *Vthis, unsigned long long Vsize) {
            unsigned char *Vtemp;    /* Address-exposed local */
;
        (Vthis->rfirst) = ((struct l_class_OC_Rule *)/*NULL*/0);
        (Vthis->next) = (_ZN6Module5firstE);
        (Vthis->size) = Vsize;
        printf((("[%s] add module to list first %p this %p\n")), (("Module")), (_ZN6Module5firstE), Vthis);
        _ZN6Module5firstE = Vthis;
unsigned char *Vcall3 =         llvm_translate_malloc(Vsize);
        Vtemp = Vcall3;
        memset((Vtemp), 0, Vsize);
        (Vthis->shadow) = (((struct l_class_OC_Module *)(Vtemp)));
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
        (Vthis1_2e_i->ind) = (Vind_2e_addr_2e_i);
        _ZN4Echo7respondC1EPS_(((&Vthis1_2e_i->respondRule)), Vthis1_2e_i);
        printf((("Echo: addr %p size 0x%lx fifo %p csize 0x%lx\n")), Vthis1_2e_i, 136, ((Vthis1_2e_i->fifo)), 136);
        (Vthis1_2e_i->echoreq) = ((((Vthis1_2e_i->fifo))->enq));
        (Vthis1_2e_i->firstreq) = ((((Vthis1_2e_i->fifo))->first));
        (Vthis1_2e_i->deqreq) = ((((Vthis1_2e_i->fifo))->deq));
}

void _ZN14EchoIndicationC1Ev(struct l_class_OC_EchoIndication *Vthis) {
            struct l_class_OC_EchoIndication *Vthis_2e_addr_2e_i;    /* Address-exposed local */
;
        Vthis_2e_addr_2e_i = Vthis;
        Vthis_2e_addr_2e_i;
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
        (Vthis1_2e_i->module) = (Vmodule_2e_addr_2e_i);
        _ZN6Module7addRuleEP4Rule((((struct l_class_OC_Module *)(Vmodule_2e_addr_2e_i))), (((struct l_class_OC_Rule *)Vthis1_2e_i)));
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
        printf((("[%s] add rule to module list rfirst %p this %p\n")), (("addRule")), ((Vthis->rfirst)), Vthis);
        (Vrule->next) = ((Vthis->rfirst));
        (Vthis->rfirst) = Vrule;
}

bool _ZN8EchoTest5drive5guardEv(struct l_class_OC_EchoTest_KD__KD_drive *Vthis) {
bool Vtmp__1 =         _ZN5Fifo1IiE9EnqAction5guardEv((struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *)((((((Vthis->module))->echo))->echoreq)));
          return Vtmp__1;
;
}

void _ZN8EchoTest5drive4bodyEv(struct l_class_OC_EchoTest_KD__KD_drive *Vthis) {
        _ZN5Fifo1IiE9EnqAction4bodyEi((struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *)((((((Vthis->module))->echo))->echoreq)), 22);
}

void _ZN8EchoTest5drive6updateEv(struct l_class_OC_EchoTest_KD__KD_drive *Vthis) {
        _ZN5Fifo1IiE9EnqAction6updateEv((struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *)((((((Vthis->module))->echo))->echoreq)));
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
        printf((("Echo: addr %p size 0x%lx fifo %p csize 0x%lx\n")), Vthis, 136, ((Vthis->fifo)), 136);
        (Vthis->echoreq) = ((((Vthis->fifo))->enq));
        (Vthis->firstreq) = ((((Vthis->fifo))->first));
        (Vthis->deqreq) = ((((Vthis->fifo))->deq));
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
        printf((("Fifo1: addr %p size 0x%lx\n")), Vthis1_2e_i, 128);
}

void _ZN4Echo7respondC1EPS_(struct l_class_OC_Echo_KD__KD_respond *Vthis, struct l_class_OC_Echo *Vmodule) {
            struct l_class_OC_Echo_KD__KD_respond *Vthis_2e_addr_2e_i;    /* Address-exposed local */
;
            struct l_class_OC_Echo *Vmodule_2e_addr_2e_i;    /* Address-exposed local */
;
        Vthis_2e_addr_2e_i = Vthis;
        Vmodule_2e_addr_2e_i = Vmodule;
struct l_class_OC_Echo_KD__KD_respond *Vthis1_2e_i =         Vthis_2e_addr_2e_i;
        _ZN4Echo7respond8respond1C1EPS_(((&Vthis1_2e_i->respond1Rule)), (Vmodule_2e_addr_2e_i));
        _ZN4Echo7respond8respond2C1EPS_(((&Vthis1_2e_i->respond2Rule)), (Vmodule_2e_addr_2e_i));
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
        (Vthis1_2e_i->module) = (Vmodule_2e_addr_2e_i);
        _ZN6Module7addRuleEP4Rule((((struct l_class_OC_Module *)(Vmodule_2e_addr_2e_i))), (((struct l_class_OC_Rule *)Vthis1_2e_i)));
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
        (Vthis1_2e_i->module) = (Vmodule_2e_addr_2e_i);
        _ZN6Module7addRuleEP4Rule((((struct l_class_OC_Module *)(Vmodule_2e_addr_2e_i))), (((struct l_class_OC_Rule *)Vthis1_2e_i)));
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
unsigned int Vcallfoosuff =         _ZN5Fifo1IiE10FirstValue5valueEv((struct l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue *)((((Vthis->module))->firstreq)));
        (((Vthis->module))->pipetemp) = Vcallfoosuff;
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
bool Vtmp__1 =         _ZN5Fifo1IiE10FirstValue5guardEv((struct l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue *)((((Vthis->module))->firstreq)));
bool Vtmp__2 =         _ZN5Fifo1IiE9DeqAction5guardEv((struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *)((((Vthis->module))->deqreq)));
          return (Vtmp__1 & Vtmp__2);
;
}

void _ZN4Echo7respond8respond14bodyEv(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 *Vthis) {
        _ZN5Fifo1IiE10FirstValue5valueEv((struct l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue *)((((Vthis->module))->firstreq)));
        (((Vthis->module))->response) = ((((Vthis->module))->pipetemp));
        _ZN5Fifo1IiE9DeqAction4bodyEv((struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *)((((Vthis->module))->deqreq)));
        (((Vthis->module))->ind);
        (((Vthis->module))->response);
}

void _ZN4Echo7respond8respond16updateEv(struct l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 *Vthis) {
        _ZN5Fifo1IiE9DeqAction6updateEv((struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *)((((Vthis->module))->deqreq)));
        _ZN14EchoIndication4echoEi(((((Vthis->module))->response)));
}

void _ZN5Fifo1IiEC2Ev(struct l_class_OC_Fifo1 *Vthis) {
        *(((unsigned char ***)Vthis)) = ((_ZTV5Fifo1IiE));
        _ZN4FifoIiEC2EmP6ActionIiEP12GuardedValueIiES3_((((struct l_class_OC_Fifo *)Vthis)), 128, (((struct l_class_OC_Action *)((&Vthis->enqAction)))), (((struct l_class_OC_GuardedValue *)((&Vthis->firstValue)))), (((struct l_class_OC_Action *)((&Vthis->deqAction)))));
        *(((unsigned char ***)Vthis)) = ((_ZTV5Fifo1IiE));
        (Vthis->full) = ((unsigned char )0);
        _ZN5Fifo1IiE9EnqActionC1EPS0_(((&Vthis->enqAction)), Vthis);
        _ZN5Fifo1IiE9DeqActionC1EPS0_(((&Vthis->deqAction)), Vthis);
        _ZN5Fifo1IiE10FirstValueC1EPS0_(((&Vthis->firstValue)), Vthis);
        printf((("Fifo1: addr %p size 0x%lx\n")), Vthis, 128);
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
        (Vthis1_2e_i->fifo) = (Vfifo_2e_addr_2e_i);
        (Vthis1_2e_i->elt) = 0;
        printf((("[%s:%d] fifo %p\n")), (("EnqAction")), 30, (Vfifo_2e_addr_2e_i));
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
        (Vthis1_2e_i->fifo) = (Vfifo_2e_addr_2e_i);
        printf((("[%s:%d] fifo %p\n")), (("DeqAction")), 44, (Vfifo_2e_addr_2e_i));
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
        (Vthis1_2e_i->fifo) = (Vfifo_2e_addr_2e_i);
}

bool _ZNK5Fifo1IiE8notEmptyEv(struct l_class_OC_Fifo1 *Vthis) {
          return (((bool )((Vthis->full))&1u));
;
}

bool _ZNK5Fifo1IiE7notFullEv(struct l_class_OC_Fifo1 *Vthis) {
          return ((((bool )((Vthis->full))&1u)) ^ 1);
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
bool Vcall =         _ZNK5Fifo1IiE8notEmptyEv(((Vthis->fifo)));
          return Vcall;
;
}

unsigned int _ZN5Fifo1IiE10FirstValue5valueEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_FirstValue *Vthis) {
          return ((((Vthis->fifo))->element));
;
}

void _ZN5Fifo1IiE9DeqActionC2EPS0_(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *Vthis, struct l_class_OC_Fifo1 *Vfifo) {
        _ZN6ActionIiEC2Ev((((struct l_class_OC_Action *)Vthis)));
        *(((unsigned char ***)Vthis)) = ((_ZTVN5Fifo1IiE9DeqActionE));
        (Vthis->fifo) = Vfifo;
        printf((("[%s:%d] fifo %p\n")), (("DeqAction")), 44, Vfifo);
}

void _ZN6ActionIiEC2Ev(struct l_class_OC_Action *Vthis) {
        *(((unsigned char ***)Vthis)) = ((_ZTV6ActionIiE));
}

bool _ZN5Fifo1IiE9DeqAction5guardEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *Vthis) {
bool Vcall =         _ZNK5Fifo1IiE8notEmptyEv(((Vthis->fifo)));
          return Vcall;
;
}

void _ZN5Fifo1IiE9DeqAction4bodyEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *Vthis) {
}

void _ZN5Fifo1IiE9DeqAction4bodyEi(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *Vthis, unsigned int Vv) {
}

void _ZN5Fifo1IiE9DeqAction6updateEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_DeqAction *Vthis) {
        (((Vthis->fifo))->full) = ((unsigned char )0);
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
        printf((("[%s:%d] fifo %p\n")), (("EnqAction")), 30, Vfifo);
}

bool _ZN5Fifo1IiE9EnqAction5guardEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *Vthis) {
bool Vcall =         _ZNK5Fifo1IiE7notFullEv(((Vthis->fifo)));
          return Vcall;
;
}

void _ZN5Fifo1IiE9EnqAction4bodyEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *Vthis) {
}

void _ZN5Fifo1IiE9EnqAction4bodyEi(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *Vthis, unsigned int Vv) {
        (Vthis->elt) = Vv;
}

void _ZN5Fifo1IiE9EnqAction6updateEv(struct l_class_OC_Fifo1_MD_int_OD__KD__KD_EnqAction *Vthis) {
        (((Vthis->fifo))->element) = ((Vthis->elt));
        (((Vthis->fifo))->full) = ((unsigned char )1);
}

static void _GLOBAL__I_a(void) {
        _ZN8EchoTestC1Ev((&echoTest));
}

