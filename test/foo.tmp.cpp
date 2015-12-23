
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

unsigned char *llvm_translate_malloc(unsigned long long size)
{
    return (unsigned char *)malloc(size);
}
unsigned char *memset(unsigned char *b, int c, unsigned long long len)
{
    return (unsigned char *)memset((void *)b, c, len);
}
// operator delete(void *)
void _ZdlPv(unsigned char*)
{
}
void __cxa_pure_virtual(void)
{
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
}

unsigned int stop_main_program;
//class l_class_OC_EchoIndication {
//public:
  //void echo(unsigned int Vv);
//};
//
#include "../generatedb/output.h"
#include "../generatedb/output.cpp"
#include "../generated/output.h"
#include "../generated/output.cpp"

bool l_class_OC_EchoIndication::heard__RDY(void) {
        return true;
}
void l_class_OC_EchoIndication::heard(unsigned int v) {
        printf((("Heard an echo: %d\n")), v);
        stop_main_program = 1;
}

class l_class_OC_EchoIndication zEchoIndication;
class l_class_OC_Echo zEcho;

void memdump(unsigned char *p, int len, const char *title)
{
int i;

    i = 0;
    while (len > 0) {
        if (!(i & 0xf)) {
            if (i > 0)
                printf("\n");
            printf("%s: ",title);
        }
        printf("%02x ", *p++);
        i++;
        len--;
    }
    printf("\n");
}

int main(int argc, const char *argv[])
{
  printf("[%s:%d] starting %d\n", __FUNCTION__, __LINE__, argc);
    zEcho.setind(&zEchoIndication);
    zEcho.say(22);
    while (!stop_main_program) {
        zEcho.run();
    }
  printf("[%s:%d] ending\n", __FUNCTION__, __LINE__);
  return 0;
}

