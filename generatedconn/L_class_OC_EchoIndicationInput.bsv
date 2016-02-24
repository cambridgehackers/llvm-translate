interface L_class_OC_EchoIndicationInput;
    method Action enq(Bit#(32) enq_v);
endinterface
import "BVI" l_class_OC_EchoIndicationInput =
module mkL_class_OC_EchoIndicationInput(L_class_OC_EchoIndicationInput);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method enq(enq_v) enable(enq__ENA) ready(enq__RDY);
    schedule (enq) CF (enq);
endmodule
