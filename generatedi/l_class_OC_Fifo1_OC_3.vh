`ifndef __l_class_OC_Fifo1_OC_3_VH__
`define __l_class_OC_Fifo1_OC_3_VH__

`include "l_struct_OC_ValuePair.vh"
`define l_class_OC_Fifo1_OC_3_RULE_COUNT (0 + `l_struct_OC_ValuePair_RULE_COUNT)

//METAINTERNAL; element; l_struct_OC_ValuePair;
//METAWRITE; in_enq; :;full;
//METAGUARD; in_enq__RDY; full ^ 1;
//METAWRITE; out_deq; :;full;
//METAGUARD; out_deq__RDY; full;
//METAINVOKE; out_first; :;agg_2e_result$;
//METAGUARD; out_first__RDY; full;
`endif
