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
#include "llvm/DebugInfo.h"

using namespace llvm;

#include "declarations.h"

static int trace_meta;// = 1;

static std::map<const MDNode *, int> metamap;
static CLASS_META class_data[MAX_CLASS_DEFS];
static int class_data_index;

/*
 * Read in dwarf metadata
 */
const MDNode *getNode(const Value *val)
{
    if (val)
        return dyn_cast<MDNode>(val);
    return NULL;
}

static std::string getScope(const Value *val)
{
    const MDNode *Node = getNode(val);
    if (!Node)
        return "";
    DIType nextitem(Node);
    std::string name = getScope(nextitem.getContext()) + nextitem.getName().str();
    //int ind = name.find("<");
    //if (ind >= 0)
        //name = name.substr(0, ind);
    return name + "::";
}
void dumpType(DIType litem, CLASS_META *classp);
void dumpTref(const MDNode *Node, CLASS_META *classp)
{
    if (!Node)
        return;
    DIType nextitem(Node);
    int tag = nextitem.getTag();
    std::string name = nextitem.getName().str();
    std::map<const MDNode *, int>::iterator FI = metamap.find(Node);
    if (FI == metamap.end()) {
        metamap[Node] = 1;
        if (tag == dwarf::DW_TAG_class_type) {
            classp = &class_data[class_data_index++];
            classp->node = Node;
            classp->name = strdup(("class." + getScope(nextitem.getContext()) + name).c_str());
            int ind = name.find("<");
            if (ind >= 0) { /* also insert the class w/o template parameters */
                classp = &class_data[class_data_index++];
                *classp = *(classp-1);
                name = name.substr(0, ind);
                classp->name = strdup(("class." + getScope(nextitem.getContext()) + name).c_str());
            }
        }
        dumpType(nextitem, classp);
    }
}

void dumpType(DIType litem, CLASS_META *classp)
{
    int tag = litem.getTag();
    if (!tag)     // Ignore elements with tag of 0
        return;
    if (tag == dwarf::DW_TAG_pointer_type || tag == dwarf::DW_TAG_inheritance) {
        DICompositeType CTy(litem);
        const MDNode *Node = getNode(CTy.getTypeDerivedFrom());
        if (tag == dwarf::DW_TAG_inheritance && Node && classp) {
            if(classp->inherit) {
                printf("[%s:%d]\n", __FUNCTION__, __LINE__);
                exit(1);
            }
            classp->inherit = Node;
        }
        dumpTref(Node, classp);
        return;
    }
    if (trace_meta)
        printf("tag %s name %s off %3ld size %3ld",
            dwarf::TagString(tag), litem.getName().str().c_str(),
            (long)litem.getOffsetInBits()/8, (long)litem.getSizeInBits()/8);
    if (litem.getTag() == dwarf::DW_TAG_subprogram) {
        DISubprogram sub(litem);
        if (trace_meta)
            printf(" link %s", sub.getLinkageName().str().c_str());
    }
    if (trace_meta)
        printf("\n");
    if (litem.isCompositeType()) {
        DICompositeType CTy(litem);
        DIArray Elements = CTy.getTypeArray();
        if (tag != dwarf::DW_TAG_subroutine_type) {
            dumpTref(getNode(CTy.getTypeDerivedFrom()), classp);
            dumpTref(getNode(CTy.getContainingType()), classp);
        }
        for (unsigned k = 0, N = Elements.getNumElements(); k < N; ++k) {
            DIType Ty(Elements.getElement(k));
            int tag = Ty.getTag();
            if (tag == dwarf::DW_TAG_member || tag == dwarf::DW_TAG_subprogram) {
                const MDNode *Node = Ty;
                if (classp)
                    classp->memberl.push_back(Node);
            }
            dumpType(Ty, classp);
        }
    }
}

void process_metadata(NamedMDNode *CU_Nodes)
{
    for (unsigned i = 0, e = CU_Nodes->getNumOperands(); i != e; ++i) {
      DICompileUnit CU(CU_Nodes->getOperand(i));
      printf("\n%s: compileunit %d:%s %s\n", __FUNCTION__, CU.getLanguage(),
           // from DIScope:
           CU.getDirectory().str().c_str(), CU.getFilename().str().c_str());
      DIArray SPs = CU.getSubprograms();
      for (unsigned i = 0, e = SPs.getNumElements(); i != e; ++i) {
        DISubprogram sub(SPs.getElement(i));
            if (trace_meta)
            printf("Subprogram: %s %s %d %d %d %d\n", sub.getName().str().c_str(),
                sub.getLinkageName().str().c_str(), sub.getVirtuality(),
                sub.getVirtualIndex(), sub.getFlags(), sub.getScopeLineNumber());
            dumpTref(getNode(sub.getContainingType()), NULL);
        DIArray tparam(sub.getTemplateParams());
        if (tparam.getNumElements() > 0) {
            if (trace_meta) {
                printf("tparam: ");
                for (unsigned j = 0, je = tparam.getNumElements(); j != je; j++) {
                   DIDescriptor ee(tparam.getElement(j));
                   printf("[%s:%d] %d/%d\n", __FUNCTION__, __LINE__, j, je);
                   ee.dump();
                }
            }
        }
        dumpType(DICompositeType(sub.getType()), NULL);
      }
      DIArray EnumTypes = CU.getEnumTypes();
      for (unsigned i = 0, e = EnumTypes.getNumElements(); i != e; ++i) {
        if (trace_meta)
        printf("[%s:%d]enumtypes\n", __FUNCTION__, __LINE__);
        dumpType(DIType(EnumTypes.getElement(i)), NULL);
      }
      DIArray RetainedTypes = CU.getRetainedTypes();
      for (unsigned i = 0, e = RetainedTypes.getNumElements(); i != e; ++i) {
        if (trace_meta)
        printf("[%s:%d]retainedtypes\n", __FUNCTION__, __LINE__);
        dumpType(DIType(RetainedTypes.getElement(i)), NULL);
      }
      DIArray Imports = CU.getImportedEntities();
      for (unsigned i = 0, e = Imports.getNumElements(); i != e; ++i) {
        DIImportedEntity Import = DIImportedEntity(Imports.getElement(i));
        DIDescriptor Entity = Import.getEntity();
        if (Entity.isType()) {
          if (trace_meta)
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          dumpType(DIType(Entity), NULL);
        }
        else if (Entity.isSubprogram()) {
          if (trace_meta) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          DISubprogram(Entity)->dump();
          }
        }
        else if (Entity.isNameSpace()) {
          //DINameSpace(Entity).getContext()->dump();
        }
        else
          printf("[%s:%d] entity not type/subprog/namespace\n", __FUNCTION__, __LINE__);
      }
      DIArray GVs = CU.getGlobalVariables();
      for (unsigned i = 0, e = GVs.getNumElements(); i != e; ++i) {
        DIGlobalVariable DIG(GVs.getElement(i));
        const GlobalVariable *gv = DIG.getGlobal();
        //const Constant *con = DIG.getConstant();
        const Value *val = dyn_cast<Value>(gv);
        std::string cp = DIG.getLinkageName().str();
        if (!cp.length())
            cp = DIG.getName().str();
        if (trace_meta)
            printf("%s: globalvar: %s GlobalVariable %p type %d\n", __FUNCTION__, cp.c_str(), gv, val->getType()->getTypeID());
      }
    }
}

/*
 * Lookup/usage of dwarf metadata
 */
void dump_class_data()
{
    CLASS_META *classp = class_data;
    for (int i = 0; i < class_data_index; i++) {
      printf("class %s node %p inherit %p; ", classp->name, classp->node, classp->inherit);
      for (std::list<const MDNode *>::iterator MI = classp->memberl.begin(), ME = classp->memberl.end(); MI != ME; MI++) {
          DIType Ty(*MI);
          DISubprogram CTy(*MI);
          uint64_t off = Ty.getOffsetInBits()/8;
          const char *cp = CTy.getLinkageName().str().c_str();
          if (Ty.getTag() != dwarf::DW_TAG_subprogram || !strlen(cp))
              cp = Ty.getName().str().c_str();
          printf(" %s/%lld", cp, (long long)off);
      }
      printf("\n");
      classp++;
    }
}
CLASS_META *lookup_class(const char *cp)
{
    CLASS_META *classp = class_data;
    for (int i = 0; i < class_data_index; i++) {
      if (!strcmp(cp, classp->name))
          return classp;
      classp++;
    }
    return NULL;
}
const MDNode *lookup_class_member(const char *cp, uint64_t Total)
{
    CLASS_META *classp = lookup_class(cp);
    if (!classp)
        return NULL;
    for (std::list<const MDNode *>::iterator MI = classp->memberl.begin(), ME = classp->memberl.end(); MI != ME; MI++) {
        DIType Ty(*MI);
        if (Ty.getOffsetInBits()/8 == Total)
            return *MI;
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
    for (std::list<const MDNode *>::iterator MI = classp->memberl.begin(), ME = classp->memberl.end(); MI != ME; MI++) {
        DISubprogram Ty(*MI);
        if (Ty.getTag() == dwarf::DW_TAG_subprogram && Ty.getName().str() == methodname)
            return Ty.getVirtualIndex();
    }
    return -1;
}
int lookup_field(const char *classname, std::string methodname)
{
    CLASS_META *classp = lookup_class(classname);
    if (!classp)
        return -1;
    for (std::list<const MDNode *>::iterator MI = classp->memberl.begin(), ME = classp->memberl.end(); MI != ME; MI++) {
        DIType Ty(*MI);
        if (Ty.getTag() == dwarf::DW_TAG_member && Ty.getName().str() == methodname)
            return Ty.getOffsetInBits()/8;
    }
    return -1;
}
