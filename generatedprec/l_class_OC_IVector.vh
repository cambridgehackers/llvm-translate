`ifndef __l_class_OC_IVector_VH__
`define __l_class_OC_IVector_VH__

`include "l_class_OC_Fifo1_OC_1.vh"
`define l_class_OC_IVector_RULE_COUNT (1 + `l_class_OC_Fifo1_OC_1_RULE_COUNT)

//METAWRITE; respond; :agg_2e_tmp;:agg_2e_tmp4;:gcounter;
//METAINVOKE; respond; :fifo$out$deq;:fifo$out$first;:ind$heard;
//METAGUARD; respond__RDY; (fifo$out$first__RDY & fifo$out$deq__RDY) & ind$heard__RDY;
//METAINVOKE; say; :fifo$in$enq;
//METAGUARD; say__RDY; fifo$in$enq__RDY;
//METARULES; respond
//METAINTERNAL; fifo; l_class_OC_Fifo1_OC_1;
//METAEXTERNAL; ind; l_class_OC_IVectorIndication;
`endif
