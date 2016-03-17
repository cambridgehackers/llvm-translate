interface L_class_OC_Echo;
    method Action delay_rule();
    method Action respond_rule();
    method Action request$say(Bit#(32) request$say_methBit#(32) request$say_v);
endinterface
import "BVI" l_class_OC_Echo =
module mkL_class_OC_Echo(L_class_OC_Echo);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method delay_rule() enable(delay_rule__ENA) ready(delay_rule__RDY);
    method respond_rule() enable(respond_rule__ENA) ready(respond_rule__RDY);
    method request$say(request$say_methrequest$say_v) enable(request$say__ENA) ready(request$say__RDY);
    schedule (delay_rule, respond_rule, request$say) CF (delay_rule, respond_rule, request$say);
endmodule
