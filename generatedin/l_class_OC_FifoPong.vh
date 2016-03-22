`ifndef __l_class_OC_FifoPong_VH__
`define __l_class_OC_FifoPong_VH__

`include "l_class_OC_Fifo1_OC_3.vh"
`define l_class_OC_FifoPong_RULE_COUNT (0 + `l_class_OC_Fifo1_OC_3_RULE_COUNT + `l_class_OC_Fifo1_OC_3_RULE_COUNT)

//METAINTERNAL; element1; l_class_OC_Fifo1_OC_3;
//METAINTERNAL; element2; l_class_OC_Fifo1_OC_3;
//METAGUARD; out$deq__RDY; (element28$out$deq__RDY | (pong ^ 1)) & (element18$out$deq__RDY | pong);
//METAGUARD; in$enq__RDY; (element28$in$enq__RDY | (pong ^ 1)) & (element18$in$enq__RDY | pong);
//METAGUARD; out$first__RDY; (element28$out$first__RDY | (pong ^ 1)) & (element18$out$first__RDY | pong);
//METAREAD; out$deq; :pong;
//METAWRITE; out$deq; :pong;
//METAINVOKE; out$deq; pong ^ 1:element18$out$deq;pong:element28$out$deq;
//METAREAD; in$enq; :pong;
//METAINVOKE; in$enq; pong ^ 1:element18$in$enq;pong:element28$in$enq;
//METAINVOKE; out$first; pong ^ 1:element18$out$first;pong:element28$out$first;
`endif
