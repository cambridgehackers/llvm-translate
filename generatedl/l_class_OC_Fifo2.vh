`ifndef __l_class_OC_Fifo2_VH__
`define __l_class_OC_Fifo2_VH__

`define l_class_OC_Fifo2_RULE_COUNT (0)

//METAREAD; out$deq; :rindex;
//METAWRITE; out$deq; :rindex;
//METAGUARD; out$deq; rindex != windex;
//METAREAD; in$enq; :windex;
//METAWRITE; in$enq; windex == 0:element0;windex != 0:element1;:windex;
//METAGUARD; in$enq; ((windex + 1) % 2) != rindex;
//METAREAD; out$first; rindex == 0:element0;rindex != 0:element1;:rindex;
//METABEFORE; out$first; :out$deq
//METAGUARD; out$first; rindex != windex;
//METAEXTERNAL; element; l_struct_OC_ValuePair;
`endif
