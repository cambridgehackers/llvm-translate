`ifndef __l_class_OC_LpmMemory_VH__
`define __l_class_OC_LpmMemory_VH__

`include "l_struct_OC_ValuePair.vh"
`define l_class_OC_LpmMemory_RULE_COUNT (1)

//METAEXCLUSIVE; memdelay; req; resAccept
//METABEFORE; memdelay; :req; :resAccept
//METAGUARD; memdelay; delayCount > 1;
//METAEXCLUSIVE; req; resAccept
//METAGUARD; req; delayCount == 0;
//METAGUARD; resAccept; delayCount == 1;
//METABEFORE; resValue; :req
//METAGUARD; resValue; delayCount == 1;
//METARULES; memdelay
`endif
