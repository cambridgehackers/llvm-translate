`ifndef __l_class_OC_FifoPong_VH__
`define __l_class_OC_FifoPong_VH__

`include "l_class_OC_Fifo1_OC_3.vh"
`define l_class_OC_FifoPong_RULE_COUNT (0 + `l_class_OC_Fifo1_OC_3_RULE_COUNT + `l_class_OC_Fifo1_OC_3_RULE_COUNT)

//METAINTERNAL; element1; l_class_OC_Fifo1_OC_3;
//METAINTERNAL; element2; l_class_OC_Fifo1_OC_3;
//METAGUARD; in_enq__RDY; (element2$in_enq__RDY | (pong ^ 1)) & (element1$in_enq__RDY | pong);
//METAGUARD; out_deq__RDY; (element2$out_deq__RDY | (pong ^ 1)) & (element1$out_deq__RDY | pong);
//METAGUARD; out_first__RDY; (element2$out_first__RDY | (pong ^ 1)) & (element1$out_first__RDY | pong);
//METAREAD; in_enq; :;pong:;pong:pong ^ 1;pong:pong ^ 1;pong;
//METAINVOKE; in_enq; :pong;element2$in_enq:pong ^ 1;element1$in_enq;
//METAREAD; out_deq; :;pong:pong ^ 1;pong:;pong;
//METAWRITE; out_deq; :;pong;
//METAINVOKE; out_deq; :pong;element2$out_deq:pong ^ 1;element1$out_deq;
//METAINVOKE; out_first; :pong;element2$out_first:pong ^ 1;element1$out_first;
`endif
