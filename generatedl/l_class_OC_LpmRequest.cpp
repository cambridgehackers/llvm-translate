#include "l_class_OC_LpmRequest.h"
void l_class_OC_LpmRequest__say(void *thisarg, unsigned int say_meth, unsigned int say_v) {
        l_class_OC_LpmRequest * thisp = (l_class_OC_LpmRequest *)thisarg;
}
bool l_class_OC_LpmRequest__say__RDY(void *thisarg) {
        l_class_OC_LpmRequest * thisp = (l_class_OC_LpmRequest *)thisarg;
        return 1;
}
void l_class_OC_LpmRequest::run()
{
    commit();
}
void l_class_OC_LpmRequest::commit()
{
}
