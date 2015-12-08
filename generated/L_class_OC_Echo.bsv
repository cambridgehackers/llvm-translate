interface L_class_OC_Echo;
    method Action echoReq(Bit#(32) v);
    method Action rule_respondexport();
    method Action rule_respond();
endinterface
import "BVI" l_class_OC_Echo =
module mkL_class_OC_Echo(L_class_OC_Echo);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method echoReq(echoReq_v) enable(echoReq__ENA) ready(echoReq__RDY);
    method rule_respondexport() enable(rule_respondexport__ENA) ready(rule_respondexport__RDY);
    method rule_respond() enable(rule_respond__ENA) ready(rule_respond__RDY);
    schedule (echoReq, rule_respondexport, rule_respond) CF (echoReq, rule_respondexport, rule_respond);
endmodule
