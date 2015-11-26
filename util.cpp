/* Copyright (c) 2015 The Connectal Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
// Portions of this program were derived from source with the license:
//     This file is distributed under the University of Illinois Open Source
//     License. See LICENSE.TXT for details.
//#define DEBUG_TYPE "llvm-translate"
#include <stdio.h>
#include <list>
#include <cxxabi.h> // abi::__cxa_demangle
#include "llvm/ADT/STLExtras.h"

#define MAGIC_VTABLE_OFFSET 2

using namespace llvm;

#include "declarations.h"

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

void memdumpl(unsigned char *p, int len, const char *title)
{
int i;

    i = 0;
    while (len > 0) {
        if (!(i & 0x3f)) {
            if (i > 0)
                printf("\n");
            printf("%s: ",title);
        }
        uint64_t temp = *(uint64_t *)p;
        if (temp == 0x5a5a5a5a5a5a5a5a)
            printf("_ ");
        else
            //printf("0x%lx ", temp);
            printf("%s ", mapAddress((void *)temp).c_str());
        p += sizeof(uint64_t);
        i += sizeof(uint64_t);
        len -= sizeof(uint64_t);
    }
    printf("\n");
}

bool endswith(const char *str, const char *suffix)
{
    int skipl = strlen(str) - strlen(suffix);
    return skipl >= 0 && !strcmp(str + skipl, suffix);
}

std::string ucName(std::string inname)
{
    if (inname.length() && inname[0] >= 'a' && inname[0] <= 'z')
        return ((char)(inname[0] - 'a' + 'A')) + inname.substr(1, inname.length() - 1);
    return inname;
}

int getClassName(const char *name, const char **className, const char **methodName)
{
    int status;
    static char temp[1000];
    char *pmethod = temp;
    temp[0] = 0;
    *className = NULL;
    *methodName = NULL;
    const char *demang = abi::__cxa_demangle(name, 0, 0, &status);
    if (demang) {
        strcpy(temp, demang);
        while (*pmethod && pmethod[0] != '(')
            pmethod++;
        *pmethod = 0;
        while (pmethod > temp && pmethod[0] != ':')
            pmethod--;
        char *p = pmethod++;
        while (p[0] == ':')
            *p-- = 0;
        int len = 0;
        const char *p1 = demang;
        while (*p1 && *p1 != '(')
            p1++;
        while (p1 != demang && *p1 != ':') {
            len++;
            p1--;
        }
        *className = temp;
        *methodName = pmethod;
        return 1;
    }
    return 0;
}

std::string printString(std::string arg)
{
    const char *cp = arg.c_str();
    int len = arg.length();
    std::string cbuffer = "\"";
    char temp[100];
    if (!cp[len-1])
        len--;
    bool LastWasHex = false;
    for (unsigned i = 0, e = len; i != e; ++i) {
        unsigned char C = cp[i];
        if (isprint(C) && (!LastWasHex || !isxdigit(C))) {
            LastWasHex = false;
            if (C == '"' || C == '\\')
                cbuffer += "\\";
            sprintf(temp, "%c", (char)C);
            cbuffer += temp;
        } else {
            LastWasHex = false;
            switch (C) {
            case '\n': cbuffer += "\\n"; break;
            case '\t': cbuffer += "\\t"; break;
            case '\r': cbuffer += "\\r"; break;
            case '\v': cbuffer += "\\v"; break;
            case '\a': cbuffer += "\\a"; break;
            case '\"': cbuffer += "\\\""; break;
            case '\'': cbuffer += "\\\'"; break;
            default:
                cbuffer += "\\x";
                sprintf(temp, "%c", (char)(( C/16  < 10) ? ( C/16 +'0') : ( C/16 -10+'A')));
                cbuffer += temp;
                sprintf(temp, "%c", (char)(((C&15) < 10) ? ((C&15)+'0') : ((C&15)-10+'A')));
                cbuffer += temp;
                LastWasHex = true;
                break;
            }
        }
    }
    return cbuffer + "\"";
}
