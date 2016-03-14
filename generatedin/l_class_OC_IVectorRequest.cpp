#include "l_class_OC_IVectorRequest.h"
void l_class_OC_IVectorRequest__say(void *thisarg, unsigned int say_meth, unsigned int say_v) {
        l_class_OC_IVectorRequest * thisp = (l_class_OC_IVectorRequest *)thisarg;
}
bool l_class_OC_IVectorRequest__say__RDY(void *thisarg) {
        l_class_OC_IVectorRequest * thisp = (l_class_OC_IVectorRequest *)thisarg;
        return 1;
}
void l_class_OC_IVectorRequest::run()
{
    commit();
}
void l_class_OC_IVectorRequest::commit()
{
}
