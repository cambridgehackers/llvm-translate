interface L_class_OC_LpmMemory;
    method Action memdelay();
    method Action req(Bit#(32) v);
    method Action resAccept();
    method Bit#(32) resValue();
endinterface
import "BVI" l_class_OC_LpmMemory =
module mkL_class_OC_LpmMemory(L_class_OC_LpmMemory);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method memdelay() enable(memdelay__ENA) ready(memdelay__RDY);
    method req(v) enable(req__ENA) ready(req__RDY);
    method resAccept() enable(resAccept__ENA) ready(resAccept__RDY);
    method resValue resValue() ready(resValue__RDY);
    schedule (memdelay, req, resAccept, resValue) CF (memdelay, req, resAccept, resValue);
endmodule
