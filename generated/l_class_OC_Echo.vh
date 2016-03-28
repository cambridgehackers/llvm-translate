`ifndef __l_class_OC_Echo_VH__
`define __l_class_OC_Echo_VH__

`include "l_class_OC_Fifo1.vh"
`define l_class_OC_Echo_RULE_COUNT (1 + `l_class_OC_Fifo1_RULE_COUNT)

//METAINTERNAL; fifo; l_class_OC_Fifo1;
//METAEXTERNAL; ind; l_class_OC_EchoIndication;
//METAINVOKE; respond_rule; :fifo$out$deq;:fifo$out$first;:ind$heard;
//METAGUARD; respond_rule__RDY; (fifo$out$first__RDY & fifo$out$deq__RDY) & ind$heard__RDY;
//METAINVOKE; say; :fifo$in$enq;
//METAGUARD; say__RDY; fifo$in$enq__RDY;
//METARULES; respond_rule
`endif
