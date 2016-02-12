`ifndef __l_class_OC_FifoPong_VH__
`define __l_class_OC_FifoPong_VH__

`include "l_class_OC_Fifo1_OC_4.vh"
`define l_class_OC_FifoPong_RULE_COUNT (0 + `l_class_OC_Fifo1_OC_4_RULE_COUNT)

//METAINTERNAL; element1; l_class_OC_Fifo1_OC_4;
//METAINVOKE; in_enq; :;element1$in_enq;
//METAGUARD; in_enq__RDY; element1$in_enq__RDY;
//METAINVOKE; out_deq; :;element1$out_deq;
//METAGUARD; out_deq__RDY; element1$out_deq__RDY;
//METAINVOKE; out_first; :;element1$out_first;
//METAGUARD; out_first__RDY; element1$out_first__RDY;
`endif
