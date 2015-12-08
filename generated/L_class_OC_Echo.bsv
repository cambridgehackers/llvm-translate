interface L_class_OC_Echo;
    method Action echoReq(Bit#(32) v);
    method Action rule_respond();
endinterface
import "BVI" l_class_OC_Echo =
module mkL_class_OC_Echo(L_class_OC_Echo);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method echoReq(echoReq_v) enable(echoReq__ENA) ready(echoReq__RDY);
    method rule_respond() enable(rule_respond__ENA) ready(rule_respond__RDY);
    schedule (echoReq, rule_respond) CF (echoReq, rule_respond);
endmodule
