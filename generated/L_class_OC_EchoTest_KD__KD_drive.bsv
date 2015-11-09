interface L_class_OC_EchoTest_KD__KD_drive;
    method Bit#(32) RDY();
    method Action ENA();
endinterface
import "BVI" l_class_OC_EchoTest_KD__KD_drive =
module mkL_class_OC_EchoTest_KD__KD_drive(L_class_OC_EchoTest_KD__KD_drive);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method RDY RDY() ready(RDY__RDY);
    method ENA() enable(ENA__ENA) ready(ENA__RDY);
    schedule (RDY, ENA) CF (RDY, ENA);
endmodule
