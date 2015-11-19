interface L_class_OC_Echo;
    method Action respond__ENA();
endinterface
import "BVI" l_class_OC_Echo =
module mkL_class_OC_Echo(L_class_OC_Echo);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method respond__ENA() enable(respond__ENA__ENA) ready(respond__ENA__RDY);
    schedule (respond__ENA) CF (respond__ENA);
endmodule
