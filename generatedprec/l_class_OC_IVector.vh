`ifndef __l_class_OC_IVector_VH__
`define __l_class_OC_IVector_VH__

`include "l_class_OC_Fifo_OC_1.vh"
`define l_class_OC_IVector_RULE_COUNT (1 + `l_class_OC_Fifo_OC_1_RULE_COUNT)

//METAINTERNAL; fifo; l_class_OC_Fifo_OC_1;
//METAEXTERNAL; ind; l_class_OC_IVectorIndication;
//METAINVOKE; respond; :;fifo$out_first:;fifo$out_deq:;agg_2e_tmp$FixedPoint:;agg_2e_tmp3$FixedPoint:;ind$heard:;agg_2e_tmp3$~FixedPoint:;agg_2e_tmp$~FixedPoint:;fifo$out_first$;
//METAGUARD; respond__RDY; (fifo$out_first__RDY & fifo$out_deq__RDY) & ind$heard__RDY;
//METAREAD; say; :;(say_meth)$data:;(say_v)$data;
//METAWRITE; say; :;temp$a$data:;temp$b$data;
//METAINVOKE; say; :;temp$:;agg_2e_tmp$:;fifo$in_enq:;agg_2e_tmp$:;temp$;
//METAGUARD; say__RDY; fifo$in_enq__RDY;
`endif
