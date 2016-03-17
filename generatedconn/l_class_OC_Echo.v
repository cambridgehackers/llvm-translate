
`include "l_class_OC_Echo.vh"

module l_class_OC_Echo (
    input CLK,
    input nRST,
    input request$say__ENA,
    input [31:0]request$say_meth,
    input [31:0]request$say_v,
    output request$say__RDY,
    output indication$heard__ENA,
    output [31:0]indication$heard_meth,
    output [31:0]indication$heard_v,
    input indication$heard__RDY,
    input [`l_class_OC_Echo_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_Echo_RULE_COUNT:0]rule_ready);
    wire request$say__RDY_internal;
    wire request$say__ENA_internal = request$say__ENA && request$say__RDY_internal;
    assign indication$heard__ENA = request$say__ENA_internal;
    assign indication$heard_meth = say_meth;
    assign indication$heard_v = say_v;
    assign request$say__RDY = request$say__RDY_internal;
    assign request$say__RDY_internal = indication$heard__RDY;
endmodule 

