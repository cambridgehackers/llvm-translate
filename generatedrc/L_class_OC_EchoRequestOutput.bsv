interface L_class_OC_EchoRequestOutput;
    method Action request$say(Bit#(32) request$say_methBit#(32) request$say_v);
    method Action request$say2(Bit#(32) request$say2_methBit#(32) request$say2_v);
endinterface
import "BVI" l_class_OC_EchoRequestOutput =
module mkL_class_OC_EchoRequestOutput(L_class_OC_EchoRequestOutput);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method request$say(request$say_methrequest$say_v) enable(request$say__ENA) ready(request$say__RDY);
    method request$say2(request$say2_methrequest$say2_v) enable(request$say2__ENA) ready(request$say2__RDY);
    schedule (request$say, request$say2) CF (request$say, request$say2);
endmodule
