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
//METAREAD; respond0; :;agg_2e_tmp$data;
//METAINVOKE; respond0; :;fifo0$out_first:;fifo0$out_deq:;ind$heard;
//METAGUARD; respond0__RDY; (fifo0$out_first__RDY & fifo0$out_deq__RDY) & ind$heard__RDY;
//METAREAD; respond1; :;agg_2e_tmp$data;
//METAINVOKE; respond1; :;fifo1$out_first:;fifo1$out_deq:;ind$heard;
//METAGUARD; respond1__RDY; (fifo1$out_first__RDY & fifo1$out_deq__RDY) & ind$heard__RDY;
//METAREAD; respond2; :;agg_2e_tmp$data;
//METAINVOKE; respond2; :;fifo2$out_first:;fifo2$out_deq:;ind$heard;
//METAGUARD; respond2__RDY; (fifo2$out_first__RDY & fifo2$out_deq__RDY) & ind$heard__RDY;
//METAREAD; respond3; :;agg_2e_tmp$data;
//METAINVOKE; respond3; :;fifo3$out_first:;fifo3$out_deq:;ind$heard;
//METAGUARD; respond3__RDY; (fifo3$out_first__RDY & fifo3$out_deq__RDY) & ind$heard__RDY;
//METAREAD; respond4; :;agg_2e_tmp$data;
//METAINVOKE; respond4; :;fifo4$out_first:;fifo4$out_deq:;ind$heard;
//METAGUARD; respond4__RDY; (fifo4$out_first__RDY & fifo4$out_deq__RDY) & ind$heard__RDY;
//METAREAD; respond5; :;agg_2e_tmp$data;
//METAINVOKE; respond5; :;fifo5$out_first:;fifo5$out_deq:;ind$heard;
//METAGUARD; respond5__RDY; (fifo5$out_first__RDY & fifo5$out_deq__RDY) & ind$heard__RDY;
//METAREAD; respond6; :;agg_2e_tmp$data;
//METAINVOKE; respond6; :;fifo6$out_first:;fifo6$out_deq:;ind$heard;
//METAGUARD; respond6__RDY; (fifo6$out_first__RDY & fifo6$out_deq__RDY) & ind$heard__RDY;
//METAREAD; respond7; :;agg_2e_tmp$data;
//METAINVOKE; respond7; :;fifo7$out_first:;fifo7$out_deq:;ind$heard;
//METAGUARD; respond7__RDY; (fifo7$out_first__RDY & fifo7$out_deq__RDY) & ind$heard__RDY;
//METAREAD; respond8; :;agg_2e_tmp$data;
//METAINVOKE; respond8; :;fifo8$out_first:;fifo8$out_deq:;ind$heard;
//METAGUARD; respond8__RDY; (fifo8$out_first__RDY & fifo8$out_deq__RDY) & ind$heard__RDY;
//METAREAD; respond9; :;agg_2e_tmp$data;
//METAINVOKE; respond9; :;fifo9$out_first:;fifo9$out_deq:;ind$heard;
//METAGUARD; respond9__RDY; (fifo9$out_first__RDY & fifo9$out_deq__RDY) & ind$heard__RDY;
//METAWRITE; say; :;v$data;
//METAINVOKE; say; :;temp$ValuePair:;(say_meth == 0 ? &fifo0:say_meth == 1 ? &fifo1:say_meth == 2 ? &fifo2:say_meth == 3 ? &fifo3:say_meth == 4 ? &fifo4:say_meth == 5 ? &fifo5:say_meth == 6 ? &fifo6:say_meth == 7 ? &fifo7:say_meth == 8 ? &fifo8:&fifo9)$in_enq;
//METAGUARD; say__RDY; ((((((((fifo0$in_enq__RDY & fifo1$in_enq__RDY) & fifo2$in_enq__RDY) & fifo3$in_enq__RDY) & fifo4$in_enq__RDY) & fifo5$in_enq__RDY) & fifo6$in_enq__RDY) & fifo7$in_enq__RDY) & fifo8$in_enq__RDY) & fifo9$in_enq__RDY;
`endif
