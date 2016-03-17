interface L_class_OC_FifoPong;
    method Action out$deq();
    method Action in$enq(Bit#(32) in$enq_v);
    method Bit#(32) out$first();
endinterface
import "BVI" l_class_OC_FifoPong =
module mkL_class_OC_FifoPong(L_class_OC_FifoPong);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method out$deq() enable(out$deq__ENA) ready(out$deq__RDY);
    method in$enq(in$enq_v) enable(in$enq__ENA) ready(in$enq__RDY);
    method out$first out$first() ready(out$first__RDY);
    schedule (out$deq, in$enq, out$first) CF (out$deq, in$enq, out$first);
endmodule
