`ifndef __l_class_OC_Fifo1_OC_1_VH__
`define __l_class_OC_Fifo1_OC_1_VH__

`include "l_struct_OC_ValueType.vh"
`define l_class_OC_Fifo1_OC_1_RULE_COUNT (0)

//METAEXCLUSIVE; out$deq; in$enq
//METAGUARD; out$deq; full;
//METAGUARD; in$enq; full ^ 1;
//METAGUARD; out$first; full;
`endif
