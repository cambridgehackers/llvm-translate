interface L_class_OC_EchoIndicationInput;
    method Action enq(Bit#(32) enq_v);
    method Action input_rule();
endinterface
import "BVI" l_class_OC_EchoIndicationInput =
module mkL_class_OC_EchoIndicationInput(L_class_OC_EchoIndicationInput);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method enq(enq_v) enable(enq__ENA) ready(enq__RDY);
    method input_rule() enable(input_rule__ENA) ready(input_rule__RDY);
    schedule (enq, input_rule) CF (enq, input_rule);
endmodule
