#include "l_class_OC_Connect.h"
void l_class_OC_Connect::run()
{
    lEchoIndicationOutput.run();
    lEchoRequestInput.run();
    lEcho.run();
    lEchoRequestOutput_test.run();
    lEchoIndicationInput_test.run();
    commit();
}
void l_class_OC_Connect::commit()
{
    lEchoIndicationOutput.commit();
    lEchoRequestInput.commit();
    lEcho.commit();
    lEchoRequestOutput_test.commit();
    lEchoIndicationInput_test.commit();
}
