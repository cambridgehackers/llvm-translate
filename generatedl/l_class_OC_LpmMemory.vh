`ifndef __l_class_OC_LpmMemory_VH__
`define __l_class_OC_LpmMemory_VH__

`include "l_struct_OC_ValuePair.vh"
`define l_class_OC_LpmMemory_RULE_COUNT (1)

//METAREAD; memdelay; :delayCount;
//METAWRITE; memdelay; :delayCount;
//METAEXCLUSIVE; memdelay; :req; :resAccept
//METAGUARD; memdelay__RDY; delayCount > 1;
//METAWRITE; req; :delayCount;:saved;
//METAGUARD; req__RDY; delayCount == 0;
//METAWRITE; resAccept; :delayCount;
//METAGUARD; resAccept__RDY; delayCount == 1;
//METAREAD; resValue; :saved;
//METABEFORE; resValue; :req
//METAGUARD; resValue__RDY; delayCount == 1;
//METARULES; memdelay
`endif
