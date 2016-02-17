#include "l_class_OC_MemreadRequestInput.h"
void l_class_OC_MemreadRequestInput::pipe_enq(unsigned int pipe_enq_v_2e_coerce) {
        l_struct_OC_MemreadRequest_data v;
        v.tag = pipe_enq_v_2e_coerce;
}
bool l_class_OC_MemreadRequestInput::pipe_enq__RDY(void) {
        return 0;
}
void l_class_OC_MemreadRequestInput::run()
{
}
