`ifndef __l_class_OC_IVector_VH__
`define __l_class_OC_IVector_VH__

`include "l_class_OC_FifoPong.vh"
`define l_class_OC_IVector_RULE_COUNT (1 + `l_class_OC_FifoPong_RULE_COUNT)

//METAINTERNAL; fifo; l_class_OC_FifoPong;
//METAEXTERNAL; ind; l_class_OC_IVectorIndication;
//METAINVOKE; respond_rule; :;fifo$out_deq:;fifo$out_first:;ind$heard;
//METAGUARD; respond_rule__RDY; (fifo$out_deq__RDY & fifo$out_first__RDY) & ind$heard__RDY;
//METAINVOKE; say; :;fifo$in_enq;
//METAGUARD; say__RDY; fifo$in_enq__RDY;
`endif
