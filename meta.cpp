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
//#include <stdio.h>
#include "llvm/IR/Instructions.h"
//#include "llvm/ADT/StringExtras.h"

using namespace llvm;

#include "declarations.h"

MetaRef readList, writeList, invokeList;

static int inhibitAppend;
void appendList(MetaRef &list, BasicBlock *cond, std::string item)
{
    if (!inhibitAppend) {
        Value *val = getCondition(cond, 0);
        if (!val)
            list[item].clear();
        for (auto condIter: list[item])
             if (!condIter)
                 return;
             else if (condIter == val)
                 return;
        list[item].push_back(val);
    }
}
std::string gatherList(MetaRef &list)
{
    std::string temp;
    inhibitAppend = 1;
    for (auto titem: list)
        for (auto item: titem.second)
            temp += printOperand(item,false) + ":" + titem.first + ";";
    inhibitAppend = 0;
    return temp;
}
