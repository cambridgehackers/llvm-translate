`ifndef __l_class_OC_Fifo2_VH__
`define __l_class_OC_Fifo2_VH__

`include "l_struct_OC_ValuePair.vh"
`define l_class_OC_Fifo2_RULE_COUNT (0)

//METAGUARD; out$deq; rindex != windex;
//METAGUARD; in$enq; ((windex + 1) % 2) != rindex;
//METABEFORE; out$first; :out$deq
//METAGUARD; out$first; rindex != windex;
`endif
