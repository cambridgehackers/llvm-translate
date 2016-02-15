// Copyright (c) 2015 The Connectal Project

// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#if 0
typedef uint32_t BITS4;
typedef uint32_t BITS6;
typedef uint32_t BITS; // for FixedPointV
#else
class BITS {
public:
    uint32_t data;
    BITS(): data(0) {}
    BITS(uint32_t arg): data(arg) {}
    ~BITS() {}
    inline BITS & operator=(const BITS & arg) {
        this->data = arg.data;
        return *this;
    }
};
typedef BITS BITS4;
typedef BITS BITS6;
#endif
#include "l_struct_OC_ValueType.h"   // HACKHACK -> need to scan method bodies for used datatypes
//#include "l_class_OC_Fifo1.cpp"
//#define __l_class_OC_Fifo_OC_1_H__
//typedef l_class_OC_Fifo1 l_class_OC_Fifo_OC_1;
#include "l_class_OC_IVector.cpp"
#include "l_class_OC_Fifo1_OC_1.cpp"
//#include "l_class_OC_Fifo1_OC_3.cpp"
//#include "l_class_OC_FifoPong.cpp"

unsigned int stop_main_program;
int testCount;
class l_class_OC_IVectorIndication zIVectorIndication;
class l_class_OC_IVector zIVector;

bool l_class_OC_IVectorIndication::heard__RDY(void) {
        return true;
}
void l_class_OC_IVectorIndication::heard(BITS4 meth, BITS6 v) {
        printf("Heard an echo: %d %d\n", meth.data, v.data);
        if (--testCount <= 0)
            stop_main_program = 1;
}

int main(int argc, const char *argv[])
{
  printf("[%s:%d] starting %d\n", __FUNCTION__, __LINE__, argc);
    zIVector.setind(&zIVectorIndication);
    //for (int i = 0; i < 10; i++) {
        zIVector.say(1, 44 * 1); testCount++;
    //}
    while (!stop_main_program) {
        zIVector.run();
    }
  printf("[%s:%d] ending\n", __FUNCTION__, __LINE__);
  return 0;
}


