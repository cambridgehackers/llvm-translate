#include "l_class_OC_Fifo_OC_2.h"
void l_class_OC_Fifo_OC_2__in_enq(void *thisarg, l_struct_OC_ValueType in_enq_v) {
        l_class_OC_Fifo_OC_2 * thisp = (l_class_OC_Fifo_OC_2 *)thisarg;
}
bool l_class_OC_Fifo_OC_2__in_enq__RDY(void *thisarg) {
        l_class_OC_Fifo_OC_2 * thisp = (l_class_OC_Fifo_OC_2 *)thisarg;
        return 0;
}
void l_class_OC_Fifo_OC_2__out_deq(void *thisarg) {
        l_class_OC_Fifo_OC_2 * thisp = (l_class_OC_Fifo_OC_2 *)thisarg;
}
bool l_class_OC_Fifo_OC_2__out_deq__RDY(void *thisarg) {
        l_class_OC_Fifo_OC_2 * thisp = (l_class_OC_Fifo_OC_2 *)thisarg;
        return 0;
}
l_struct_OC_ValueType l_class_OC_Fifo_OC_2__out_first(void *thisarg) {
        l_class_OC_Fifo_OC_2 * thisp = (l_class_OC_Fifo_OC_2 *)thisarg;
        l_struct_OC_ValueType out_first;
        return out_first;
}
bool l_class_OC_Fifo_OC_2__out_first__RDY(void *thisarg) {
        l_class_OC_Fifo_OC_2 * thisp = (l_class_OC_Fifo_OC_2 *)thisarg;
        return 0;
}
void l_class_OC_Fifo_OC_2::run()
{
    commit();
}
void l_class_OC_Fifo_OC_2::commit()
{
}
