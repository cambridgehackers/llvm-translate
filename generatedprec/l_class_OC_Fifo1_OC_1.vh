`ifndef __l_class_OC_Fifo1_OC_1_VH__
`define __l_class_OC_Fifo1_OC_1_VH__

`include "l_struct_OC_ValueType.vh"
`define l_class_OC_Fifo1_OC_1_RULE_COUNT (0)

//METAGUARD; in_enq__RDY; full ^ 1;
//METAGUARD; out_deq__RDY; full;
//METAGUARD; out_first__RDY; full;
//METAWRITE; in_enq; :full;
//METAWRITE; out_deq; :full;
//METAINVOKE; out_first; :out_first$;
`endif
