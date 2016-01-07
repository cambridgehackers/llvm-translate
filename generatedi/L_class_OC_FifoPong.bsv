interface L_class_OC_FifoPong;
    method Action in_enq(Bit#(32) in_enq_v);
    method Action out_deq();
    method Action out_first();
endinterface
import "BVI" l_class_OC_FifoPong =
module mkL_class_OC_FifoPong(L_class_OC_FifoPong);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method in_enq(in_enq_v) enable(in_enq__ENA) ready(in_enq__RDY);
    method out_deq() enable(out_deq__ENA) ready(out_deq__RDY);
    method out_first() enable(out_first__ENA) ready(out_first__RDY);
    schedule (in_enq, out_deq, out_first) CF (in_enq, out_deq, out_first);
endmodule
