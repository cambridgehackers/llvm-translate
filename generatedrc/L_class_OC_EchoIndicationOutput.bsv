interface L_class_OC_EchoIndicationOutput;
    method Action heard(Bit#(32) heard_methBit#(32) heard_v);
    method Action output_rulee();
    method Action output_ruleo();
endinterface
import "BVI" l_class_OC_EchoIndicationOutput =
module mkL_class_OC_EchoIndicationOutput(L_class_OC_EchoIndicationOutput);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method heard(heard_methheard_v) enable(heard__ENA) ready(heard__RDY);
    method output_rulee() enable(output_rulee__ENA) ready(output_rulee__RDY);
    method output_ruleo() enable(output_ruleo__ENA) ready(output_ruleo__RDY);
    schedule (heard, output_rulee, output_ruleo) CF (heard, output_rulee, output_ruleo);
endmodule
