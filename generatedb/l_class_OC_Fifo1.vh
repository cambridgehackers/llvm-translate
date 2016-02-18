`ifndef __l_class_OC_Fifo1_VH__
`define __l_class_OC_Fifo1_VH__

`define l_class_OC_Fifo1_RULE_COUNT (0)

//METAREAD; in_enq; :;in_enq_v;
//METAWRITE; in_enq; :;element:;full;
//METAGUARD; in_enq__RDY; full ^ 1;
//METAWRITE; out_deq; :;full;
//METAGUARD; out_deq__RDY; full;
//METAREAD; out_first; :;element;
//METAGUARD; out_first__RDY; full;
`endif
