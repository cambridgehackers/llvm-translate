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
argparser.add_argument('--directory', help='directory', default='')
argparser.add_argument('--output', help='linked top level', default='')
argparser.add_argument('verilog', help='Verilog files to parse', nargs='+')

verilogTemplate='''
module EchoVerilog(%(verilogArgs)s
 output RDY_messageSize_size, input[15:0] messageSize_size_methodNumber, output[15:0] messageSize_size
 );

 wire respond_rule_wire;
 %(userWires)s

 l_class_OC_Echo lEcho(%(userArgs)s
   .respond_rule__RDY(respond_rule_wire), .respond_rule__ENA(respond_rule_wire));

 mkEchoIndicationOutput myEchoIndicationOutput(%(userLinks)s
   .RDY_portalIfc_messageSize_size(RDY_messageSize_size), .portalIfc_messageSize_size_methodNumber(messageSize_size_methodNumber), .portalIfc_messageSize_size(messageSize_size));
endmodule  // mkEcho
'''

bsvTemplate='''
%(importFiles)s

interface %(name)s;
   interface %(request)s request;
   interface %(indication)sPortalOutput l%(indication)sOutput;
endinterface
interface %(name)sBVI;
   interface %(request)s request;
   %(exportInterface)s
endinterface

import "BVI" %(name)sVerilog =
module mk%(name)sBVI(%(name)sBVI);
    default_clock clk();
    default_reset rst();
    interface %(request)s request;
        %(methodList)s;
    endinterface
    interface PortalSize messageSize;
        method messageSize_size size(messageSize_size_methodNumber) ready(RDY_messageSize_size);
    endinterface
    %(interfaceBody)s
endmodule

(*synthesize*)
module mk%(name)s(%(name)s);
    let bvi <- mk%(name)sBVI;
    Vector#(1, PipeOut#(Bit#(SlaveDataBusWidth))) tmpInd;
    tmpInd[0] = bvi.indications;
    interface %(request)s request = bvi.request;
    interface %(indication)sPortalOutput l%(indication)sOutput;
        interface PortalSize messageSize = bvi.messageSize;
        interface Vector indications = tmpInd;
        interface PortalInterrupt intr = bvi.intr;
    endinterface
endmodule
'''

importFiles = ['ConnectalConfig', 'Portal', 'Pipe', 'Vector', 'EchoReq', 'EchoIndication']
exportInterface = {'indications': 'PipeOut#(Bit#(SlaveDataBusWidth))',
             'intr': 'PortalInterrupt#(SlaveDataBusWidth)',
             'messageSize': 'PortalSize',
            }
verilogActions = {'indications': ['indications_0', [['deq', []]]]}
verilogValues  = {'indications': ['indications_0', [['notEmpty', ''], ['first', '[31:0]']]],
                  'intr':        ['intr',          [['status', ''], ['channel', '[31:0]']]]}
userRequests =    {'request': [['say', [['v', '[31:0]']]]]}
userIndications = [['ifc_heard', [['v', '[31:0]']], 'ind$heard']]

verilogArgValue =     'output RDY_%(name)s, output %(adim)s%(name)s,'
verilogArgAction =    'output RDY_%(name)s, input EN_%(name)s,'
verilogArgActionArg = ' input%(adim)s %(name)s_%(aname)s,'
userArgAction =       '.%(uname)s__RDY(RDY_%(name)s), .%(uname)s__ENA(EN_%(name)s),'
userArgActionArg =    ' .%(uname)s%(paramSep)s%(aname)s(%(name)s_%(aname)s),'
userArgLinkArg =      ' .%(name)s_%(aname)s(%(name)s_%(aname)s),'
userArgWire =         'wire RDY_%(name)s, EN_%(name)s;'
userArgWireArg =      'wire %(adim)s%(name)s_%(aname)s;'
userArgLink =         '.RDY_%(name)s(RDY_%(name)s), .EN_%(name)s(EN_%(name)s),'
userArgReq =          'ready(RDY_%(name)s) enable(EN_%(name)s)'
userArgReqArg =       '%(uname)s(%(name)s_%(aname)s)'
verilogLink =         '.RDY_%(pname)s(RDY_%(name)s), .%(eprefix)s%(pname)s(%(eprefix)s%(name)s),'

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
    #print 'ALL', json.dumps(titem, sort_keys=True, indent = 4)
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
    #print 'parseExpression', string, ' -> ', retVal, ind
    return retVal

def addAction(item, pmap):
    for aitem in item[1]:
        pmap['aname'] = aitem[0]
        pmap['adim'] = aitem[1]
        vArgs.append(verilogArgActionArg % pmap)
        uArgs.append(userArgActionArg % pmap)
    vArgs.append(verilogArgAction % pmap)

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

        vArgs = ['input CLK,', 'input RST_N,']
        uArgs = ['.CLK(CLK),', '.nRST(RST_N),']
        uLinks = ['.CLK(CLK),', '.RST_N(RST_N),']
        uWires = []
        uReq = []
        eIfc = []
        eBody = []
        for key, value in verilogActions.iteritems():
            for item in value[1]:
                rname = value[0] + '_' + item[0]
                pmap = {'uname': item[0], 'name':rname, 'paramSep': '_', 'pname':'portalIfc_' + rname, 'eprefix': 'EN_'}
                addAction(item, pmap)
                uLinks.append(verilogLink % pmap)
        for key, value in userRequests.iteritems():
            for item in value:
                rname = key + '_' + item[0]
                pmap = {'uname': item[0], 'name':rname, 'paramSep': '_'}
                uArgs.append(userArgAction % pmap)
                addAction(item, pmap)
                uReq.append('method')
                for aitem in item[1]:
                    pmap['aname'] = aitem[0]
                    pmap['adim'] = aitem[1]
                    uReq.append(userArgReqArg % pmap)
                uReq.append(userArgReq % pmap)
        for key, value in verilogValues.iteritems():
            for item in value[1]:
                rname = value[0] + '_' + item[0]
                pmap = {'name':rname, 'adim': item[1], 'pname':'portalIfc_' + rname, 'eprefix': ''}
                vArgs.append(verilogArgValue % pmap)
                uLinks.append(verilogLink % pmap)
        for item in userIndications:
            pmap = {'name':item[0], 'uname': item[2], 'paramSep': '$'}
            for aitem in item[1]:
                pmap['aname'] = aitem[0]
                pmap['adim'] = aitem[1]
                uArgs.append(userArgActionArg % pmap)
                uWires.append(userArgWireArg % pmap)
                uLinks.append(userArgLinkArg % pmap)
            uWires.append(userArgWire % pmap)
            uLinks.append(userArgLink % pmap)
        for key, value in exportInterface.iteritems():
            eIfc.append('interface %s %s;' % (value, key))
            vVal = verilogValues.get(key)
            vAct = verilogActions.get(key)
            if vAct or vVal:
                eBody.append('interface %s %s;' % (value.split('#')[0], key))
                if vAct:
                   for item in vAct[1]:
                       pmap = {'iname': vAct[0], 'name': item[0]}
                       eBody.append('    method %(name)s() enable(EN_%(iname)s_%(name)s) ready(RDY_%(iname)s_%(name)s);' % pmap)
                if vVal:
                   for item in vVal[1]:
                       pmap = {'iname': vVal[0], 'name': item[0]}
                       eBody.append('    method %(iname)s_%(name)s %(name)s() ready(RDY_%(iname)s_%(name)s);' % pmap)
                eBody.append('endinterface')
        pmap = {'name': 'Echo', 'request': 'EchoRequest', 'indication': 'EchoIndication',
            'methodList': ' '.join(uReq),
            'importFiles': '\n'.join(['import %s::*;' % name for name in importFiles]),
            'verilogArgs': '\n'.join(vArgs),
            'userArgs': '\n'.join(uArgs),
            'userWires': '\n '.join(uWires),
            'userLinks': '\n   '.join(uLinks),
            'exportInterface': '\n   '.join(eIfc),
            'interfaceBody': '\n    '.join(eBody),
            }
        open(options.directory + '/' + options.output + '.bsv', 'w').write(bsvTemplate % pmap)
        open(options.directory + '/' + options.output + 'Verilog' + '.v', 'w').write(verilogTemplate % pmap)

