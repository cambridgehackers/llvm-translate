interface L_class_OC_Echo_KD__KD_respond_KD__KD_respond1;
    method Bit#(32) RDY();
    method Action ENA();
endinterface
import "BVI" l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 =
module mkL_class_OC_Echo_KD__KD_respond_KD__KD_respond1(L_class_OC_Echo_KD__KD_respond_KD__KD_respond1);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method RDY RDY() ready(RDY__RDY);
    method ENA() enable(ENA__ENA) ready(ENA__RDY);
    schedule (RDY, ENA) CF (RDY, ENA);
endmodule
