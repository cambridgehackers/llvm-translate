
`include "l_class_OC_Echo.vh"

module l_class_OC_Echo (
    input CLK,
    input nRST,
    input say__ENA,
    input [31:0]say_meth,
    input [31:0]say_v,
    output say__RDY,
    output indication$heard__ENA,
    output [31:0]indication$heard_meth,
    output [31:0]indication$heard_v,
    input indication$heard__RDY,
    input [`l_class_OC_Echo_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_Echo_RULE_COUNT:0]rule_ready);
    wire say__RDY_internal;
    wire say__ENA_internal = say__ENA && say__RDY_internal;
    assign indication$heard__ENA = say__ENA_internal;
    assign indication$heard_meth = say_meth;
    assign indication$heard_v = say_v;
    assign say__RDY = say__RDY_internal;
    assign say__RDY_internal = indication$heard__RDY;
endmodule 

