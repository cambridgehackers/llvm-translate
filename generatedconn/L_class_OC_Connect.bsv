interface L_class_OC_Connect;
endinterface
import "BVI" l_class_OC_Connect =
module mkL_class_OC_Connect(L_class_OC_Connect);
    default_reset rst(nRST);
    default_clock clk(CLK);
    schedule () CF ();
endmodule
