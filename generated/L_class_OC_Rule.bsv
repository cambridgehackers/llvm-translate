interface L_class_OC_Rule;
endinterface
import "BVI" l_class_OC_Rule =
module mkL_class_OC_Rule(L_class_OC_Rule);
    default_reset rst(nRST);
    default_clock clk(CLK);
    schedule () CF ();
endmodule
