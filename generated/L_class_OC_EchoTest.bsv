interface L_class_OC_EchoTest;
    method Action drive__ENA();
endinterface
import "BVI" l_class_OC_EchoTest =
module mkL_class_OC_EchoTest(L_class_OC_EchoTest);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method drive__ENA() enable(drive__ENA__ENA) ready(drive__ENA__RDY);
    schedule (drive__ENA) CF (drive__ENA);
endmodule
