`ifndef __l_class_OC_Lpm_VH__
`define __l_class_OC_Lpm_VH__

`include "l_class_OC_Fifo1_OC_0.vh"
`include "l_class_OC_LpmMemory.vh"
`define l_class_OC_Lpm_RULE_COUNT (4 + `l_class_OC_Fifo1_OC_0_RULE_COUNT + `l_class_OC_Fifo1_OC_0_RULE_COUNT + `l_class_OC_Fifo1_OC_0_RULE_COUNT + `l_class_OC_LpmMemory_RULE_COUNT)

//METARULES; enter; exit; recirc; respond
//METAINTERNAL; inQ; l_class_OC_Fifo1_OC_0;
//METAINTERNAL; fifo; l_class_OC_Fifo1_OC_0;
//METAINTERNAL; outQ; l_class_OC_Fifo1_OC_0;
//METAINTERNAL; mem; l_class_OC_LpmMemory;
//METAEXTERNAL; indication; l_class_OC_LpmIndication;
//METAGUARD; enter__RDY; ((inQ$out$first__RDY & inQ$out$deq__RDY) & fifo$in$enq__RDY) & mem$req__RDY;
//METAGUARD; exit__RDY; (((((((doneCount % 5) != 0) ^ 1) & fifo$out$first__RDY) & mem$resValue__RDY) & mem$resAccept__RDY) & fifo$out$deq__RDY) & outQ$in$enq__RDY;
//METAGUARD; recirc__RDY; (((((((((doneCount % 5) != 0) ^ 1) ^ 1) & fifo$out$first__RDY) & mem$resValue__RDY) & mem$resAccept__RDY) & fifo$out$deq__RDY) & fifo$in$enq__RDY) & mem$req__RDY;
//METAGUARD; respond__RDY; (outQ$out$first__RDY & outQ$out$deq__RDY) & indication$heard__RDY;
//METAGUARD; say__RDY; inQ$in$enq__RDY;
//METAINVOKE; enter; :fifo$in$enq;:inQ$out$deq;:inQ$out$first;:mem$req;
//METAINVOKE; exit; :fifo$out$deq;:fifo$out$first;:mem$resAccept;:mem$resValue;:outQ$in$enq;
//METAINVOKE; recirc; :fifo$in$enq;:fifo$out$deq;:fifo$out$first;:mem$req;:mem$resAccept;:mem$resValue;
//METAINVOKE; respond; :indication$heard;:outQ$out$deq;:outQ$out$first;
//METAINVOKE; say; :inQ$in$enq;
`endif
