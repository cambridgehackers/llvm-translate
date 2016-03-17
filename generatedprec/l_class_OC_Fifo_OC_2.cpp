#include "l_class_OC_Fifo_OC_2.h"
void l_class_OC_Fifo_OC_2__deq(void *thisarg) {
        l_class_OC_Fifo_OC_2 * thisp = (l_class_OC_Fifo_OC_2 *)thisarg;
}
bool l_class_OC_Fifo_OC_2__deq__RDY(void *thisarg) {
        l_class_OC_Fifo_OC_2 * thisp = (l_class_OC_Fifo_OC_2 *)thisarg;
        return 0;
}
void l_class_OC_Fifo_OC_2__enq(void *thisarg, l_struct_OC_ValueType enq_v) {
        l_class_OC_Fifo_OC_2 * thisp = (l_class_OC_Fifo_OC_2 *)thisarg;
}
bool l_class_OC_Fifo_OC_2__enq__RDY(void *thisarg) {
        l_class_OC_Fifo_OC_2 * thisp = (l_class_OC_Fifo_OC_2 *)thisarg;
        return 0;
}
l_struct_OC_ValueType l_class_OC_Fifo_OC_2__first(void *thisarg) {
        l_class_OC_Fifo_OC_2 * thisp = (l_class_OC_Fifo_OC_2 *)thisarg;
        l_struct_OC_ValueType first;
        return first;
}
bool l_class_OC_Fifo_OC_2__first__RDY(void *thisarg) {
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
