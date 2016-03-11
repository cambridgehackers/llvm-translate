#include "l_class_OC_EchoIndicationOutput.h"
void l_class_OC_EchoIndicationOutput__heard(l_class_OC_EchoIndicationOutput *thisp, unsigned int heard_meth, unsigned int heard_v) {
        if ((thisp->even) != 0)
            thisp->ind1.tag = 1;
        if ((thisp->even) != 0)
            thisp->ind1.data.heard.meth = heard_meth;
        if ((thisp->even) != 0)
            thisp->ind1.data.heard.v = heard_v;
        if (((thisp->even) != 0) ^ 1)
            thisp->ind0.tag = 1;
        if (((thisp->even) != 0) ^ 1)
            thisp->ind0.data.heard.meth = heard_meth;
        if (((thisp->even) != 0) ^ 1)
            thisp->ind0.data.heard.v = heard_v;
        thisp->ind_busy = 1;
        thisp->even = ((thisp->even) != 0) ^ 1;
        printf("[%s:%d]EchoIndicationOutput even %d\n", ("heard"), 145, thisp->even);
}
bool l_class_OC_EchoIndicationOutput__heard__RDY(l_class_OC_EchoIndicationOutput *thisp) {
        return ((thisp->ind_busy) != 0) ^ 1;
}
void l_class_OC_EchoIndicationOutput__output_rulee(l_class_OC_EchoIndicationOutput *thisp) {
        thisp->ind_busy = 0;
        printf("[output_rulee:%d]EchoIndicationOutput tag %d\n", 165, thisp->ind0.tag);
        thisp->pipe->enq(thisp->ind0);
}
bool l_class_OC_EchoIndicationOutput__output_rulee__RDY(l_class_OC_EchoIndicationOutput *thisp) {
        bool tmp__1;
        tmp__1 = thisp->pipe->enq__RDY();
        return ((((thisp->ind_busy) != 0) & ((thisp->even) != 0)) != 0) & tmp__1;
}
void l_class_OC_EchoIndicationOutput__output_ruleo(l_class_OC_EchoIndicationOutput *thisp) {
        thisp->ind_busy = 0;
        printf("[output_ruleo:%d]EchoIndicationOutput tag %d\n", 170, thisp->ind1.tag);
        thisp->pipe->enq(thisp->ind1);
}
bool l_class_OC_EchoIndicationOutput__output_ruleo__RDY(l_class_OC_EchoIndicationOutput *thisp) {
        bool tmp__1;
        tmp__1 = thisp->pipe->enq__RDY();
        return ((((thisp->ind_busy) != 0) & ((thisp->even) == 0)) != 0) & tmp__1;
}
void l_class_OC_EchoIndicationOutput::run()
{
    if (output_rulee__RDY()) output_rulee();
    if (output_ruleo__RDY()) output_ruleo();
}
