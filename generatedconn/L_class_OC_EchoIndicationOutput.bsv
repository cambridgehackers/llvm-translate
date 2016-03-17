interface L_class_OC_EchoIndicationOutput;
    method Action indication$heard(Bit#(32) indication$heard_methBit#(32) indication$heard_v);
endinterface
import "BVI" l_class_OC_EchoIndicationOutput =
module mkL_class_OC_EchoIndicationOutput(L_class_OC_EchoIndicationOutput);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method indication$heard(indication$heard_methindication$heard_v) enable(indication$heard__ENA) ready(indication$heard__RDY);
    schedule (indication$heard) CF (indication$heard);
endmodule
