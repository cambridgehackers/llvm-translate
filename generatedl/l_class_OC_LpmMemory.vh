`ifndef __l_class_OC_LpmMemory_VH__
`define __l_class_OC_LpmMemory_VH__

`include "l_struct_OC_ValuePair.vh"
`define l_class_OC_LpmMemory_RULE_COUNT (1)

//METARULES; memdelay
//METAGUARD; memdelay__RDY; delayCount > 1;
//METAGUARD; req__RDY; delayCount == 0;
//METAGUARD; resAccept__RDY; delayCount == 1;
//METAGUARD; resValue__RDY; delayCount == 1;
//METAREAD; memdelay; :delayCount;
//METAWRITE; memdelay; :delayCount;
//METAWRITE; req; :delayCount;
//METAWRITE; resAccept; :delayCount;
//METAREAD; resValue; :saved;
`endif
