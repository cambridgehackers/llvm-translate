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
import argparse, copy, json, re, os, sys, shutil, string, types

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
  ([0-9A-Za-z_][$A-Za-z0-9_]*) |   # identifiers, numbers
  "((?:[^"\n\\]|\\.)*)" |      # regular string literal
  ([(]) |                      # lparen
  ([)]) |                      # rparen
  (<<|>>|!=?|==?|\|\|?|[][{}<>,;:*+-/^&%]) | # punctuation
  (.)                          # an error!
''', re.DOTALL | re.VERBOSE)

def guardToString(source):
    ret = ''
    if type(source) == str:
        return source
    for item in source:
        if type(item) == str:
            ret += ' ' + item
        elif type(item) is types.ListType:
            ret += ' (' + guardToString(item) + ')'
    return ret

def optGuard(item):
    if len(item) == 3 and item[1] == "!=" and item[2] == "0":
        return optGuard(item[0])
    if (len(item) == 3 and item[1] == "^" and item[2] == "1") \
     or (len(item) == 3 and item[1] == "==" and item[2] == "0"):
        return [ "!", item[0]]
    if len(item) == 3 and item[1] == "&" and len(item[0]) == 3 and item[0][1] == "&":
        return item[0] + item[1:]
    if len(item) == 3 and item[1] == "&" and len(item[2]) == 3 and item[2][1] == "&":
        return item[:1] + item[2]
    if len(item) == 1:
        return optGuard(item[0])
    return item

def splitGuard(string):
    #print 'splitg', string
    expr = [[]]
    for match in re.finditer(SCANNER, string):
        for ind in range(2, 7):
            tfield = match.group(ind)
            if tfield is None:
                continue
            #print 'regex', ind, tfield, len(expr)
            if ind == 2:    # identifiers
                expr[-1].append(tfield)
            elif ind == 3:  # string
                expr[-1].append(tfield)
            elif ind == 4:  # lparen
                expr.append([])
            elif ind == 5:  # rparen
                item = optGuard(expr.pop())
                expr[-1].append(item)
            elif ind == 6:  # operator
                expr[-1].append(tfield)
            elif ind == 7:  # error
                print 'Guard parse error', tfield
                sys.exit(1)
    expr = optGuard(expr)
    #print 'splitgover', json.dumps(expr, sort_keys=True, indent = 4)
    return expr

def prependInternal(name, argList):
    retList = []
    if type(argList) == str:
        if argList[0].isalpha():
            argList = name + argList
        return argList
    for tfield in argList:
        if type(tfield) == str:
            if tfield[0].isalpha():
                tfield = name + tfield
        elif type(tfield) == types.ListType:
            tfield = prependInternal(name, tfield)
        retList.append(tfield)
    return retList

def expandGItem(mitem, name, tfield):
    if not tfield.endswith('__RDY'):
        tfield = name + tfield
    else:
        tsep = tfield.split('$')
        item = mitem['internal'].get(tsep[0])
        if item:
            rmitem = mInfo[item]
            methodItem = rmitem['methods']['$'.join(tsep[1:])[:-5]]
            tfield = expandGuard(rmitem, tsep[0] + '$', methodItem['guard'])
        else:
            tfield = name + tfield
    return tfield

def expandGuard(mitem, name, guardList):
    retList = []
    if type(guardList) == str:
        if guardList[0].isalpha():
            return expandGItem(mitem, name, guardList)
    for tfield in guardList:
        if type(tfield) == str:
            if tfield[0].isalpha():
                tfield = expandGItem(mitem, name, tfield)
        elif type(tfield) == types.ListType:
            tfield = expandGuard(mitem, name, tfield)
        retList.append(tfield)
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

def disjointCondition(element, cond):
    #print 'DDDDD', element, cond
    for eIndex in range(0, len(element)):
        eitem = element[eIndex]
        if eitem == '!' or eitem == '&':
            continue
        if eitem == '|':
            break
        ePrev = ''
        if eIndex > 0:
            ePrev = element[eIndex - 1]
        if type(eitem) != str and len(eitem) == 2 and eitem[0] == '!':
            ePrev = '!'
            eitem = eitem[1]
        for cIndex in range(0, len(cond)):
            citem = cond[cIndex]
            if citem == '!' or citem == '&':
                continue
            cPrev = ''
            if cIndex > 0:
                cPrev = cond[cIndex - 1]
            if type(citem) != str and len(citem) == 2 and citem[0] == '!':
                cPrev = '!'
                citem = citem[1]
            if citem != eitem:
                continue
            if (cPrev == '!' and ePrev != '!') or (cPrev != '!' and ePrev == '!'):
                #print 'disjoint', element, cond, citem
                return True
    return False

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
                        canonVec = splitGuard(inVector[2])
                        #print 'CANON', canonVec
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
                            temp = splitRef(vitem)
                            temp[0] = splitGuard(temp[0])
                            moduleItem['methods'][inVector[1]][metaIndex].append(temp)
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
    newExcl = []
    for item in moduleItem['exclusive']:
        newItem = []
        removedList = []
        for element in item:
            disjointFlag = True
            for cond in item:
                if element != cond and cond not in removedList and \
                   not disjointCondition(moduleItem['methods'][element]['guard'], moduleItem['methods'][cond]['guard']):
                    disjointFlag = False
            if disjointFlag:
                #print 'add to removedList', element
                removedList.append(element)
            else:
                #print 'add to newItem', element
                newItem.append(element)
        if newItem != []:
            newExcl.append(newItem)
    moduleItem['exclusive'] = newExcl
    for item in moduleItem['exclusive']:
        print 'EXCLUSIVE', item
        for element in item:
            print '        ', element + ": " + guardToString(moduleItem['methods'][element]['guard'])
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
            nitem.append(prependInternal(prefix, field))
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
        condStr = guardToString(item[0])
        if condStr != '':
            ret += condStr + ':'
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
        exclusiveList.append([ prefix + field for field in eitem])

def formatRules():
    for item in ruleList:
         totalList[formatAccess(1, item['write']) + '/' + item['name']] = formatAccess(0, item['read']) + '\n        ' + guardToString(item['guard'])

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

