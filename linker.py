#!/usr/bin/python
## Copyright (c) 2015 The Connectal Project

## Permission is hereby granted, free of charge, to any person
## obtaining a copy of this software and associated documentation
## files (the "Software"), to deal in the Software without
## restriction, including without limitation the rights to use, copy,
## modify, merge, publish, distribute, sublicense, and/or sell copies
## of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:

## The above copyright notice and this permission notice shall be
## included in all copies or substantial portions of the Software.

## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
## EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
## MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
## NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
## BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
## ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
## CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
## SOFTWARE.
import json
import os, sys, shutil, string
import argparse
#import re

argparser = argparse.ArgumentParser('Generate verilog schedule.')
argparser.add_argument('--directory', help='directory', default='')
argparser.add_argument('verilog', help='Verilog files to parse', nargs='+')

mInfo = {}

def processFile(filename):
    print 'infile', filename
    titem = {}
    titem['name'] = filename
    titem['methods'] = {}
    titem['internal'] = {}
    titem['external'] = {}
    mInfo[filename] = titem
    with open(options.directory + '/' + filename + '.v') as inFile:
        for inLine in inFile:
            if inLine.startswith('//META'):
                inVector = map(str.strip, inLine.strip().split(';'))
                if inVector[-1] == '':
                    inVector.pop()
                if inVector[0] == '//METAGUARD':
                    if not inVector[1].endswith('__RDY'):
                        print 'Guard name invalid', invector
                        sys.exit(-1)
                    tempName = inVector[1][:-5]
                    titem['methods'][tempName] = {}
                    titem['methods'][tempName]['guard'] = inVector[2]
                    titem['methods'][tempName]['read'] = []
                    titem['methods'][tempName]['write'] = []
                    titem['methods'][tempName]['invoke'] = []
                elif inVector[0] == '//METAINTERNAL':
                    titem['internal'][inVector[1]] = inVector[2]
                elif inVector[0] == '//METAEXTERNAL':
                    titem['external'][inVector[1]] = inVector[2]
                elif inVector[0] == '//METAINVOKE':
                    titem['methods'][inVector[1]]['invoke'].append(inVector[2].split(':'))
                elif inVector[0] == '//METAREAD':
                    titem['methods'][inVector[1]]['read'].append(inVector[2].split(':'))
                elif inVector[0] == '//METAWRITE':
                    titem['methods'][inVector[1]]['write'].append(inVector[2].split(':'))
                else:
                    print 'Unknown case', inVector
                #print 'LL', inVector
    #print 'ALL', json.dumps(titem, separators=(',',':'), sort_keys=True)
    print 'ALL', json.dumps(titem, sort_keys=True, indent = 4)
    for key, value in titem['internal'].iteritems():
        processFile(value)


if __name__=='__main__':
    options = argparser.parse_args()
    for item in options.verilog:
        processFile(item)
