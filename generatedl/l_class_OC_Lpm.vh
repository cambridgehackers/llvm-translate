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
//METAGUARD; enter__RDY; ((inQ8$out$first__RDY & inQ8$out$deq__RDY) & fifo8$in$enq__RDY) & mem$req__RDY;
//METAGUARD; exit__RDY; (((fifo8$out$first__RDY & mem$resValue__RDY) & mem$resAccept__RDY) & fifo8$out$deq__RDY) & outQ8$in$enq__RDY;
//METAGUARD; recirc__RDY; ((((((1 ^ 1) & fifo8$out$first__RDY) & mem$resValue__RDY) & mem$resAccept__RDY) & fifo8$out$deq__RDY) & fifo8$in$enq__RDY) & mem$req__RDY;
//METAGUARD; respond__RDY; (outQ8$out$first__RDY & outQ8$out$deq__RDY) & indication$heard__RDY;
//METAGUARD; say__RDY; inQ8$in$enq__RDY;
//METAINVOKE; enter; :inQ8$out$first;:inQ8$out$deq;:fifo8$in$enq;:mem$req;
//METAINVOKE; exit; :fifo8$out$first;:mem$resValue;:mem$resAccept;:fifo8$out$deq;:outQ8$in$enq;
//METAINVOKE; recirc; :fifo8$out$first;:mem$resValue;:mem$resAccept;:fifo8$out$deq;:fifo8$in$enq;:mem$req;
//METAINVOKE; respond; :outQ8$out$first;:outQ8$out$deq;:indication$heard;
//METAINVOKE; say; :inQ8$in$enq;
`endif
