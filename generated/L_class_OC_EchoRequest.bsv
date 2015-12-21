interface L_class_OC_EchoRequest;
endinterface
import "BVI" l_class_OC_EchoRequest =
module mkL_class_OC_EchoRequest(L_class_OC_EchoRequest);
    default_reset rst(nRST);
    default_clock clk(CLK);
    schedule () CF ();
endmodule
