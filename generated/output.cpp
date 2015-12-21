void l_class_OC_Fifo::deq(void) {
}
bool l_class_OC_Fifo::deq__RDY(void) {
        return 0;
}
void l_class_OC_Fifo::enq(unsigned int v) {
}
bool l_class_OC_Fifo::enq__RDY(void) {
        return 0;
}
unsigned int l_class_OC_Fifo::first(void) {
        return 0;
}
bool l_class_OC_Fifo::first__RDY(void) {
        return 0;
}
void l_class_OC_Fifo1::deq(void) {
        full = 0;
}
bool l_class_OC_Fifo1::deq__RDY(void) {
        return full;
}
void l_class_OC_Fifo1::enq(unsigned int v) {
        element = v;
        full = 1;
}
bool l_class_OC_Fifo1::enq__RDY(void) {
        return full ^ 1;
}
unsigned int l_class_OC_Fifo1::first(void) {
        return element;
}
bool l_class_OC_Fifo1::first__RDY(void) {
        return full;
}
void l_class_OC_Echo::respond_rule(void) {
        unsigned int call = fifo.first();
        fifo.deq();
        ind->heard(call);
}
bool l_class_OC_Echo::respond_rule__RDY(void) {
        bool tmp__1 = fifo.first__RDY();
        bool tmp__2 = fifo.deq__RDY();
        bool tmp__3 = ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_Echo::say(unsigned int v) {
        fifo.enq(v);
}
bool l_class_OC_Echo::say__RDY(void) {
        bool tmp__1 = fifo.enq__RDY();
        return tmp__1;
}
void l_class_OC_Echo::run()
{
    if (respond_rule__RDY()) respond_rule();
}
