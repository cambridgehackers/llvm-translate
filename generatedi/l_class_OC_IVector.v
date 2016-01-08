
`include "l_class_OC_IVector.vh"

module l_class_OC_IVector (
    input CLK,
    input nRST,
    input say__ENA,
    input [703:0]say_v,
    output say__RDY,
    output ind$heard__ENA,
    output [703:0]ind$heard_heard_v,
    input ind$heard__RDY,
    input [`l_class_OC_IVector_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_IVector_RULE_COUNT:0]rule_ready);
    wire respond_rule__RDY_internal;
    wire respond_rule__ENA_internal = rule_enable[0] && respond_rule__RDY_internal;
    wire say__RDY_internal;
    wire say__ENA_internal = say__ENA && say__RDY_internal;
    wire fifo$out_deq__RDY;
    wire fifo$out_first__RDY;
    l_class_OC_FifoPong fifo (
        CLK,
        nRST,
        say__ENA_internal,
        say_v,
        say__RDY_internal,
        respond_rule__ENA_internal,
        fifo$out_deq__RDY,
        ind$heard_heard_v,
        fifo$out_first__RDY,
        rule_enable[1:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[1:`l_class_OC_FifoPong_RULE_COUNT]);
    assign ind$heard__ENA = respond_rule__ENA_internal;
    assign respond_rule__RDY_internal = (fifo$out_first__RDY & fifo$out_deq__RDY) & ind$heard__RDY;
    assign rule_ready[0] = respond_rule__RDY_internal;
    assign say__RDY = say__RDY_internal;
endmodule 

