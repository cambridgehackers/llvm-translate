interface L_class_OC_EchoRequestInput;
    method Action enq(Bit#(32) enq_v);
endinterface
import "BVI" l_class_OC_EchoRequestInput =
module mkL_class_OC_EchoRequestInput(L_class_OC_EchoRequestInput);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method enq(enq_v) enable(enq__ENA) ready(enq__RDY);
    schedule (enq) CF (enq);
endmodule
