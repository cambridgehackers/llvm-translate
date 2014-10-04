
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

unsigned int stop_main_program;
unsigned char *llvm_translate_malloc(size_t size)
{
    return (unsigned char *)malloc(size);
}
unsigned char *memset(unsigned char *b, int c, size_t len)
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
#include "../foo.tmp.h"
#include "../foo.tmp.xc"

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
  _GLOBAL__I_a();
    while (!stop_main_program) {
        struct l_class_OC_Module *curmod = _ZN6Module5firstE;
        while (curmod) {
            struct l_class_OC_Rule *currule = curmod->field1;
            while (currule) {
                ((void (*)(void *))currule->field0[3])(currule);
                if (((int (*)(void *))currule->field0[2])(currule)) {
                    ((void (*)(void *))currule->field0[4])(currule);
                }
                currule = currule->field1;
            }
            curmod = curmod->field2;
        }
    }
  printf("[%s:%d] ending\n", __FUNCTION__, __LINE__);
  return 0;
}

