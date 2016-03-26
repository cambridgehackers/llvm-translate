`ifndef __l_class_OC_Fifo1_VH__
`define __l_class_OC_Fifo1_VH__

`define l_class_OC_Fifo1_RULE_COUNT (0)

//METAWRITE; out$deq; :full;
//METAGUARD; out$deq__RDY; full;
//METAWRITE; in$enq; :element;:full;
//METAGUARD; in$enq__RDY; full ^ 1;
//METAREAD; out$first; :element;
//METAGUARD; out$first__RDY; full;
`endif
