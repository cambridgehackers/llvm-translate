interface L_class_OC_EchoIndicationInput;
    method Action pipe_enq(Bit#(32) pipe_enq_v);
endinterface
import "BVI" l_class_OC_EchoIndicationInput =
module mkL_class_OC_EchoIndicationInput(L_class_OC_EchoIndicationInput);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method pipe_enq(pipe_enq_v) enable(pipe_enq__ENA) ready(pipe_enq__RDY);
    schedule (pipe_enq) CF (pipe_enq);
endmodule
