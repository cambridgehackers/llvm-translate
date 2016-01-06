`ifndef __l_class_OC_FifoPong_VH__
`define __l_class_OC_FifoPong_VH__

`define l_class_OC_FifoPong_RULE_COUNT (0)

//METAREAD; in_enq; :pong ^ 1;pong:;pong;
//METAWRITE; in_enq; :pong;element2:pong ^ 1;element1:;full;
//METAGUARD; in_enq__RDY; full ^ 1;
//METAREAD; out_deq; :;pong;
//METAWRITE; out_deq; :;full:;pong;
//METAGUARD; out_deq__RDY; full;
//METAREAD; out_first; :;pong:pong;element2:pong ^ 1;element1;
//METAGUARD; out_first__RDY; full;
`endif
