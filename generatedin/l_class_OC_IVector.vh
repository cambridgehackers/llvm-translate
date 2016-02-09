`ifndef __l_class_OC_IVector_VH__
`define __l_class_OC_IVector_VH__

`include "l_class_OC_FifoPong.vh"
`define l_class_OC_IVector_RULE_COUNT (10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT)

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
//METAREAD; respond0; :;fifo0$out_first$a:;fifo0$out_first$b;
//METAINVOKE; respond0; :;fifo0$out_first:;fifo0$out_deq:;ind$heard;
//METAGUARD; respond0__RDY; (fifo0$out_first__RDY & fifo0$out_deq__RDY) & ind$heard__RDY;
//METAREAD; respond1; :;fifo1$out_first$a:;fifo1$out_first$b;
//METAINVOKE; respond1; :;fifo1$out_first:;fifo1$out_deq:;ind$heard;
//METAGUARD; respond1__RDY; (fifo1$out_first__RDY & fifo1$out_deq__RDY) & ind$heard__RDY;
//METAREAD; respond2; :;fifo2$out_first$a:;fifo2$out_first$b;
//METAINVOKE; respond2; :;fifo2$out_first:;fifo2$out_deq:;ind$heard;
//METAGUARD; respond2__RDY; (fifo2$out_first__RDY & fifo2$out_deq__RDY) & ind$heard__RDY;
//METAREAD; respond3; :;fifo3$out_first$a:;fifo3$out_first$b;
//METAINVOKE; respond3; :;fifo3$out_first:;fifo3$out_deq:;ind$heard;
//METAGUARD; respond3__RDY; (fifo3$out_first__RDY & fifo3$out_deq__RDY) & ind$heard__RDY;
//METAREAD; respond4; :;fifo4$out_first$a:;fifo4$out_first$b;
//METAINVOKE; respond4; :;fifo4$out_first:;fifo4$out_deq:;ind$heard;
//METAGUARD; respond4__RDY; (fifo4$out_first__RDY & fifo4$out_deq__RDY) & ind$heard__RDY;
//METAREAD; respond5; :;fifo5$out_first$a:;fifo5$out_first$b;
//METAINVOKE; respond5; :;fifo5$out_first:;fifo5$out_deq:;ind$heard;
//METAGUARD; respond5__RDY; (fifo5$out_first__RDY & fifo5$out_deq__RDY) & ind$heard__RDY;
//METAREAD; respond6; :;fifo6$out_first$a:;fifo6$out_first$b;
//METAINVOKE; respond6; :;fifo6$out_first:;fifo6$out_deq:;ind$heard;
//METAGUARD; respond6__RDY; (fifo6$out_first__RDY & fifo6$out_deq__RDY) & ind$heard__RDY;
//METAREAD; respond7; :;fifo7$out_first$a:;fifo7$out_first$b;
//METAINVOKE; respond7; :;fifo7$out_first:;fifo7$out_deq:;ind$heard;
//METAGUARD; respond7__RDY; (fifo7$out_first__RDY & fifo7$out_deq__RDY) & ind$heard__RDY;
//METAREAD; respond8; :;fifo8$out_first$a:;fifo8$out_first$b;
//METAINVOKE; respond8; :;fifo8$out_first:;fifo8$out_deq:;ind$heard;
//METAGUARD; respond8__RDY; (fifo8$out_first__RDY & fifo8$out_deq__RDY) & ind$heard__RDY;
//METAREAD; respond9; :;fifo9$out_first$a:;fifo9$out_first$b;
//METAINVOKE; respond9; :;fifo9$out_first:;fifo9$out_deq:;ind$heard;
//METAGUARD; respond9__RDY; (fifo9$out_first__RDY & fifo9$out_deq__RDY) & ind$heard__RDY;
//METAWRITE; say; :;temp$a:;temp$b;
//METAINVOKE; say; :;fifo1$in_enq;
//METAGUARD; say__RDY; fifo1$in_enq__RDY;
`endif
