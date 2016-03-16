interface L_class_OC_foo;
endinterface
import "BVI" l_class_OC_foo =
module mkL_class_OC_foo(L_class_OC_foo);
    default_reset rst(nRST);
    default_clock clk(CLK);
    schedule () CF ();
endmodule
