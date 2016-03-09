`ifndef __l_class_OC_Echo_VH__
`define __l_class_OC_Echo_VH__

`include "l_class_OC_Fifo1.vh"
`define l_class_OC_Echo_RULE_COUNT (1 + `l_class_OC_Fifo1_RULE_COUNT)

//METAINTERNAL; fifo; l_class_OC_Fifo1;
//METAEXTERNAL; ind; l_class_OC_EchoIndication;
//METAGUARD; respond_rule__RDY; (fifo$out_first__RDY & fifo$out_deq__RDY) & ind$heard__RDY;
//METAGUARD; say__RDY; fifo$in_enq__RDY;
//METAINVOKE; respond_rule; :;fifo$out_first:;fifo$out_deq:;ind$heard;
//METAINVOKE; say; :;fifo$in_enq;
`endif
