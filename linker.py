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
import argparse, copy, json, re, os, sys, shutil, string

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

def expandGuard(mitem, name, inList):
    retList = []
    for string in inList:
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
        retList.append(retVal)
    return retList

def splitName(value):
    valsplit = value.split('$')
    return valsplit[0], '$'.join(valsplit[1:])

def splitRef(value):
    temp = value.split(':')
    if len(temp) == 1:
        temp.insert(0, '')
    return temp

def checkMethod(dictBase, name):
    if dictBase['methods'].get(name) is None:
        dictBase['methods'][name] = {'guard': '', 'read': [], 'write': [], 'invoke': [], 'before': []}

def splitGuard(canonVec, source):
    inStr = ''
    indent = 0
    for inchar in source:
        if inchar == '(':
            indent += 1
        elif inchar == ')':
            indent -= 1
        elif inchar == '&' and indent == 0:
            splitGuard(canonVec, inStr.strip())
            inStr = ''
            continue
        inStr += inchar
    if inStr != '':
        if inStr[0] == '(' and inStr[-1] == ')':
            splitGuard(canonVec, inStr[1:-1])
        else:
            canonVec.append(inStr.strip())

def processFile(moduleName):
    print 'processFile:', moduleName
    if mInfo.get(moduleName) is not None:
        return
    moduleItem = {'name': moduleName, 'rules': [], 'export': [], 'methods': {},
        'internal': {}, 'external': {}, 'connect': [], 'connDictionary': {}, 'exclusive': []}
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
                    metaIndex = {'//METAINVOKE': 'invoke', '//METAREAD': 'read', '//METAWRITE': 'write',
                         '//METABEFORE': 'before'}.get(inVector[0])
                    #print 'MM', inLine, inVector, metaIndex
                    if inVector[0] == '//METAGUARD':
                        checkMethod(moduleItem, inVector[1])
                        canonVec = []
                        splitGuard(canonVec, inVector[2])
                        print 'CANON', inVector[2], canonVec
                        moduleItem['methods'][inVector[1]]['guard'] = canonVec
                    elif inVector[0] == '//METARULES':
                        moduleItem['rules'] = inVector[1:]
                    elif inVector[0] == '//METAINTERNAL':
                        moduleItem['internal'][inVector[1]] = inVector[2]
                    elif inVector[0] == '//METAEXTERNAL':
                        moduleItem['external'][inVector[1]] = inVector[2]
                    elif inVector[0] == '//METACONNECT':
                        moduleItem['connect'].append(inVector[1:])
                    elif inVector[0] == '//METAEXCLUSIVE':
                        moduleItem['exclusive'].append(inVector[1:])
                    elif metaIndex:
                        checkMethod(moduleItem, inVector[1])
                        for vitem in inVector[2:]:
                            moduleItem['methods'][inVector[1]][metaIndex].append(splitRef(vitem))
                    else:
                        print 'Unknown case', inVector
                        sys.exit(1)
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
    for item in moduleItem['exclusive']:
        print 'EXCLUSIVE', item
        for element in item:
            print '        ', element + ": " + ' & '.join(moduleItem['methods'][element]['guard'])
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

def lookupInvoke(prefix, moduleItem, item, connDictionary):
    refName = item.split('$')
    innerFileName = moduleItem['internal'].get(refName[0])
    if innerFileName:
        thisRef = prefix + refName[0]
        thisMeth =  '$'.join(refName[1:])
        thisDict = moduleItem['connDictionary']
    else:
        newItem = connDictionary.get(item)
        if not newItem:
            print 'itemnotfound', item, 'dict', connDictionary.get(item)
            return None, None, None, None
        thisRef = newItem[0]
        innerFileName = newItem[1]
        thisMeth = newItem[2]
        thisDict = {}
    return thisRef, innerFileName, thisMeth, thisDict

def getList(prefix, moduleName, methodName, readOrWrite, connDictionary):
    #print 'getlist', '"' + prefix + '"', moduleName, methodName, readOrWrite, connDictionary
    if connDictionary is None:
        connDictionary = {}
    moduleItem = mInfo[moduleName]
    methodItem = moduleItem['methods'][methodName]
    returnList = prependList(prefix, methodItem[readOrWrite])
    for iitem in methodItem['invoke']:
        #print 'IIII', iitem
        for item in iitem[1:]:
            thisRef, innerFileName, thisMeth, thisDict = lookupInvoke(prefix, moduleItem, item, connDictionary)
            if innerFileName is None:
                continue
            rList = getList(thisRef + '$', innerFileName, thisMeth, readOrWrite, thisDict)
            for rItem in rList:
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
         ruleList.append({'module': moduleName, 'name': prefix + ruleName, 'connDictionary': modDict,
             'guard': expandGuard(moduleItem, prefix, methodItem['guard']),
             'write': getList(prefix, moduleName, ruleName, 'write', modDict),
             'read': getList(prefix, moduleName, ruleName, 'read', modDict),
             'invoke': getList(prefix, moduleName, ruleName, 'invoke', modDict)})
    for key, value in moduleItem['internal'].iteritems():
        dumpRules(prefix + key + '$', value, modDict.get(key))
    for key, value in moduleItem['external'].iteritems():
        dumpRules(prefix + key + '$', value, modDict.get(key))
    for eitem in moduleItem['exclusive']:
        exclusiveList.append([ prependName(prefix, field) for field in eitem])

def formatRules():
    for item in ruleList:
         totalList[formatAccess(1, item['write']) + '/' + item['name']] = formatAccess(0, item['read']) + '\n        ' + ' & '.join(item['guard'])

def intersect(left, right):
    for litem in left:
        if litem in right:
            return True
    return False

def dumpDep(schedList, wIndex, targetItem):
    wItem = schedList[wIndex]
    #print 'DD', wItem, wItem['read']
    dumpString = ''
    foundTarget = False
    for item in wItem['read']:
        for sIndex in range(wIndex + 1, len(schedList)):
            sItem = schedList[sIndex]
            if item in sItem['write']:
                #print 'write', item[1], sItem['name']
                ret = dumpDep(schedList, sIndex, targetItem)
                if ret != '':
                    dumpString += ' reads[' + item[1] + ']: ' + sItem['name'] + ret
        if targetItem == item:
            return ' ' + dumpString
    return dumpString

if __name__=='__main__':
    options = argparser.parse_args()
    for moduleName in options.verilog:
        processFile(moduleName)
        extractInterfaces(moduleName)
    if options.output:
        print 'output'
        secondPass = False
        ruleList = []
        exclusiveList = []
        totalList = {}
        accessList = [[], []]
        for moduleName in options.verilog:
            dumpRules('', moduleName, mInfo[moduleName]['connDictionary'])
        formatRules()
        for key, value in sorted(totalList.iteritems()):
            print key + ': ' + value
        #print 'RR', json.dumps(ruleList, sort_keys=True, indent = 4)
        schedList = []
        for ritem in ruleList:
            sIndex = 0
            while sIndex < len(schedList):
                if intersect(schedList[sIndex]['write'], ritem['read']):
                    # our rule read something that was written at this stage, insert before
                    break
                sIndex += 1
            schedList.insert(sIndex, ritem)
        print 'SCHED'
        for sitem in schedList:
            print '    ', sitem['name'], ', '.join([foo[1] for foo in sitem['write']]), '    :', ', '.join([foo[1] for foo in sitem['read']])
            for iitem in sitem['invoke']:
                for item in iitem[1:]:
                    moduleItem = mInfo[sitem['module']]
                    thisRef, innerFileName, thisMeth, thisDict = lookupInvoke('', moduleItem, item, sitem['connDictionary'])
                    befList = []
                    if innerFileName is not None:
                        befList = prependList(thisRef + '$', mInfo[innerFileName]['methods'][thisMeth]['before'])
                    #print '        INV:', iitem, thisRef, innerFileName, thisMeth, befList
                    print '        INV:', iitem, befList, sitem['module'], sitem['connDictionary']
            #if sitem['before'] != []:
                #print 'BEFORE', moduleName, methodName, methodItem['before']
        for sIndex in range(len(schedList)):
            sitem = schedList[sIndex]
            for item in sitem['read']:
                for wIndex in range(0, sIndex-1):
                    witem = schedList[wIndex]
                    if item in witem['write']:
                        print 'error', item, 'read: ' + sitem['name'] + ', written: ' + witem['name'] + dumpDep(schedList, wIndex, item)
        print 'EXCL', exclusiveList
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

