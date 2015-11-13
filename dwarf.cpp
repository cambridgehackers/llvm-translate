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
#include <stdio.h>
#include <cxxabi.h> // abi::__cxa_demangle
#include "llvm/IR/LLVMContext.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/DebugInfo.h"

using namespace llvm;

#include "declarations.h"

static int trace_meta;// = 1;
static int trace_mapt;// = 1;

static std::map<const Metadata *, int> metamap;
std::map<std::string, DICompositeType *> retainedTypes;
static CLASS_META class_data[MAX_CLASS_DEFS];
static std::map<MAPSEEN_TYPE, int, MAPSEENcomp> mapseen;
static int class_data_index;
static DITypeIdentifierMap TypeIdentifierMap;
static DebugInfoFinder Finder;
SmallDenseMap<const MDString *, const DIType *, 32> TypeRefs;

/*
 * Read in dwarf metadata
 */
static std::string getScope(Metadata *Node)
{
    std::string name = "";
    if (const DINamespace *nextitem = dyn_cast_or_null<DINamespace>(Node))
        name = getScope(nextitem->getRawScope()) + nextitem->getName().str() + "::";
    else if (const MDString *MDS = dyn_cast_or_null<MDString>(Node)) {
        int status;
        const char *demang = abi::__cxa_demangle(MDS->getString().str().c_str(), 0, 0, &status);
        if (demang) {
            const char *p;
            while((p = strstr(demang, " "))) {
                demang = p+1;
            }
            name = std::string(demang) + "::";
        }
    }
    //int ind = name.find("<");
    //if (ind >= 0)
        //name = name.substr(0, ind);
    return name;
}
/*
 * Lookup/usage of dwarf metadata
 */
void dump_class_data()
{
    CLASS_META *classp = class_data;
    for (int i = 0; i < class_data_index; i++) {
      printf("class %s node %p inherit %p; ", classp->name.c_str(), classp->node, classp->inherit);
      for (std::list<const Metadata *>::iterator MI = classp->memberl.begin(), ME = classp->memberl.end(); MI != ME; MI++) {
          if (const DIType *Ty = dyn_cast<DIType>(*MI)) {
              uint64_t off = Ty->getOffsetInBits()/8;
              const char *cp = Ty->getName().str().c_str();
              printf(" M[%s/%lld]", cp, (long long)off);
          }
          else if (const DISubprogram *SP = dyn_cast<DISubprogram>(*MI)) {
              uint64_t off = SP->getVirtualIndex();
              const char *cp = SP->getLinkageName().str().c_str();
              if (!strlen(cp))
                  cp = SP->getName().str().c_str();
              printf(" S[%s/%lld]", cp, (long long)off);
          }
      }
      printf("\n");
      classp++;
    }
}
CLASS_META *lookup_class(const char *cp)
{
    CLASS_META *classp = class_data;
    for (int i = 0; i < class_data_index; i++) {
      if (cp == classp->name)
          return classp;
      classp++;
    }
    return NULL;
}
int lookup_method(const char *classname, std::string methodname)
{
    if (trace_meta)
        printf("[%s:%d] class %s meth %s\n", __FUNCTION__, __LINE__, classname, methodname.c_str());
    CLASS_META *classp = lookup_class(classname);
    if (!classp)
        return -1;
    for (std::list<const Metadata *>::iterator MI = classp->memberl.begin(), ME = classp->memberl.end(); MI != ME; MI++) {
        const DISubprogram *SP = dyn_cast<DISubprogram>(*MI);
        if (SP && SP->getName().str() == methodname)
            return SP->getVirtualIndex();
    }
    return -1;
}
int lookup_field(const char *classname, std::string methodname)
{
    if (trace_meta)
        printf("[%s:%d] class %s field %s\n", __FUNCTION__, __LINE__, classname, methodname.c_str());
    CLASS_META *classp = lookup_class(classname);
    if (!classp)
        return -1;
    for (std::list<const Metadata *>::iterator MI = classp->memberl.begin(), ME = classp->memberl.end(); MI != ME; MI++) {
        const DIType *Ty = dyn_cast<DIType>(*MI);
        if (Ty->getTag() == dwarf::DW_TAG_member && Ty->getName().str() == methodname)
            return Ty->getOffsetInBits()/8;
    }
    return -1;
}
const Metadata *lookupMember(const StructType *STy, uint64_t ind, unsigned int tag)
{
static int errorCount;
    if (!STy)
        return NULL;
    if (!STy->isLiteral()) { // unnamed items
    std::string cname = STy->getName();
    CLASS_META *classp = lookup_class(cname.c_str());
    if (trace_meta)
        printf("%s: lookup class '%s' = %p\n", __FUNCTION__, cname.c_str(), classp);
    if (!classp) {
        printf("%s: can't find class '%s', will exit\n", __FUNCTION__, cname.c_str());
        if (errorCount++ > 20)
            exit(-1);
        return NULL;
    }
    if (classp->inherit) {
        //const DISubprogram *SP = dyn_cast<DISubprogram>(classp->inherit);
        if (!ind--)
            return classp->inherit;
    }
    for (std::list<const Metadata *>::iterator MI = classp->memberl.begin(), ME = classp->memberl.end(); MI != ME; MI++) {
        unsigned int itemTag = -1;
        if (const DIType *Ty = dyn_cast<DIType>(*MI)) {
            itemTag = Ty->getTag();
            if (trace_meta)
                printf("[%s:%d] tag %x name %s\n", __FUNCTION__, __LINE__, itemTag, CBEMangle(Ty->getName().str()).c_str());
        }
        else if (const DISubprogram *SP = dyn_cast<DISubprogram>(*MI)) {
            itemTag = SP->getTag();
            if (trace_meta)
                printf("[%s:%d] tag %x name %s\n", __FUNCTION__, __LINE__, itemTag, CBEMangle(SP->getName().str()).c_str());
        }
        if (itemTag == tag)
            if (!ind--)
                return *MI;
    }
    }
    return NULL;
}
std::string fieldName(const StructType *STy, uint64_t ind)
{
    char temp[MAX_CHAR_BUFFER];
    const Metadata *tptr = lookupMember(STy, ind, dwarf::DW_TAG_member);
    if (tptr) {
        const DIType *Ty = dyn_cast<DIType>(tptr);
        return CBEMangle(Ty->getName().str());
    }
    sprintf(temp, "field%d", (int)ind);
    return temp;
}
const DISubprogram *lookupMethod(const StructType *STy, uint64_t ind)
{
    return dyn_cast_or_null<DISubprogram>(lookupMember(STy, ind, dwarf::DW_TAG_subprogram));
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

const Metadata *fetchType(const Metadata *arg)
{
    if (auto *S = dyn_cast_or_null<MDString>(arg)) {
      // Don't error on missing types (checked elsewhere).
      const DIType *DT = TypeRefs.lookup(S);
      if (!DT) {
          printf("[%s:%d] lookup named type failed\n", __FUNCTION__, __LINE__);
          exit(-1);
      }
      arg = DT;
      if (trace_mapt)
          printf("        [%s:%d] replacedmeta S %p %s = %p\n", __FUNCTION__, __LINE__, S, S->getString().str().c_str(), arg);
    }
    return arg;
}

void mapDwarfType(int derived, const Metadata *aMeta, char *addr, int aoffset, std::string aname)
{
    aMeta = fetchType(aMeta);
    const DIType *Ty = dyn_cast<DIType>(aMeta);
    if (!Ty) {
        printf("[%s:%d] mapDwarfType cast failed\n", __FUNCTION__, __LINE__);
        exit(-1);
    }
    int off = Ty->getOffsetInBits()/8;
    int offset = aoffset + off;
    char *addr_target = *(char **)(addr + offset);
    std::string incomingAddr = mapAddress(addr);
    if (trace_mapt)
        printf("%s+%d+%d N %p A %p aname %s D %d\n", incomingAddr.c_str(),
            offset - off, off, aMeta, addr_target, aname.c_str(), derived);
    if (validateAddress(5000, addr) || validateAddress(5001, (addr + offset)))
        exit(1);
    //std::string vname = getVtableName(addr_target);
    std::string fname = aname;
    if (auto *DT = dyn_cast<DIDerivedTypeBase>(aMeta)) {
        const Metadata *node = fetchType(DT->getRawBaseType());
        if (DT->getTag() == dwarf::DW_TAG_member || DT->getTag() == dwarf::DW_TAG_inheritance) {
            if (const DIDerivedTypeBase *DI = dyn_cast_or_null<DIDerivedTypeBase>(node)) {
                DT = DI;
                node = fetchType(DT->getRawBaseType());
            }
        }
        std::string tname;
        if (auto *DR = dyn_cast_or_null<DIDerivedType>(node))
            tname = DR->getName();
        /* Handle pointer types */
        if (DT->getTag() == dwarf::DW_TAG_pointer_type && tname != "__vtbl_ptr_type"
         && addr_target && !mapseen[MAPSEEN_TYPE{addr_target, node}]) {  // process item, if not seen before
            if (trace_mapt)
                printf("    pointer ;");
            mapseen[MAPSEEN_TYPE{addr_target, node}] = 1;
            mapDwarfType(100+derived, node, addr_target, 0, fname);
            if (validateAddress(5010, addr_target))
                exit(1);
            return;
        }
    }
    std::string mstr = setMapAddress(addr + offset, fname); // setup mapping!
    if (trace_mapt) {
        printf("%p @[%s]=val %s der %d\n", addr, mstr.c_str(), mapAddress(addr_target).c_str(), derived);
        memdumpl((unsigned char *)addr + offset, 64, mapAddress(addr+offset).c_str());
    }
    if (auto *CTy = dyn_cast<DICompositeType>(aMeta)) {
        /* Handle class types */
        if (trace_mapt)
            printf("   CLASSSSS name %s id %s N %p A %p\n", CTy->getName().str().c_str(), CTy->getIdentifier().str().c_str(), aMeta, addr+offset);
        DINodeArray Elements = CTy->getElements();
        for (unsigned k = 0, N = Elements.size(); k < N; ++k)
            if (DIType *Ty = dyn_cast<DIType>(Elements[k])) {
                if (Ty->isStaticMember())  // don't recurse on static member elements
                    continue;
                std::string mname = Ty->getName();
                if (trace_mapt)
                    printf("   %p+%d+%ld %s ", addr, offset, Ty->getOffsetInBits()/8, mname.c_str());
                int tag = Ty->getTag();
                const Metadata *node = fetchType(Ty);
                if (tag == dwarf::DW_TAG_member)
                    mapDwarfType(1000+derived, node, addr, offset, fname + "$$" + mname);
                else if (tag == dwarf::DW_TAG_inheritance)
                    mapDwarfType(10000+derived, node, addr, aoffset, fname);
            }
    }
}

void process_metadata(Module *Mod)
{
    Finder.processModule(*Mod);
    if (trace_meta)
    printf("[%s:%d]compile_unit_count %d global_variable_count %d subprogram_count %d type_count %d scope_count %d\n", __FUNCTION__, __LINE__,
    Finder.compile_unit_count(), Finder.global_variable_count(), Finder.subprogram_count(), Finder.type_count(), Finder.scope_count());

    for (DICompileUnit *CU : Finder.compile_units()) {
        if (trace_meta)
            printf("Compile unit: (%d)\n", CU->getSourceLanguage());
        // Visit all the compile units again to map the type references.
        if (auto Ts = cast<DICompileUnit>(CU)->getRetainedTypes())
            for (DIType *Op : Ts)
              if (auto *T = dyn_cast<DICompositeType>(Op))
                if (auto *S = T->getRawIdentifier()) {
                    TypeRefs.insert(std::make_pair(S, T));
                    if (trace_meta)
                        printf("[%s:%d] insertTypeRef S %p %s T %p\n", __FUNCTION__, __LINE__, S, S->getString().str().c_str(), T);
                    int status;
                    const char *demang = abi::__cxa_demangle(S->getString().str().c_str(), 0, 0, &status);
                    if (strncmp(demang, "typeinfo name for ", 18)) {
                        printf("[%s:%d] NOT TYPEINFO '%s'\n", __FUNCTION__, __LINE__, demang);
                        exit(-1);
                    }
                    std::string name = "l_class." + std::string(demang+18);
printf("[%s:%d] RETAIN %s\n", __FUNCTION__, __LINE__, name.c_str());
                    retainedTypes[CBEMangle(name)] = T;
                    int ind = name.find("<");
                    if (ind >= 0) { /* also insert the class w/o template parameters */
                        name = name.substr(0, ind);
printf("[%s:%d] RETAIN %s\n", __FUNCTION__, __LINE__, name.c_str());
                        retainedTypes[CBEMangle(name)] = T;
                    }
                }
    }

    if (trace_meta) {
    for (DISubprogram *S : Finder.subprograms()) {
        printf("Subprogram: %s", S->getName().str().c_str());
        if (!S->getLinkageName().empty())
            printf(" ('%s')", S->getLinkageName().str().c_str());
        printf("\n");
    }
    for (const DIGlobalVariable *GV : Finder.global_variables()) {
        printf("Global variable: %s", GV->getName().str().c_str());
        if (!GV->getLinkageName().empty())
            printf(" ('%s')", GV->getLinkageName().str().c_str());
        printf("\n");
    }
    }

    for (const DIType *T : Finder.types()) {
        int tag = T->getTag();
        if (tag != dwarf::DW_TAG_class_type)
            continue;
        if (trace_meta) {
        if (!T->getName().empty())
            printf("Type: Name %s", T->getName().str().c_str());
        else {
            //printf("Type:");
            continue;
        }
        if (auto *BT = dyn_cast<DIBasicType>(T)) {
            printf(" Basic %s", dwarf::AttributeEncodingString(BT->getEncoding()));
        } else {
            printf(" Tag %s", dwarf::TagString(tag));
        }
        if (auto *CT = dyn_cast<DICompositeType>(T)) {
            printf(" (identifier: '%s)", CT->getRawIdentifier()->getString().str().c_str());
        }
        printf("\n");
        }
        std::string name = T->getName().str();
        const Metadata *Node = dyn_cast<Metadata>(T);
        if (!metamap[Node]) {
            metamap[Node] = 1;
            if (tag == dwarf::DW_TAG_class_type) {
                CLASS_META *classp = &class_data[class_data_index++];
                classp->node = Node;
                Metadata *sc = T->getRawScope();
                classp->name = "class." + getScope(sc) + name;
                if (trace_meta)
printf("[%s:%d] ADDCLASS name %s tag %s Node %p cname %s scope %p\n", __FUNCTION__, __LINE__, name.c_str(), dwarf::TagString(tag), Node, classp->name.c_str(), sc);
                int ind = name.find("<");
                if (ind >= 0) { /* also insert the class w/o template parameters */
                    classp = &class_data[class_data_index++];
                    *classp = *(classp-1);
                    name = name.substr(0, ind);
                    classp->name = "class." + getScope(T->getRawScope()) + name;
                }
                if (const DICompositeType *CTy = dyn_cast<DICompositeType>(T)) {
                    DINodeArray Elements = CTy->getElements();
                    for (unsigned k = 0, N = Elements.size(); k < N; ++k) {
                        if (DIType *Ty = dyn_cast<DIType>(Elements[k])) {
                            int tag = Ty->getTag();
                            const Metadata *Node = Ty;
//printf("[%s:%d] name %s NODE %p\n", __FUNCTION__, __LINE__, name.c_str(), Node);
                            if (tag == dwarf::DW_TAG_member)
                                classp->memberl.push_back(Node);
                            else if (tag == dwarf::DW_TAG_inheritance) {
                                classp->inherit = Node;
                            }
else
printf("[%s:%d] NOTMEMBER tag %s name %s\n", __FUNCTION__, __LINE__, dwarf::TagString(tag), name.c_str());
                        }
                        else if (DISubprogram *SP = dyn_cast<DISubprogram>(Elements[k])) {
                            const Metadata *Node = SP;
//printf("[%s:%d] name %s NODE %p\n", __FUNCTION__, __LINE__, name.c_str(), Node);
                            if (classp)
                                classp->memberl.push_back(Node);
                        }
                        //dumpType(Ty, classp);
                    }
                    //auto *DT = dyn_cast<DIDerivedTypeBase>(CTy);
                    //dumpTref(getNode(DT->getBaseType()), classp);
                }
            }
        }

    }
}
