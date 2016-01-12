`ifndef __l_class_OC_IVector_VH__
`define __l_class_OC_IVector_VH__

`include "l_class_OC_FifoPong.vh"
`define l_class_OC_IVector_RULE_COUNT (1 + `l_class_OC_FifoPong_RULE_COUNT)

//METAINTERNAL; fifo; l_class_OC_FifoPong;
//METAEXTERNAL; ind; l_class_OC_IVectorIndication;
//METAREAD; respond_rule; :;&fifo[1]$out_first$a:;&fifo[1]$out_first$b;
//METAINVOKE; respond_rule; :;&fifo[1]$out_first:;&fifo[1]$out_deq:;ind$heard;
//METAGUARD; respond_rule__RDY; ((&fifo[1]$out_first__RDY) & (&fifo[1]$out_deq__RDY)) & ind$heard__RDY;
//METAWRITE; say; :;temp$a:;temp$b;
//METAINVOKE; say; :;&fifo[1]$in_enq;
//METAGUARD; say__RDY; &fifo[1]$in_enq__RDY;
`endif
