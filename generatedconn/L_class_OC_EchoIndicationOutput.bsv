interface L_class_OC_EchoIndicationOutput;
    method Action heard(Bit#(32) heard_methBit#(32) heard_v);
endinterface
import "BVI" l_class_OC_EchoIndicationOutput =
module mkL_class_OC_EchoIndicationOutput(L_class_OC_EchoIndicationOutput);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method heard(heard_methheard_v) enable(heard__ENA) ready(heard__RDY);
    schedule (heard) CF (heard);
endmodule
