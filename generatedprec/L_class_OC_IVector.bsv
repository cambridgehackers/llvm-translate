interface L_class_OC_IVector;
    method Action respond();
    method Action say(Bit#(32) say_methBit#(32) say_v);
endinterface
import "BVI" l_class_OC_IVector =
module mkL_class_OC_IVector(L_class_OC_IVector);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method respond() enable(respond__ENA) ready(respond__RDY);
    method say(say_methsay_v) enable(say__ENA) ready(say__RDY);
    schedule (respond, say) CF (respond, say);
endmodule