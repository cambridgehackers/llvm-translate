#include "l_class_OC_LpmTest.h"
void l_class_OC_LpmTest::run()
{
    lpm.run();
    commit();
}
void l_class_OC_LpmTest::commit()
{
    lpm.commit();
}
