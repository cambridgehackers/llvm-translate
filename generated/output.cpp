

/* Global Variable Definitions and Initialization */
class l_class_OC_EchoTest echoTest;
unsigned int stop_main_program;
//processing printf
void l_class_OC_EchoIndication::echo(unsigned int v) {
        printf((("Heard an echo: %d\n")), v);
        stop_main_program = 1;
}
bool l_class_OC_Fifo1::deq__RDY(void) {
        return (full);
}
bool l_class_OC_Fifo1::enq__RDY(void) {
        return ((full) ^ 1);
}
void l_class_OC_Fifo1::enq(unsigned int v) {
        element = v;
        full = 1;
}
void l_class_OC_Fifo1::deq(void) {
        full = 0;
}
bool l_class_OC_Fifo1::first__RDY(void) {
        return (full);
}
unsigned int l_class_OC_Fifo1::first(void) {
        return (element);
}
bool l_class_OC_Echo::rule_respond__RDY(void) {
        return (fifo.deq__RDY()) & (fifo.first__RDY());
}
void l_class_OC_Echo::rule_respond(void) {
        fifo.deq();
        (ind)->echo((fifo.first()));
}
void l_class_OC_Echo::run()
{
    if (rule_respond__RDY()) rule_respond();
}
void l_class_OC_EchoTest::rule_drive(void) {
        echo->fifo.enq(22);
}
bool l_class_OC_EchoTest::rule_drive__RDY(void) {
        return (echo->fifo.enq__RDY());
}
void l_class_OC_EchoTest::run()
{
    if (rule_drive__RDY()) rule_drive();
    echo->run();
}
