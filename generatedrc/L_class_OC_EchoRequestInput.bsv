interface L_class_OC_EchoRequestInput;
    method Action pipe$enq(Bit#(32) pipe$enq_v);
endinterface
import "BVI" l_class_OC_EchoRequestInput =
module mkL_class_OC_EchoRequestInput(L_class_OC_EchoRequestInput);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method pipe$enq(pipe$enq_v) enable(pipe$enq__ENA) ready(pipe$enq__RDY);
    schedule (pipe$enq) CF (pipe$enq);
endmodule
