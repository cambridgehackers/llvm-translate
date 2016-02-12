interface L_class_OC_IVector;
    method Action respond0();
    method Action respond1();
    method Action respond2();
    method Action respond3();
    method Action respond4();
    method Action respond5();
    method Action respond6();
    method Action respond7();
    method Action respond8();
    method Action respond9();
    method Action say(Bit#(32) say_methBit#(32) say_v.coerce);
endinterface
import "BVI" l_class_OC_IVector =
module mkL_class_OC_IVector(L_class_OC_IVector);
    default_reset rst(nRST);
    default_clock clk(CLK);
    method respond0() enable(respond0__ENA) ready(respond0__RDY);
    method respond1() enable(respond1__ENA) ready(respond1__RDY);
    method respond2() enable(respond2__ENA) ready(respond2__RDY);
    method respond3() enable(respond3__ENA) ready(respond3__RDY);
    method respond4() enable(respond4__ENA) ready(respond4__RDY);
    method respond5() enable(respond5__ENA) ready(respond5__RDY);
    method respond6() enable(respond6__ENA) ready(respond6__RDY);
    method respond7() enable(respond7__ENA) ready(respond7__RDY);
    method respond8() enable(respond8__ENA) ready(respond8__RDY);
    method respond9() enable(respond9__ENA) ready(respond9__RDY);
    method say(say_methsay_v.coerce) enable(say__ENA) ready(say__RDY);
    schedule (respond0, respond1, respond2, respond3, respond4, respond5, respond6, respond7, respond8, respond9, say) CF (respond0, respond1, respond2, respond3, respond4, respond5, respond6, respond7, respond8, respond9, say);
endmodule
