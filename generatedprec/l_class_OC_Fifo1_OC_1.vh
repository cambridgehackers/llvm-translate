`ifndef __l_class_OC_Fifo1_OC_1_VH__
`define __l_class_OC_Fifo1_OC_1_VH__

`include "l_struct_OC_ValueType.vh"
`define l_class_OC_Fifo1_OC_1_RULE_COUNT (0)

//METAWRITE; out$deq; :full;
//METAEXCLUSIVE; :out$deq; :in$enq
//METAGUARD; out$deq; full;
//METAWRITE; in$enq; :element;:full;
//METAGUARD; in$enq; full ^ 1;
//METAWRITE; out$first; :first$a;:first$b;
//METAGUARD; out$first; full;
`endif
