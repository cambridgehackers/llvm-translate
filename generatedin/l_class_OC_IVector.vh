`ifndef __l_class_OC_IVector_VH__
`define __l_class_OC_IVector_VH__

`define l_class_OC_IVector_RULE_COUNT (1)

//METAEXTERNAL; fifo; l_class_OC_Fifo_OC_0;
//METAEXTERNAL; ind; l_class_OC_IVectorIndication;
//METAREAD; respond_rule; :;fifo$out_first$a:;fifo$out_first$b;
//METAINVOKE; respond_rule; :;fifo$out_first:;fifo$out_deq:;ind$heard;
//METAGUARD; respond_rule__RDY; (fifo$out_first__RDY & fifo$out_deq__RDY) & ind$heard__RDY;
//METAWRITE; say; :;temp$a:;temp$b;
//METAINVOKE; say; :;fifo$in_enq;
//METAGUARD; say__RDY; fifo$in_enq__RDY;
`endif
