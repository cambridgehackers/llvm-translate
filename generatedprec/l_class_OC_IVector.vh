`ifndef __l_class_OC_IVector_VH__
`define __l_class_OC_IVector_VH__

`include "l_class_OC_Fifo_OC_0.vh"
`define l_class_OC_IVector_RULE_COUNT (1 + `l_class_OC_Fifo_OC_0_RULE_COUNT)

//METAINTERNAL; fifo; l_class_OC_Fifo_OC_0;
//METAEXTERNAL; ind; l_class_OC_IVectorIndication;
//METAREAD; respond; :;agg_2e_tmp$data:;agg_2e_tmp3$data;
//METAINVOKE; respond; :;fifo$out_first:;fifo$out_deq:;ind$heard;
//METAGUARD; respond__RDY; (fifo$out_first__RDY & fifo$out_deq__RDY) & ind$heard__RDY;
//METAWRITE; say; :;meth$data:;v$data;
//METAINVOKE; say; :;temp$ValuePair:;fifo$in_enq;
//METAGUARD; say__RDY; fifo$in_enq__RDY;
`endif
