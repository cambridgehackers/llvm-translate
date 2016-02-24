
`include "l_class_OC_EchoIndicationInput.vh"

module l_class_OC_EchoIndicationInput (
    input CLK,
    input nRST,
    input enq__ENA,
    input [95:0]enq_v,
    output enq__RDY,
    input [`l_class_OC_EchoIndicationInput_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_EchoIndicationInput_RULE_COUNT:0]rule_ready);
    wire enq__RDY_internal;
    wire enq__ENA_internal = enq__ENA && enq__RDY_internal;
    assign enq__RDY = enq__RDY_internal;
    assign enq__RDY_internal = request$heard__RDY;
    assign request$heard__ENA = enq__ENA_internal & enq_v$tag == 1;
    assign request$heard_meth = enq_v$data$heard$meth;
    assign request$heard_v = enq_v$data$heard$v;
endmodule 

