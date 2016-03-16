interface L_class_OC_foo;
    method Action heard(Bit#(32) methBit#(32) v);
endinterface
import "BVI" l_class_OC_foo =
module mkL_class_OC_foo(L_class_OC_foo);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method heard(methv) enable(heard__ENA) ready(heard__RDY);
    schedule (heard) CF (heard);
endmodule
