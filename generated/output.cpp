void l_class_OC_Fifo1::deq(void) {
        full = 0;
}
bool l_class_OC_Fifo1::deq__RDY(void) {
        return (full);
}
void l_class_OC_Fifo1::enq(unsigned int v) {
        element = v;
        full = 1;
}
bool l_class_OC_Fifo1::enq__RDY(void) {
        return ((full) ^ 1);
}
unsigned int l_class_OC_Fifo1::first(void) {
        return (element);
}
bool l_class_OC_Fifo1::first__RDY(void) {
        return (full);
}
void l_class_OC_Echo::echoReq(unsigned int v) {
        fifo.enq(v);
}
bool l_class_OC_Echo::echoReq__RDY(void) {
        bool tmp__1 = fifo.enq__RDY();
        return tmp__1;
}
void l_class_OC_Echo::rule_respond(void) {
        unsigned int call = fifo.first();
        fifo.deq();
        (ind)->echo(call);
}
bool l_class_OC_Echo::rule_respond__RDY(void) {
        bool tmp__1 = fifo.deq__RDY();
        bool tmp__2 = fifo.first__RDY();
        return (tmp__1 & tmp__2);
}
void l_class_OC_Echo::run()
{
    if (rule_respond__RDY()) rule_respond();
}
