#include "l_class_OC_XsimTop.h"
void l_class_OC_XsimTop::run()
{
    top.run();
    lMMURequestInput.run();
    lMMUIndicationOutput.run();
    lMemServerIndicationOutput.run();
    lMemServerRequestInput.run();
    lMMU.run();
    lMemServer.run();
}
