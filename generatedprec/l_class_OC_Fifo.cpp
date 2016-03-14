#include "l_class_OC_Fifo.h"
void l_class_OC_Fifo__in_enq(void *thisarg, unsigned int in_enq_v) {
        l_class_OC_Fifo * thisp = (l_class_OC_Fifo *)thisarg;
}
bool l_class_OC_Fifo__in_enq__RDY(void *thisarg) {
        l_class_OC_Fifo * thisp = (l_class_OC_Fifo *)thisarg;
        return 0;
}
void l_class_OC_Fifo__out_deq(void *thisarg) {
        l_class_OC_Fifo * thisp = (l_class_OC_Fifo *)thisarg;
}
bool l_class_OC_Fifo__out_deq__RDY(void *thisarg) {
        l_class_OC_Fifo * thisp = (l_class_OC_Fifo *)thisarg;
        return 0;
}
unsigned int l_class_OC_Fifo__out_first(void *thisarg) {
        l_class_OC_Fifo * thisp = (l_class_OC_Fifo *)thisarg;
        return 0;
}
bool l_class_OC_Fifo__out_first__RDY(void *thisarg) {
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
