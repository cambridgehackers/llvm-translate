#include "l_class_OC_Echo.h"
void l_class_OC_Echo__say(void *thisarg, unsigned int say_meth, unsigned int say_v) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        thisp->indication->heard(say_meth, say_v);
}
bool l_class_OC_Echo__say__RDY(void *thisarg) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        bool tmp__1;
        tmp__1 = thisp->indication->heard__RDY();
        return tmp__1;
}
void l_class_OC_Echo::run()
{
    commit();
}
void l_class_OC_Echo::commit()
{
}
