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

class l_class_OC_EchoRequest {
public:
    void say(int say_meth, int say_v) {
        printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    bool say__RDY(void) { return true;}
};
class l_class_OC_EchoIndication {
public:
    void heard(int heard_meth, int heard_v) {
        printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    bool heard__RDY(void) { return true;}
};

#include "l_struct_OC_EchoRequest_data.h"
#include "l_struct_OC_EchoIndication_data.h"
#define request_say say
#define request_say__RDY say__RDY
#include "l_class_OC_EchoRequestInput.h"
#include "l_class_OC_EchoIndicationInput.h"
typedef l_class_OC_EchoRequestInput l_class_OC_PipeIn;
typedef l_class_OC_EchoIndicationInput l_class_OC_PipeIn_OC_0;

#include "l_struct_OC_ValueType.h"   // HACKHACK -> need to scan method bodies for used datatypes
#include "l_class_OC_Connect.cpp"
#include "l_class_OC_Echo.cpp"
#include "l_class_OC_EchoIndicationOutput.cpp"
#include "l_class_OC_EchoRequestInput.cpp"
#include "l_class_OC_EchoRequestOutput.cpp"
#include "l_class_OC_EchoIndicationInput.cpp"

unsigned int stop_main_program;
int testCount;
class l_class_OC_ConnectIndication zConnectIndication;
class l_class_OC_Connect zConnect;

bool l_class_OC_ConnectIndication::heard__RDY(void) {
        return true;
}
void l_class_OC_ConnectIndication::heard(unsigned int meth, unsigned int v) {
        printf("Heard an echo: %d %d\n", meth, v);
        if (--testCount <= 0)
            stop_main_program = 1;
}

int main(int argc, const char *argv[])
{
  printf("[%s:%d] starting %d\n", __FUNCTION__, __LINE__, argc);
    zConnect.setind(&zConnectIndication);
    zConnect.say(1, 44 * 1); testCount++;
    while (!stop_main_program) {
        zConnect.run();
    }
  printf("[%s:%d] ending\n", __FUNCTION__, __LINE__);
  return 0;
}


