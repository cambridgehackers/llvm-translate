#include "l_class_OC_EchoRequest.h"
void l_class_OC_EchoRequest__say(void *thisarg, unsigned int say_v) {
        l_class_OC_EchoRequest * thisp = (l_class_OC_EchoRequest *)thisarg;
}
bool l_class_OC_EchoRequest__say__RDY(void *thisarg) {
        l_class_OC_EchoRequest * thisp = (l_class_OC_EchoRequest *)thisarg;
        return 1;
}
void l_class_OC_EchoRequest::run()
{
    commit();
}
void l_class_OC_EchoRequest::commit()
{
}
