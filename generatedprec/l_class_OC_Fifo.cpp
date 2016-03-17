#include "l_class_OC_Fifo.h"
void l_class_OC_Fifo__deq(void *thisarg) {
        l_class_OC_Fifo * thisp = (l_class_OC_Fifo *)thisarg;
}
bool l_class_OC_Fifo__deq__RDY(void *thisarg) {
        l_class_OC_Fifo * thisp = (l_class_OC_Fifo *)thisarg;
        return 0;
}
void l_class_OC_Fifo__enq(void *thisarg, unsigned int enq_v) {
        l_class_OC_Fifo * thisp = (l_class_OC_Fifo *)thisarg;
}
bool l_class_OC_Fifo__enq__RDY(void *thisarg) {
        l_class_OC_Fifo * thisp = (l_class_OC_Fifo *)thisarg;
        return 0;
}
unsigned int l_class_OC_Fifo__first(void *thisarg) {
        l_class_OC_Fifo * thisp = (l_class_OC_Fifo *)thisarg;
        return 0;
}
bool l_class_OC_Fifo__first__RDY(void *thisarg) {
        l_class_OC_Fifo * thisp = (l_class_OC_Fifo *)thisarg;
        return 0;
}
void l_class_OC_Fifo::run()
{
    commit();
}
void l_class_OC_Fifo::commit()
{
}
