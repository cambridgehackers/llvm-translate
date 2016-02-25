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
#include "runtime.h"
#define l_class_OC_PipeIn l_class_OC_EchoRequestInput 
#define l_class_OC_PipeIn_OC_0 l_class_OC_EchoIndicationInput 
#define l_class_OC_foo l_class_OC_EchoIndication
#define request0 request
#include "l_class_OC_EchoIndication.h"
#include "l_class_OC_EchoRequest.h"
#include "l_class_OC_Connect.cpp"
#include "l_class_OC_Echo.cpp"
#include "l_class_OC_EchoIndicationOutput.cpp"
#include "l_class_OC_EchoRequestInput.cpp"
#include "l_class_OC_EchoRequestOutput.cpp"
#include "l_class_OC_EchoIndicationInput.cpp"

unsigned int stop_main_program;
int testCount;
class l_class_OC_Connect zConnect;

class foo: public l_class_OC_EchoIndication {
    void heard(unsigned int heard_meth, unsigned int heard_v) {
        printf("Heard an echo: %d %d\n", heard_meth, heard_v);
        if (--testCount <= 0)
            stop_main_program = 1;
    };
    bool heard__RDY(void) { return true;}
};
class foo zConnectresp;

int main(int argc, const char *argv[])
{
  printf("[%s:%d] starting %d\n", __FUNCTION__, __LINE__, argc);
zConnect.lEchoRequestInput.setrequest(&zConnect.lEcho);
zConnect.lEcho.setindication(&zConnect.lEchoIndicationOutput);
zConnect.lEchoIndicationOutput.setpipe(&zConnect.lEchoIndicationInput_test);

zConnect.lEchoRequestOutput_test.setpipe(&zConnect.lEchoRequestInput);
zConnect.lEchoIndicationInput_test.setrequest(&zConnectresp);
    zConnect.lEchoRequestOutput_test.say(1, 44 * 1); testCount++;
    while (!stop_main_program) {
        zConnect.run();
    }
  printf("[%s:%d] ending\n", __FUNCTION__, __LINE__);
  return 0;
}


