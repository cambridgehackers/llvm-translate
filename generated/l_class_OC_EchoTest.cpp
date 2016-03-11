#include "l_class_OC_EchoTest.h"
void l_class_OC_EchoTest::run()
{
    commit();
}
void l_class_OC_EchoTest::commit()
{
    if (x_valid) x = x_shadow;
    x_valid = 0;
}
