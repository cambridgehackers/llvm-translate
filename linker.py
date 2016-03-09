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
import argparse, json, re, os, sys, shutil, string

argparser = argparse.ArgumentParser('Generate verilog schedule.')
argparser.add_argument('--directory', help='directory', default=[], action='append')
argparser.add_argument('--output', help='linked top level', default='')
argparser.add_argument('verilog', help='Verilog files to parse', nargs='+')

mInfo = {}

SCANNER = re.compile(r'''
  (\s+) |                      # whitespace
  (<<|>>|[][(){}<>=,;:*+-/^&]) | # punctuation
  ([0-9A-Za-z_][$A-Za-z0-9_]*) |   # identifiers, numbers
  "((?:[^"\n\\]|\\.)*)" |      # regular string literal
  (.)                          # an error!
''', re.DOTALL | re.VERBOSE)

def prependName(name, string):
    retVal = ''
    for match in re.finditer(SCANNER, string):
        for i in range(1, 5):
            tfield = match.group(i)
            if tfield:
                if i == 5:
                    print 'Error in regex', string, tfield
                if i == 3 and (tfield[0] < '0' or tfield[0] > '9'):
                    retVal += name
                retVal += tfield
    #print 'prependName', name, string, ' -> ', retVal
    return retVal

def expandGuard(mitem, name, string):
    retVal = ''
    for match in re.finditer(SCANNER, string):
        for i in range(1, 5):
            tfield = match.group(i)
            if tfield:
                if i == 5:
                    print 'Error in regex', string, tfield
                if i == 3 and (tfield[0] < '0' or tfield[0] > '9'):
                    if tfield.endswith('__RDY'):
                        tsep = tfield.split('$')
                        item = mitem['internal'].get(tsep[0])
                        if item:
                            rmitem = mInfo[item]
                            rtitem = rmitem['methods'][tsep[1][:-5]]
                            tfield = expandGuard(rmitem, tsep[0] + '$', rtitem['guard'])
                        else:
                            tfield = name + tfield
                    else:
                        tfield = name + tfield
                retVal += tfield
    #print 'expandGuard', string, ' -> ', retVal
    return retVal

def processFile(filename):
    print 'infile', filename
    if mInfo.get(filename) is not None:
        return
    titem = {}
    titem['name'] = filename
    titem['rules'] = []
    titem['methods'] = {}
    titem['internal'] = {}
    titem['external'] = {}
    mInfo[filename] = titem
    fileDesc = None
    for item in options.directory:
        try:
            fileDesc = open(item + '/' + filename + '.vh')
        except:
            continue
        break
    print 'DD', fileDesc
    if fileDesc is not None:
        with fileDesc as inFile:
            for inLine in inFile:
                if inLine.startswith('//META'):
                    inVector = map(str.strip, inLine.strip().split(';'))
                    if inVector[-1] == '':
                        inVector.pop()
                    #print 'MM', inLine, inVector
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
                    elif inVector[0] == '//METARULES':
                        titem['rules'] = inVector[1:]
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
    print 'ALL', json.dumps(titem, sort_keys=True, indent = 4)
    for key, value in titem['internal'].iteritems():
        processFile(value)
    for key, value in titem['external'].iteritems():
        titem = {}
        titem['name'] = value
        titem['methods'] = {}
        titem['internal'] = {}
        titem['external'] = {}
        mInfo[value] = titem

def getList(filename, mname, field):
    #print 'getlist', filename, mname, field
    mitem = mInfo[filename]
    titem = mitem['methods'][mname]
    retVal = titem[field]
    tguard = expandGuard(mitem, '', titem['guard'])
    titem['nguard'] = tguard
    for iitem in titem['invoke']:
        for item in iitem[1:]:
            refName = item.split('$')
            if mitem['internal'].get(refName[0]):
                gitem, rList = getList(mitem['internal'][refName[0]], refName[1], field)
                for rItem in rList:
                    gVal = prependName(refName[0] + "$", gitem['guard'])
                    if rItem[0] == '':
                        rItem[0] = gVal
                    elif gVal != '':
                        rItem[0] = rItem[0] + ' & ' +  gVal
                    gVal = tguard
                    if rItem[0] == '':
                        rItem[0] = gVal
                    elif gVal != '':
                        rItem[0] = rItem[0] + ' & ' +  gVal
                    for ind in range(1, len(rItem)):
                        rItem[ind] = refName[0] + "$" + rItem[ind]
                    retVal.append(rItem)
    return titem, retVal

def testEqual(A, B):
    if isinstance(A, list) and isinstance(B, list) and len(A) == len(B):
        for ind in range(0, len(A) - 1):
            if not testEqual(A[ind], B[ind]):
                return False
        return True
    if isinstance(A, str) and isinstance(B, str):
        return A == B
    return False

def appendItem(lis, item):
    if len(lis) >= 2 and isinstance(lis[-1], str) and lis[-1] == '&' and testEqual(lis[-2], item):
        lis.pop()
    else:
        lis.append(item)

def parseExpression(string):
    retVal = [[]]
    ind = 0
    for match in re.finditer(SCANNER, string):
        for i in range(1, 5):
            tfield = match.group(i)
            if tfield == '(':
                ind += 1
                retVal.append([])
            elif tfield == ')':
                rVal = retVal[ind]
                if len(rVal) == 1:
                    rVal = rVal[0]
                ind -= 1
                appendItem(retVal[ind], rVal)
                retVal.pop()
            elif tfield:
                if i == 5:
                    print 'Error in regex', string, tfield
                    sys.exit(-1)
                if i != 1:
                    appendItem(retVal[ind], tfield)
    while isinstance(retVal, list) and len(retVal) == 1:
        retVal = retVal[0]
    print 'parseExpression', string, ' -> ', retVal, ind
    return retVal

if __name__=='__main__':
    options = argparser.parse_args()
    for item in options.verilog:
        processFile(item)
    for item in options.verilog:
        for key, value in mInfo[item]['methods'].iteritems():
            tignore, rList = getList(item, key, 'read')
            print 'readList', key
            for rItem in rList:
                print parseExpression(rItem[0]), rItem[1:]
            tignore, wList = getList(item, key, 'write')
            print 'writeList', key
            for wItem in wList:
                print parseExpression(wItem[0]), wItem[1:]
    if options.output:
        print 'output', options.output
        for key, titem in mInfo.iteritems():
            print 'ALL', key, json.dumps(titem, sort_keys=True, indent = 4)
            pass

