interface L_class_OC_Fifo1;
    method Action enq(Bit#(32) v);
    method Action deq();
    method Bit#(32) first();
    method Bit#(32) notEmpty();
    method Bit#(32) notFull();
endinterface
import "BVI" l_class_OC_Fifo1 =
module mkL_class_OC_Fifo1(L_class_OC_Fifo1);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method enq(enq_v) enable(enq__ENA) ready(enq__RDY);
    method deq() enable(deq__ENA) ready(deq__RDY);
    method first first() ready(first__RDY);
    method notEmpty notEmpty() ready(notEmpty__RDY);
    method notFull notFull() ready(notFull__RDY);
    schedule (enq, deq, first, notEmpty, notFull) CF (enq, deq, first, notEmpty, notFull);
endmodule
