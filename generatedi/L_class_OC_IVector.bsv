interface L_class_OC_IVector;
    method Action respond_rule();
    method Action say(Bit#(32) say_v);
endinterface
import "BVI" l_class_OC_IVector =
module mkL_class_OC_IVector(L_class_OC_IVector);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method respond_rule() enable(respond_rule__ENA) ready(respond_rule__RDY);
    method say(say_v) enable(say__ENA) ready(say__RDY);
    schedule (respond_rule, say) CF (respond_rule, say);
endmodule
