// Copyright (c) 2015 The Lpmal Project

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

#define fifo8 fifo
#define inQ8 inQ
#define outQ8 outQ
#include "l_class_OC_Lpm.cpp"
#include "l_class_OC_LpmMemory.cpp"
#include "l_class_OC_Fifo1_OC_0.cpp"

unsigned int stop_main_program;
int testCount;
l_class_OC_Lpm zLpm;

void respheard(void *thisp, unsigned int heard_meth, unsigned int heard_v) {
    printf("Heard an echo: %d %d\n", heard_meth, heard_v);
    if (--testCount <= 0)
        stop_main_program = 1;
};
bool respheard__RDY(void *thisp) { return true;}

int main(int argc, const char *argv[])
{
    printf("[%s:%d] starting %d\n", __FUNCTION__, __LINE__, argc);
    zLpm.setindication(new l_class_OC_LpmIndication(NULL, respheard__RDY, respheard));
    testCount++; testCount++; testCount++;
    while(!zLpm.say__RDY()) zLpm.run(); zLpm.say(1, 44 * 1);
    while(!zLpm.say__RDY()) zLpm.run(); zLpm.say(2, 44 * 2);
    while(!zLpm.say__RDY()) zLpm.run(); zLpm.say(3, 44 * 3);
    while (!stop_main_program) {
        zLpm.run();
    }
    printf("[%s:%d] ending\n", __FUNCTION__, __LINE__);
    return 0;
}

