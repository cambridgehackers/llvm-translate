`ifndef __l_class_OC_Fifo1_OC_1_VH__
`define __l_class_OC_Fifo1_OC_1_VH__

`include "l_struct_OC_ValueType.vh"
`define l_class_OC_Fifo1_OC_1_RULE_COUNT (0)

//METAREAD; in_enq; :;(in_enq_v)$a$data:;(in_enq_v)$b$data;
//METAWRITE; in_enq; :;element$a$data:;element$b$data:;full;
//METAGUARD; in_enq__RDY; full ^ 1;
//METAWRITE; out_deq; :;full;
//METAGUARD; out_deq__RDY; full;
//METAREAD; out_first; :;element$a$data:;element$b$data;
//METAWRITE; out_first; :;(agg_2e_result)$a$data:;(agg_2e_result)$b$data;
//METAGUARD; out_first__RDY; full;
`endif
