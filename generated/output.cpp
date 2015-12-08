

/* Global Variable Definitions and Initialization */
class l_class_OC_EchoTest echoTest;
unsigned int stop_main_program;
//processing printf
void l_class_OC_EchoIndication::echo(unsigned int v) {
        printf((("Heard an echo: %d\n")), v);
        stop_main_program = 1;
}
void l_class_OC_Fifo1::enq(unsigned int v) {
        element = v;
        full = 1;
}
bool l_class_OC_Fifo1::deq__RDY(void) {
    bool call =     this->notEmpty();
        return call;
}
void l_class_OC_Fifo1::deq(void) {
        full = 0;
}
bool l_class_OC_Fifo1::first__RDY(void) {
    bool call =     this->notEmpty();
        return call;
}
unsigned int l_class_OC_Fifo1::first(void) {
        return (element);
}
bool l_class_OC_Fifo1::notEmpty(void) {
        return (full);
}
bool l_class_OC_Echo::echoReq__RDY(void) {
        return 1;
}
void l_class_OC_Echo::echoReq(unsigned int v) {
        fifo.enq(v);
}
void l_class_OC_Echo::rule_respondexport(void) {
        this->echoReq(99);
}
bool l_class_OC_Echo::rule_respondexport__RDY(void) {
    bool tmp__1 =     this->echoReq__RDY();
        return tmp__1;
}
void l_class_OC_Echo::rule_respond(void) {
    unsigned int call =     fifo.first();
        fifo.deq();
        (ind)->echo(call);
}
bool l_class_OC_Echo::rule_respond__RDY(void) {
    bool tmp__1 =     fifo.deq__RDY();
    bool tmp__2 =     fifo.first__RDY();
        return (tmp__1 & tmp__2);
}
void l_class_OC_Echo::run()
{
    if (rule_respondexport__RDY()) rule_respondexport();
    if (rule_respond__RDY()) rule_respond();
}
