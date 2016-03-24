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

traceList = False
traceAll = False
removeUnref = False
mInfo = {}

SCANNER = re.compile(r'''
  (\s+) |                      # whitespace
  (<<|>>|[][(){}<>=,;:*+-/^&!%]) | # punctuation
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
                            methodItem = rmitem['methods']['$'.join(tsep[1:])[:-5]]
                            tfield = expandGuard(rmitem, tsep[0] + '$', methodItem['guard'])
                        else:
                            tfield = name + tfield
                    else:
                        tfield = name + tfield
                retVal += tfield
    #print 'expandGuard', string, ' -> ', retVal
    return retVal

def splitName(value):
    valsplit = value.split('$')
    return valsplit[0], '$'.join(valsplit[1:])

def splitRef(value):
    temp = value.split(':')
    if len(temp) == 1:
        temp.insert(0, '')
    return temp

def processFile(moduleName):
    print 'processFile:', moduleName
    if mInfo.get(moduleName) is not None:
        return
    moduleItem = {}
    moduleItem['name'] = moduleName
    moduleItem['rules'] = []
    moduleItem['export'] = []
    moduleItem['methods'] = {}
    moduleItem['internal'] = {}
    moduleItem['external'] = {}
    moduleItem['connect'] = []
    moduleItem['connDictionary'] = {}
    mInfo[moduleName] = moduleItem
    fileDesc = None
    for item in options.directory:
        try:
            fileDesc = open(item + '/' + moduleName + '.vh')
        except:
            continue
        break
    #print 'DD', fileDesc
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
                        moduleItem['methods'][tempName] = {}
                        moduleItem['methods'][tempName]['guard'] = inVector[2]
                        moduleItem['methods'][tempName]['read'] = []
                        moduleItem['methods'][tempName]['write'] = []
                        moduleItem['methods'][tempName]['invoke'] = []
                    elif inVector[0] == '//METARULES':
                        moduleItem['rules'] = inVector[1:]
                    elif inVector[0] == '//METAINTERNAL':
                        moduleItem['internal'][inVector[1]] = inVector[2]
                    elif inVector[0] == '//METAEXTERNAL':
                        moduleItem['external'][inVector[1]] = inVector[2]
                    elif inVector[0] == '//METACONNECT':
                        moduleItem['connect'].append(inVector[1:])
                    elif inVector[0] == '//METAINVOKE':
                        for vitem in inVector[2:]:
                            moduleItem['methods'][inVector[1]]['invoke'].append(splitRef(vitem))
                    elif inVector[0] == '//METAREAD':
                        for vitem in inVector[2:]:
                            moduleItem['methods'][inVector[1]]['read'].append(splitRef(vitem))
                    elif inVector[0] == '//METAWRITE':
                        for vitem in inVector[2:]:
                            moduleItem['methods'][inVector[1]]['write'].append(splitRef(vitem))
                    else:
                        print 'Unknown case', inVector
    if traceAll:
        print 'ALL', json.dumps(moduleItem, sort_keys=True, indent = 4)
    for key, value in moduleItem['internal'].iteritems():
        processFile(value)
    for key, value in moduleItem['external'].iteritems():
        processFile(value)
    for value in moduleItem['connect']:
        targetItem, targetIfc = splitName(value[0])
        sourceItem, sourceIfc = splitName(value[1])
        print 'connect', targetItem, targetIfc, sourceItem, sourceIfc
        if not moduleItem['connDictionary'].get(targetItem):
            moduleItem['connDictionary'][targetItem] = {}
        moduleItem['connDictionary'][targetItem][targetIfc] = [sourceItem, moduleItem['internal'][sourceItem], sourceIfc]
    #print 'moduleItem[connDictionary]', moduleItem['name'], json.dumps(moduleItem['connDictionary'], sort_keys=True, indent = 4)

def extractInterfaces(moduleName):
    moduleItem = mInfo[moduleName]
    for methodName in moduleItem['methods']:
        if not methodName in moduleItem['rules']:
            moduleItem['export'].append(methodName)

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
    #print 'parseExpression', string, ' -> ', retVal, ind
    return retVal

def prependList(prefix, aList):
    rList = []
    for item in aList:
        nitem = []
        for field in item:
            nitem.append(prependName(prefix, field))
        rList.append(nitem)
    return rList

def formatAccess(isWrite, aList):
    global accessList, secondPass
    ret = ''
    sep = ''
    for item in aList:
        if secondPass and item[1] not in accessList[1-isWrite]:
            continue
        if item[1] not in accessList[isWrite]:
            accessList[isWrite].append(item[1])
        ret += sep
        if item[0] != '':
            ret += item[0] + ':'
        ret += item[1]
        sep = ', '
    return ret

def getList(prefix, moduleName, methodName, readOrWrite, connDictionary):
    #print 'getlist', '"' + prefix + '"', moduleName, methodName, readOrWrite, connDictionary
    if connDictionary is None:
        connDictionary = {}
    moduleItem = mInfo[moduleName]
    methodItem = moduleItem['methods'][methodName]
    returnList = prependList(prefix, methodItem[readOrWrite])
    #tguard = expandGuard(moduleItem, prefix, methodItem['guard'])
    #methodItem['guard'] = tguard
    for iitem in methodItem['invoke']:
        #print 'IIII', iitem
        for item in iitem[1:]:
            refName = item.split('$')
            innerFileName = moduleItem['internal'].get(refName[0])
            if innerFileName:
                rList = getList(prefix + refName[0] + '$', innerFileName, '$'.join(refName[1:]), readOrWrite, moduleItem['connDictionary'])
            else:
                newItem = connDictionary.get(item)
                if not newItem:
                    print 'itemnotfound', item, 'dict', connDictionary.get(item)
                    continue
                else:
                    rList = getList(newItem[0] + '$', newItem[1], newItem[2], readOrWrite, {})
            for rItem in rList:
                #gVal = prependName(prefix + refName[0] + "$", gitem['guard'])
                #gVal = tguard
                #for ind in range(1, len(rItem)):
                    #rItem[ind] = prefix + refName[0] + "$" + rItem[ind]
                insertItem = True
                for item in returnList:
                    if item[0] == rItem[0] and item[1] == rItem[1]:
                        #print 'already in list', methodName, rItem
                        insertItem = False
                        break
                if insertItem:
                    returnList.append(rItem)
    if traceList and returnList != []:
        print 'getlistreturn', moduleName, methodName, readOrWrite, returnList
    return returnList

def dumpRules(prefix, moduleName, modDict):
    global totalList, accessList
    moduleItem = mInfo[moduleName]
    #print 'DUMPR', '"' + prefix + '"', moduleName, modDict
    if modDict is None:
        modDict = {}
    for ruleName in (moduleItem['rules'] + moduleItem['export']):
         if traceList:
             print 'RULE', '"' + prefix + '"', ruleName
         methodItem = moduleItem['methods'][ruleName]
         rList = getList(prefix, moduleName, ruleName, 'read', modDict)
         wList = getList(prefix, moduleName, ruleName, 'write', modDict)
         tGuard = expandGuard(moduleItem, prefix, methodItem['guard'])
         ruleList.append({'module': moduleName, 'name': ruleName, 'guard': tGuard, 'write': wList, 'read': rList})
    for key, value in moduleItem['internal'].iteritems():
        dumpRules(prefix + key + '$', value, modDict.get(key))
    for key, value in moduleItem['external'].iteritems():
        dumpRules(prefix + key + '$', value, modDict.get(key))

def formatRules():
    for item in ruleList:
         totalList[formatAccess(1, item['write']) + '/' + item['name']] = formatAccess(0, item['read']) + '\n        ' + item['guard']

if __name__=='__main__':
    options = argparser.parse_args()
    for moduleName in options.verilog:
        processFile(moduleName)
        extractInterfaces(moduleName)
    if options.output:
        print 'output'
        secondPass = False
        ruleList = []
        totalList = {}
        accessList = [[], []]
        for moduleName in options.verilog:
            dumpRules('', moduleName, mInfo[moduleName]['connDictionary'])
        formatRules()
        for key, value in sorted(totalList.iteritems()):
            print key + ': ' + value
        if removeUnref:
            secondPass = True
            ruleList = []
            totalList = {}
            print '\nACCESSLIST', accessList
            for moduleName in options.verilog:
                dumpRules('', moduleName, mInfo[moduleName]['connDictionary'])
            formatRules()
            for key, value in sorted(totalList.iteritems()):
                print key + ': ' + value

