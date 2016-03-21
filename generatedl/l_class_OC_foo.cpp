#include "l_class_OC_foo.h"
void l_class_OC_foo__heard(void *thisarg, unsigned int meth, unsigned int v) {
        l_class_OC_foo * thisp = (l_class_OC_foo *)thisarg;
        unsigned int meth_2e_addr;
        l_class_OC_foo *this_2e_addr;
        unsigned int v_2e_addr;
        this_2e_addr = this;
        meth_2e_addr = meth;
        v_2e_addr = v;
        stop_main_program = 1;
        printf("Heard an lpm: %d %d\n", meth_2e_addr, v_2e_addr);
}
bool l_class_OC_foo__heard__RDY(void *thisarg) {
        l_class_OC_foo * thisp = (l_class_OC_foo *)thisarg;
        l_class_OC_foo *this_2e_addr;
        this_2e_addr = this;
        return 1;
}
void l_class_OC_foo::run()
{
    commit();
}
void l_class_OC_foo::commit()
{
}
