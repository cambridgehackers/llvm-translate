interface L_class_OC_Echo;
    method Action request$say(Bit#(32) request$say_methBit#(32) request$say_v);
endinterface
import "BVI" l_class_OC_Echo =
module mkL_class_OC_Echo(L_class_OC_Echo);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method request$say(request$say_methrequest$say_v) enable(request$say__ENA) ready(request$say__RDY);
    schedule (request$say) CF (request$say);
endmodule
