`ifndef __l_class_OC_IVector_VH__
`define __l_class_OC_IVector_VH__

`include "l_class_OC_FifoPong.vh"
`define l_class_OC_IVector_RULE_COUNT (10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT)

//METARULES; respond0; respond1; respond2; respond3; respond4; respond5; respond6; respond7; respond8; respond9
//METAINTERNAL; fifo0; l_class_OC_FifoPong;
//METAINTERNAL; fifo1; l_class_OC_FifoPong;
//METAINTERNAL; fifo2; l_class_OC_FifoPong;
//METAINTERNAL; fifo3; l_class_OC_FifoPong;
//METAINTERNAL; fifo4; l_class_OC_FifoPong;
//METAINTERNAL; fifo5; l_class_OC_FifoPong;
//METAINTERNAL; fifo6; l_class_OC_FifoPong;
//METAINTERNAL; fifo7; l_class_OC_FifoPong;
//METAINTERNAL; fifo8; l_class_OC_FifoPong;
//METAINTERNAL; fifo9; l_class_OC_FifoPong;
//METAINTERNAL; fifo10; l_class_OC_FifoPong;
//METAEXTERNAL; ind; l_class_OC_IVectorIndication;
//METAINVOKE; respond0; :fifo0$out$deq;:fifo0$out$first;:ind$heard;
//METAGUARD; respond0__RDY; (fifo0$out$first__RDY & fifo0$out$deq__RDY) & ind$heard__RDY;
//METAINVOKE; respond1; :fifo1$out$deq;:fifo1$out$first;:ind$heard;
//METAGUARD; respond1__RDY; (fifo1$out$first__RDY & fifo1$out$deq__RDY) & ind$heard__RDY;
//METAINVOKE; respond2; :fifo2$out$deq;:fifo2$out$first;:ind$heard;
//METAGUARD; respond2__RDY; (fifo2$out$first__RDY & fifo2$out$deq__RDY) & ind$heard__RDY;
//METAINVOKE; respond3; :fifo3$out$deq;:fifo3$out$first;:ind$heard;
//METAGUARD; respond3__RDY; (fifo3$out$first__RDY & fifo3$out$deq__RDY) & ind$heard__RDY;
//METAINVOKE; respond4; :fifo4$out$deq;:fifo4$out$first;:ind$heard;
//METAGUARD; respond4__RDY; (fifo4$out$first__RDY & fifo4$out$deq__RDY) & ind$heard__RDY;
//METAINVOKE; respond5; :fifo5$out$deq;:fifo5$out$first;:ind$heard;
//METAGUARD; respond5__RDY; (fifo5$out$first__RDY & fifo5$out$deq__RDY) & ind$heard__RDY;
//METAINVOKE; respond6; :fifo6$out$deq;:fifo6$out$first;:ind$heard;
//METAGUARD; respond6__RDY; (fifo6$out$first__RDY & fifo6$out$deq__RDY) & ind$heard__RDY;
//METAINVOKE; respond7; :fifo7$out$deq;:fifo7$out$first;:ind$heard;
//METAGUARD; respond7__RDY; (fifo7$out$first__RDY & fifo7$out$deq__RDY) & ind$heard__RDY;
//METAINVOKE; respond8; :fifo8$out$deq;:fifo8$out$first;:ind$heard;
//METAGUARD; respond8__RDY; (fifo8$out$first__RDY & fifo8$out$deq__RDY) & ind$heard__RDY;
//METAINVOKE; respond9; :fifo9$out$deq;:fifo9$out$first;:ind$heard;
//METAGUARD; respond9__RDY; (fifo9$out$first__RDY & fifo9$out$deq__RDY) & ind$heard__RDY;
//METAINVOKE; say; :(say_meth == 0 ? &fifo0:say_meth == 1 ? &fifo1:say_meth == 2 ? &fifo2:say_meth == 3 ? &fifo3:say_meth == 4 ? &fifo4:say_meth == 5 ? &fifo5:say_meth == 6 ? &fifo6:say_meth == 7 ? &fifo7:say_meth == 8 ? &fifo8:&fifo9)$in$enq;
//METAGUARD; say__RDY; ((((((((fifo0$in$enq__RDY & fifo1$in$enq__RDY) & fifo2$in$enq__RDY) & fifo3$in$enq__RDY) & fifo4$in$enq__RDY) & fifo5$in$enq__RDY) & fifo6$in$enq__RDY) & fifo7$in$enq__RDY) & fifo8$in$enq__RDY) & fifo9$in$enq__RDY;
`endif
