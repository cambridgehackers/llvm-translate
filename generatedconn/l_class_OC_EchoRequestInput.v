
`include "l_class_OC_EchoRequestInput.vh"

module l_class_OC_EchoRequestInput (
    input CLK,
    input nRST,
    input enq__ENA,
    input [95:0]enq_v,
    output enq__RDY,
    input [`l_class_OC_EchoRequestInput_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_EchoRequestInput_RULE_COUNT:0]rule_ready);
    wire enq__RDY_internal;
    wire enq__ENA_internal = enq__ENA && enq__RDY_internal;
    assign enq__RDY = enq__RDY_internal;
    assign enq__RDY_internal = request$say__RDY;
    assign request$say__ENA = enq__ENA_internal & enq_v$tag == 1;
    assign request$say_meth = enq_v$data$say$meth;
    assign request$say_v = enq_v$data$say$v;
endmodule 

