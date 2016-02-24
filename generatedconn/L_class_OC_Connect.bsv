interface L_class_OC_Connect;
    method Action say(Bit#(32) say_methBit#(32) say_v);
endinterface
import "BVI" l_class_OC_Connect =
module mkL_class_OC_Connect(L_class_OC_Connect);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method say(say_methsay_v) enable(say__ENA) ready(say__RDY);
    schedule (say) CF (say);
endmodule
