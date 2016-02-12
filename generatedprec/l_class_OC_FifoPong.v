
`include "l_class_OC_FifoPong.vh"

module l_class_OC_FifoPong (
    input CLK,
    input nRST,
    input in_enq__ENA,
    input [767:0]in_enq_v,
    output in_enq__RDY,
    input out_deq__ENA,
    output out_deq__RDY,
    output [767:0]out_first,
    output out_first__RDY,
    input [`l_class_OC_FifoPong_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_FifoPong_RULE_COUNT:0]rule_ready);
    wire in_enq__RDY_internal;
    wire in_enq__ENA_internal = in_enq__ENA && in_enq__RDY_internal;
    wire out_deq__RDY_internal;
    wire out_deq__ENA_internal = out_deq__ENA && out_deq__RDY_internal;
    wire [767:0]element1$out_first;
    l_class_OC_Fifo1_OC_4 element1 (
        CLK,
        nRST,
        in_enq__ENA_internal,
        in_enq_v,
        in_enq__RDY_internal,
        out_deq__ENA_internal,
        out_deq__RDY_internal,
        element1$out_first,
        out_first__RDY_internal,
        rule_enable[0:`l_class_OC_Fifo1_OC_4_RULE_COUNT],
        rule_ready[0:`l_class_OC_Fifo1_OC_4_RULE_COUNT]);
    assign in_enq__RDY = in_enq__RDY_internal;
    assign out_deq__RDY = out_deq__RDY_internal;
endmodule 

