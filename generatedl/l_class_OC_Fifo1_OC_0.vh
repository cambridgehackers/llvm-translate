`ifndef __l_class_OC_Fifo1_OC_0_VH__
`define __l_class_OC_Fifo1_OC_0_VH__

`include "l_struct_OC_ValuePair.vh"
`define l_class_OC_Fifo1_OC_0_RULE_COUNT (0)

//METAEXCLUSIVE; out$deq; in$enq
//METAGUARD; out$deq; full;
//METAGUARD; in$enq; full ^ 1;
//METABEFORE; out$first; :in$enq
//METAGUARD; out$first; full;
`endif
