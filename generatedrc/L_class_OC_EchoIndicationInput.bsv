interface L_class_OC_EchoIndicationInput;
    method Action pipe$enq(Bit#(32) pipe$enq_v);
    method Action input_rule();
endinterface
import "BVI" l_class_OC_EchoIndicationInput =
module mkL_class_OC_EchoIndicationInput(L_class_OC_EchoIndicationInput);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method pipe$enq(pipe$enq_v) enable(pipe$enq__ENA) ready(pipe$enq__RDY);
    method input_rule() enable(input_rule__ENA) ready(input_rule__RDY);
    schedule (pipe$enq, input_rule) CF (pipe$enq, input_rule);
endmodule
