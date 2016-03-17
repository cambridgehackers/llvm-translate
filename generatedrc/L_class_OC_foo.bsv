interface L_class_OC_foo;
    method Action indication$heard(Bit#(32) indication$methBit#(32) indication$v);
endinterface
import "BVI" l_class_OC_foo =
module mkL_class_OC_foo(L_class_OC_foo);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method indication$heard(indication$methindication$v) enable(indication$heard__ENA) ready(indication$heard__RDY);
    schedule (indication$heard) CF (indication$heard);
endmodule
