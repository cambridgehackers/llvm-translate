

/* Global Variable Definitions and Initialization */
class l_class_OC_EchoTest echoTest;
unsigned int stop_main_program;
class l_class_OC_Module *_ZN6Module5firstE;
//processing _ZN14EchoIndication4echoEi
void _ZN14EchoIndication4echoEi(unsigned int Vv) {
        printf((("Heard an echo: %d\n")), Vv);
        stop_main_program = 1;
}
//processing printf
//processing _ZN14EchoIndication4echoEi
bool l_class_OC_EchoTest::drive__RDY(void) {
    bool tmp__1 =     ((*((echo)->fifo)).enq__RDY)();
        return tmp__1;
}
void l_class_OC_EchoTest::drive__ENA(void) {
        ((*((echo)->fifo)).enq)(22);
}
bool l_class_OC_Echo::respond__RDY(void) {
    bool tmp__1 =     ((*(fifo)).deq__RDY)();
    bool tmp__2 =     ((*(fifo)).first__RDY)();
        return (tmp__1 & tmp__2);
}
void l_class_OC_Echo::respond__ENA(void) {
        ((*(fifo)).deq)();
    unsigned int call =     ((*(fifo)).first)();
        _ZN14EchoIndication4echoEi(call);
}
bool l_class_OC_Fifo1::deq__RDY(void) {
        return (full);
}
bool l_class_OC_Fifo1::enq__RDY(void) {
        return ((full) ^ 1);
}
void l_class_OC_Fifo1::enq(unsigned int v) {
        (element) = v;
        (full) = 1;
}
void l_class_OC_Fifo1::deq(void) {
        (full) = 0;
}
bool l_class_OC_Fifo1::first__RDY(void) {
        return (full);
}
unsigned int l_class_OC_Fifo1::first(void) {
        return (element);
}
