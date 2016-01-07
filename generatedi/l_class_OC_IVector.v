
`include "l_class_OC_IVector.vh"

module l_class_OC_IVector (
    input CLK,
    input nRST,
    input say__ENA,
    input [63:0]say_v,
    output say__RDY,
    output ind$heard__ENA,
    output [63:0]ind$heard_heard_v,
    input ind$heard__RDY,
    input [`l_class_OC_IVector_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_IVector_RULE_COUNT:0]rule_ready);
    wire respond_rule__RDY_internal;
    wire respond_rule__ENA_internal = rule_enable[0] && respond_rule__RDY_internal;
