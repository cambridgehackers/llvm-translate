interface L_class_OC_EchoIndicationInput;
    method Action pipe$enq(Bit#(32) pipe$enq_v);
endinterface
import "BVI" l_class_OC_EchoIndicationInput =
module mkL_class_OC_EchoIndicationInput(L_class_OC_EchoIndicationInput);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method pipe$enq(pipe$enq_v) enable(pipe$enq__ENA) ready(pipe$enq__RDY);
    schedule (pipe$enq) CF (pipe$enq);
endmodule
