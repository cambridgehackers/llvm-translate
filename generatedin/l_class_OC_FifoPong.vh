`ifndef __l_class_OC_FifoPong_VH__
`define __l_class_OC_FifoPong_VH__

`include "l_class_OC_Fifo1_OC_3.vh"
`define l_class_OC_FifoPong_RULE_COUNT (0 + `l_class_OC_Fifo1_OC_3_RULE_COUNT + `l_class_OC_Fifo1_OC_3_RULE_COUNT)

//METAREAD; out$deq; :pong;
//METAWRITE; out$deq; :pong;
//METAINVOKE; out$deq; pong ^ 1:element1$out$deq;pong:element2$out$deq;
//METAGUARD; out$deq__RDY; (element2$out$deq__RDY | (pong ^ 1)) & (element1$out$deq__RDY | pong);
//METAREAD; in$enq; :pong;
//METAINVOKE; in$enq; pong ^ 1:element1$in$enq;pong:element2$in$enq;
//METAGUARD; in$enq__RDY; (element2$in$enq__RDY | (pong ^ 1)) & (element1$in$enq__RDY | pong);
//METAINVOKE; out$first; pong ^ 1:element1$out$first;pong:element2$out$first;
//METAGUARD; out$first__RDY; (element2$out$first__RDY | (pong ^ 1)) & (element1$out$first__RDY | pong);
//METAINTERNAL; element1; l_class_OC_Fifo1_OC_3;
//METAINTERNAL; element2; l_class_OC_Fifo1_OC_3;
`endif
