
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "l_class_OC_IVector.cpp"
#include "l_class_OC_Fifo1_OC_3.cpp"
#include "l_class_OC_FifoPong.cpp"

unsigned int stop_main_program;
class l_class_OC_IVectorIndication zIVectorIndication;
class l_class_OC_IVector zIVector;

bool l_class_OC_IVectorIndication::heard__RDY(void) {
        return true;
}
void l_class_OC_IVectorIndication::heard(l_struct_OC_ValuePair v) {
        printf((("Heard an echo: %d %d\n")), v.a, v.b);
        stop_main_program = 1;
}

int main(int argc, const char *argv[])
{
  printf("[%s:%d] starting %d\n", __FUNCTION__, __LINE__, argc);
    zIVector.setind(&zIVectorIndication);
    l_struct_OC_ValuePair tdata = {22, 44};
    zIVector.say(tdata);
    while (!stop_main_program) {
        zIVector.run();
    }
  printf("[%s:%d] ending\n", __FUNCTION__, __LINE__);
  return 0;
}

