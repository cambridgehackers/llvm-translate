void l_class_OC_Fifo::deq(void) {
}
bool l_class_OC_Fifo::deq__RDY(void) {
        return 0;
}
void l_class_OC_Fifo::enq(unsigned int enq_v) {
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
void l_class_OC_Fifo1::enq(unsigned int enq_v) {
        element = enq_v;
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
void l_class_OC_Fifo_OC_1::deq(void) {
}
bool l_class_OC_Fifo_OC_1::deq__RDY(void) {
        return 0;
}
void l_class_OC_Fifo_OC_1::enq(bool enq_v) {
}
bool l_class_OC_Fifo_OC_1::enq__RDY(void) {
        return 0;
}
bool l_class_OC_Fifo_OC_1::first(void) {
        return 0;
}
bool l_class_OC_Fifo_OC_1::first__RDY(void) {
        return 0;
}
void l_class_OC_Fifo1_OC_0::deq(void) {
        full = 0;
}
bool l_class_OC_Fifo1_OC_0::deq__RDY(void) {
        return full;
}
void l_class_OC_Fifo1_OC_0::enq(bool enq_v) {
        element = enq_v;
        full = 1;
}
bool l_class_OC_Fifo1_OC_0::enq__RDY(void) {
        return full ^ 1;
}
bool l_class_OC_Fifo1_OC_0::first(void) {
        return element;
}
bool l_class_OC_Fifo1_OC_0::first__RDY(void) {
        return full;
}
