interface L_class_OC_Echo_KD__KD_respond_KD__KD_respond2;
    method Action ENA();
    method Bit#(32) RDY();
endinterface
import "BVI" l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 =
module mkL_class_OC_Echo_KD__KD_respond_KD__KD_respond2(L_class_OC_Echo_KD__KD_respond_KD__KD_respond2);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method ENA() enable(ENA__ENA) ready(ENA__RDY);
    method RDY RDY() ready(RDY__RDY);
    schedule (ENA, RDY) CF (ENA, RDY);
endmodule
