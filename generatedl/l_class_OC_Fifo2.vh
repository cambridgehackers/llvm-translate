`ifndef __l_class_OC_Fifo2_VH__
`define __l_class_OC_Fifo2_VH__

`define l_class_OC_Fifo2_RULE_COUNT (0)

//METAEXTERNAL; element; l_struct_OC_ValuePair;
//METAGUARD; out$deq__RDY; rindex != windex;
//METAGUARD; in$enq__RDY; ((windex + 1) % 2) != rindex;
//METAGUARD; out$first__RDY; rindex != windex;
//METAREAD; out$deq; :rindex;
//METAWRITE; out$deq; :rindex;
//METAREAD; in$enq; :windex;
//METAWRITE; in$enq; windex == 0:element0;windex != 0:element1;:windex;
//METAREAD; out$first; rindex == 0:element0;rindex != 0:element1;:rindex;
`endif
