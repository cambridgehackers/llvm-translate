interface L_class_OC_Lpm;
    method Action enter();
    method Action exit();
    method Action recirc();
    method Action respond();
    method Action say(Bit#(32) say_methBit#(32) say_v);
endinterface
import "BVI" l_class_OC_Lpm =
module mkL_class_OC_Lpm(L_class_OC_Lpm);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method enter() enable(enter__ENA) ready(enter__RDY);
    method exit() enable(exit__ENA) ready(exit__RDY);
    method recirc() enable(recirc__ENA) ready(recirc__RDY);
    method respond() enable(respond__ENA) ready(respond__RDY);
    method say(say_methsay_v) enable(say__ENA) ready(say__RDY);
    schedule (enter, exit, recirc, respond, say) CF (enter, exit, recirc, respond, say);
endmodule
