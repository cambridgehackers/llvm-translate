#include "l_class_OC_Fifo_OC_0.h"
void l_class_OC_Fifo_OC_0__in_enq(void *thisarg, l_struct_OC_ValuePair in_enq_v) {
        l_class_OC_Fifo_OC_0 * thisp = (l_class_OC_Fifo_OC_0 *)thisarg;
}
bool l_class_OC_Fifo_OC_0__in_enq__RDY(void *thisarg) {
        l_class_OC_Fifo_OC_0 * thisp = (l_class_OC_Fifo_OC_0 *)thisarg;
        return 0;
}
void l_class_OC_Fifo_OC_0__out_deq(void *thisarg) {
        l_class_OC_Fifo_OC_0 * thisp = (l_class_OC_Fifo_OC_0 *)thisarg;
}
bool l_class_OC_Fifo_OC_0__out_deq__RDY(void *thisarg) {
        l_class_OC_Fifo_OC_0 * thisp = (l_class_OC_Fifo_OC_0 *)thisarg;
        return 0;
}
l_struct_OC_ValuePair l_class_OC_Fifo_OC_0__out_first(void *thisarg) {
        l_class_OC_Fifo_OC_0 * thisp = (l_class_OC_Fifo_OC_0 *)thisarg;
        l_struct_OC_ValuePair out_first;
        return {0};
        return out_first;
}
bool l_class_OC_Fifo_OC_0__out_first__RDY(void *thisarg) {
        l_class_OC_Fifo_OC_0 * thisp = (l_class_OC_Fifo_OC_0 *)thisarg;
        return 0;
}
void l_class_OC_Fifo_OC_0::run()
{
    commit();
}
void l_class_OC_Fifo_OC_0::commit()
{
}
