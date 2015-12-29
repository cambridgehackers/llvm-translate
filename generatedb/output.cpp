void l_class_OC_Fifo::in_enq(unsigned int in_enq_v) {
}
bool l_class_OC_Fifo::in_enq__RDY(void) {
        return 0;
}
void l_class_OC_Fifo::out_deq(void) {
}
bool l_class_OC_Fifo::out_deq__RDY(void) {
        return 0;
}
unsigned int l_class_OC_Fifo::out_first(void) {
        return 0;
}
bool l_class_OC_Fifo::out_first__RDY(void) {
        return 0;
}
void l_class_OC_Fifo1::in_enq(unsigned int in_enq_v) {
        element = in_enq_v;
        full = 1;
}
bool l_class_OC_Fifo1::in_enq__RDY(void) {
        return full ^ 1;
}
void l_class_OC_Fifo1::out_deq(void) {
        full = 0;
}
bool l_class_OC_Fifo1::out_deq__RDY(void) {
        return full;
}
unsigned int l_class_OC_Fifo1::out_first(void) {
        return element;
}
bool l_class_OC_Fifo1::out_first__RDY(void) {
        return full;
}
