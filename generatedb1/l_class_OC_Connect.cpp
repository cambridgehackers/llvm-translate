#include "l_class_OC_Connect.h"
void l_class_OC_Connect__swap2_rule(void *thisarg) {
        l_class_OC_Connect * thisp = (l_class_OC_Connect *)thisarg;
        printf("swap2_rule:Connect\n");
        thisp->lEcho.y2xnull();
}
bool l_class_OC_Connect__swap2_rule__RDY(void *thisarg) {
        l_class_OC_Connect * thisp = (l_class_OC_Connect *)thisarg;
        bool tmp__1;
        tmp__1 = thisp->lEcho.y2xnull__RDY();
        return tmp__1;
}
void l_class_OC_Connect__swap_rule(void *thisarg) {
        l_class_OC_Connect * thisp = (l_class_OC_Connect *)thisarg;
        printf("swap_rule:Connect\n");
        thisp->lEcho.x2y();
        thisp->lEcho.y2x();
}
bool l_class_OC_Connect__swap_rule__RDY(void *thisarg) {
        l_class_OC_Connect * thisp = (l_class_OC_Connect *)thisarg;
        bool tmp__1;
        bool tmp__2;
        tmp__1 = thisp->lEcho.x2y__RDY();
        tmp__2 = thisp->lEcho.y2x__RDY();
        return tmp__1 & tmp__2;
}
void l_class_OC_Connect::run()
{
    if (swap2_rule__RDY()) swap2_rule();
    if (swap_rule__RDY()) swap_rule();
    lEIO.run();
    lERI.run();
    lEcho.run();
    lERO_test.run();
    lEII_test.run();
    commit();
}
void l_class_OC_Connect::commit()
{
    lEIO.commit();
    lERI.commit();
    lEcho.commit();
    lERO_test.commit();
    lEII_test.commit();
}
