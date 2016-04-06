`ifndef __l_class_OC_Lpm_VH__
`define __l_class_OC_Lpm_VH__

`include "l_class_OC_Fifo1_OC_0.vh"
`include "l_class_OC_Fifo2.vh"
`include "l_class_OC_LpmMemory.vh"
`define l_class_OC_Lpm_RULE_COUNT (4 + `l_class_OC_Fifo1_OC_0_RULE_COUNT + `l_class_OC_Fifo2_RULE_COUNT + `l_class_OC_Fifo1_OC_0_RULE_COUNT + `l_class_OC_LpmMemory_RULE_COUNT)

//METAINVOKE; enter; :fifo$in$enq;:inQ$out$deq;:inQ$out$first;:mem$req;
//METAEXCLUSIVE; enter; recirc
//METAGUARD; enter; ((inQ$out$first__RDY & inQ$out$deq__RDY) & fifo$in$enq__RDY) & mem$req__RDY;
//METAINVOKE; exit; :fifo$out$deq;:fifo$out$first;:mem$resAccept;:mem$resValue;:outQ$in$enq;
//METAEXCLUSIVE; exit; recirc
//METAGUARD; exit; (((fifo$out$first__RDY & mem$resValue__RDY) & mem$resAccept__RDY) & fifo$out$deq__RDY) & outQ$in$enq__RDY;
//METAINVOKE; recirc; :fifo$in$enq;:fifo$out$deq;:fifo$out$first;:mem$req;:mem$resAccept;:mem$resValue;
//METAGUARD; recirc; ((((fifo$out$first__RDY & mem$resValue__RDY) & mem$resAccept__RDY) & fifo$out$deq__RDY) & fifo$in$enq__RDY) & mem$req__RDY;
//METAINVOKE; respond; :indication$heard;:outQ$out$deq;:outQ$out$first;
//METAGUARD; respond; (outQ$out$first__RDY & outQ$out$deq__RDY) & indication$heard__RDY;
//METAINVOKE; say; :inQ$in$enq;
//METAGUARD; say; inQ$in$enq__RDY;
//METARULES; enter; exit; recirc; respond
//METAPRIORITY; recirc; enter;exit
//METAINTERNAL; inQ; l_class_OC_Fifo1_OC_0;
//METAINTERNAL; fifo; l_class_OC_Fifo2;
//METAINTERNAL; outQ; l_class_OC_Fifo1_OC_0;
//METAINTERNAL; mem; l_class_OC_LpmMemory;
//METAEXTERNAL; indication; l_class_OC_LpmIndication;
`endif
