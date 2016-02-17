`ifndef __l_class_OC_Connect_VH__
`define __l_class_OC_Connect_VH__

`include "l_class_OC_Fifo1_OC_1.vh"
`include "l_class_OC_XsimTop.vh"
`define l_class_OC_Connect_RULE_COUNT (1 + `l_class_OC_Fifo1_OC_1_RULE_COUNT + `l_class_OC_XsimTop_RULE_COUNT)

//METAINTERNAL; fifo; l_class_OC_Fifo1_OC_1;
//METAEXTERNAL; ind; l_class_OC_ConnectIndication;
//METAINTERNAL; lXsimTop; l_class_OC_XsimTop;
//METAINVOKE; respond; :;fifo$out_first:;fifo$out_deq:;ind$heard;
//METAGUARD; respond__RDY; (fifo$out_first__RDY & fifo$out_deq__RDY) & ind$heard__RDY;
//METAINVOKE; say; :;fifo$in_enq;
//METAGUARD; say__RDY; fifo$in_enq__RDY;
`endif
