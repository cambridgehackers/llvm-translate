`ifndef __l_class_OC_Fifo1_OC_1_VH__
`define __l_class_OC_Fifo1_OC_1_VH__

`include "l_struct_OC_ValueType.vh"
`define l_class_OC_Fifo1_OC_1_RULE_COUNT (0)

//METAGUARD; out$deq__RDY; full;
//METAGUARD; in$enq__RDY; full ^ 1;
//METAGUARD; out$first__RDY; full;
//METAWRITE; out$deq; :full;
//METAWRITE; in$enq; :element;:full;
`endif
