interface L_class_OC_EchoTest;
endinterface
import "BVI" l_class_OC_EchoTest =
module mkL_class_OC_EchoTest(L_class_OC_EchoTest);
    default_reset rst(nRST);
    default_clock clk(CLK);
    schedule () CF ();
endmodule
