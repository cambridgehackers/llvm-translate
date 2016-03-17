interface L_class_OC_EchoIndicationOutput;
    method Action indication$heard(Bit#(32) indication$heard_methBit#(32) indication$heard_v);
    method Action output_rulee();
    method Action output_ruleo();
endinterface
import "BVI" l_class_OC_EchoIndicationOutput =
module mkL_class_OC_EchoIndicationOutput(L_class_OC_EchoIndicationOutput);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method indication$heard(indication$heard_methindication$heard_v) enable(indication$heard__ENA) ready(indication$heard__RDY);
    method output_rulee() enable(output_rulee__ENA) ready(output_rulee__RDY);
    method output_ruleo() enable(output_ruleo__ENA) ready(output_ruleo__RDY);
    schedule (indication$heard, output_rulee, output_ruleo) CF (indication$heard, output_rulee, output_ruleo);
endmodule
