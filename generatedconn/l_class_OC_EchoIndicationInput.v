
`include "l_class_OC_EchoIndicationInput.vh"

module l_class_OC_EchoIndicationInput (
    input CLK,
    input nRST,
    input pipe$enq__ENA,
    input [95:0]pipe$enq_v,
    output pipe$enq__RDY,
    output indication$heard__ENA,
    output [31:0]indication$heard_meth,
    output [31:0]indication$heard_v,
    input indication$heard__RDY,
    input [`l_class_OC_EchoIndicationInput_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_EchoIndicationInput_RULE_COUNT:0]rule_ready);
    wire pipe$enq__RDY_internal;
    wire pipe$enq__ENA_internal = pipe$enq__ENA && pipe$enq__RDY_internal;
    assign indication$heard__ENA = pipe$enq__ENA_internal & enq_v$tag == 1;
    assign indication$heard_meth = enq_v$data$heard$meth;
    assign indication$heard_v = enq_v$data$heard$v;
    assign pipe$enq__RDY = pipe$enq__RDY_internal;
    assign pipe$enq__RDY_internal = indication$heard__RDY;
endmodule 

