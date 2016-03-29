`ifndef __l_class_OC_LpmMemory_VH__
`define __l_class_OC_LpmMemory_VH__

`include "l_struct_OC_ValuePair.vh"
`define l_class_OC_LpmMemory_RULE_COUNT (1)

//METAREAD; memdelay; :delayCount;
//METAWRITE; memdelay; :delayCount;
//METAEXCLUSIVE; delayCount > 1:memdelay; delayCount == 0:req; delayCount == 1:resAccept
//METABEFORE; memdelay; :req; :resAccept
//METAGUARD; memdelay; delayCount > 1;
//METAWRITE; req; :delayCount;:saved;
//METAGUARD; req; delayCount == 0;
//METAWRITE; resAccept; :delayCount;
//METAGUARD; resAccept; delayCount == 1;
//METAREAD; resValue; :saved;
//METABEFORE; resValue; :req
//METAGUARD; resValue; delayCount == 1;
//METARULES; memdelay
`endif
