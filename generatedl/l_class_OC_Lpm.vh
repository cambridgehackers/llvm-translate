`ifndef __l_class_OC_Lpm_VH__
`define __l_class_OC_Lpm_VH__

`include "l_class_OC_Fifo1_OC_0.vh"
`define l_class_OC_Lpm_RULE_COUNT (1 + `l_class_OC_Fifo1_OC_0_RULE_COUNT)

//METARULES; respond
//METAINTERNAL; fifo; l_class_OC_Fifo1_OC_0;
//METAEXTERNAL; indication; l_class_OC_LpmIndication;
//METAGUARD; respond__RDY; (fifo8$out$first__RDY & fifo8$out$deq__RDY) & indication$heard__RDY;
//METAGUARD; say__RDY; fifo8$in$enq__RDY;
//METAINVOKE; respond; :fifo8$out$first;:fifo8$out$deq;:indication$heard;
//METAINVOKE; say; :fifo8$in$enq;
`endif
