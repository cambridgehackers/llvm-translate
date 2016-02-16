`ifndef __l_class_OC_IVector_VH__
`define __l_class_OC_IVector_VH__

`include "l_class_OC_Fifo1_OC_1.vh"
`define l_class_OC_IVector_RULE_COUNT (1 + `l_class_OC_Fifo1_OC_1_RULE_COUNT)

//METAINTERNAL; fifo; l_class_OC_Fifo1_OC_1;
//METAEXTERNAL; ind; l_class_OC_IVectorIndication;
//METAREAD; respond; :;gcounter$data:;fifo$out_first$a$data:;fifo$out_first$b$data;
//METAWRITE; respond; :;gcounter$data:;agg_2e_tmp$data:;agg_2e_tmp3$data;
//METAINVOKE; respond; :;fifo$out_first:;fifo$out_deq:;ind$heard;
//METAGUARD; respond__RDY; (fifo$out_first__RDY & fifo$out_deq__RDY) & ind$heard__RDY;
//METAREAD; say; :;say_meth$data:;say_v$data:;temp$a$data:;temp$b$data;
//METAWRITE; say; :;temp$a$data:;temp$b$data:;agg_2e_tmp$a$data:;agg_2e_tmp$b$data;
//METAINVOKE; say; :;fifo$in_enq;
//METAGUARD; say__RDY; fifo$in_enq__RDY;
`endif
