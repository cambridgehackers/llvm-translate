interface L_class_OC_EchoRequestInput;
    method Action pipe_enq(Bit#(32) pipe_enq_v);
endinterface
import "BVI" l_class_OC_EchoRequestInput =
module mkL_class_OC_EchoRequestInput(L_class_OC_EchoRequestInput);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method pipe_enq(pipe_enq_v) enable(pipe_enq__ENA) ready(pipe_enq__RDY);
    schedule (pipe_enq) CF (pipe_enq);
endmodule
