`ifndef __l_class_OC_Fifo1_VH__
`define __l_class_OC_Fifo1_VH__

`define l_class_OC_Fifo1_RULE_COUNT (0)

//METAGUARD; in_enq__RDY; full ^ 1;
//METAGUARD; out_deq__RDY; full;
//METAGUARD; out_first__RDY; full;
//METAREAD; in_enq; :in_enq_v;
//METAWRITE; in_enq; :element;:full;
//METAWRITE; out_deq; :full;
//METAREAD; out_first; :element;
`endif
