interface L_class_OC_Echo;
    method Action say(Bit#(32) say_methBit#(32) say_v);
endinterface
import "BVI" l_class_OC_Echo =
module mkL_class_OC_Echo(L_class_OC_Echo);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method say(say_methsay_v) enable(say__ENA) ready(say__RDY);
    schedule (say) CF (say);
endmodule
