`ifndef __l_class_OC_Fifo1_OC_3_VH__
`define __l_class_OC_Fifo1_OC_3_VH__

`include "l_struct_OC_ValuePair.vh"
`define l_class_OC_Fifo1_OC_3_RULE_COUNT (0)

//METAWRITE; out$deq; :full;
//METAEXCLUSIVE; full:out$deq; full ^ 1:in$enq
//METAGUARD; out$deq; full;
//METAWRITE; in$enq; :element;:full;
//METAGUARD; in$enq; full ^ 1;
//METAREAD; out$first; :element;
//METABEFORE; out$first; :in$enq
//METAGUARD; out$first; full;
`endif
