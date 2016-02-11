
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "l_class_OC_IVector.cpp"
//#include "l_class_OC_Fifo_OC_0.cpp"
#include "l_class_OC_Fifo1_OC_3.cpp"
#include "l_class_OC_FifoPong.cpp"

unsigned int stop_main_program;
int testCount;
class l_class_OC_IVectorIndication zIVectorIndication;
class l_class_OC_IVector zIVector;

bool l_class_OC_IVectorIndication::heard__RDY(void) {
        return true;
}
void l_class_OC_IVectorIndication::heard(unsigned int meth, unsigned int v) {
        printf("Heard an echo: %d %d\n", meth, v);
        if (--testCount <= 0)
            stop_main_program = 1;
}

int main(int argc, const char *argv[])
{
  printf("[%s:%d] starting %d\n", __FUNCTION__, __LINE__, argc);
    zIVector.setind(&zIVectorIndication);
    for (int i = 0; i < 10; i++) {
        zIVector.say(i, 44 * i); testCount++;
    }
    while (!stop_main_program) {
        zIVector.run();
    }
  printf("[%s:%d] ending\n", __FUNCTION__, __LINE__);
  return 0;
}

