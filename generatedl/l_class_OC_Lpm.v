
`include "l_class_OC_Lpm.vh"

module l_class_OC_Lpm (
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
    input [`l_class_OC_Lpm_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_Lpm_RULE_COUNT:0]rule_ready);
    wire respond__RDY_internal;
    wire respond__ENA_internal = rule_enable[0] && respond__RDY_internal;
    wire say__RDY_internal;
    wire say__ENA_internal = say__ENA && say__RDY_internal;
    wire fifo$out$deq__ENA;
    wire fifo$out$deq__RDY;
    wire fifo$in$enq__ENA;
    wire [703:0]fifo$in$enq_v;
    wire fifo$in$enq__RDY;
    wire [703:0]fifo$out$first;
    wire fifo$out$first__RDY;
    l_class_OC_Fifo1_OC_0 fifo (
        CLK,
        nRST,
        fifo$out$deq__ENA,
        fifo$out$deq__RDY,
        fifo$in$enq__ENA,
        fifo$in$enq_v,
        fifo$in$enq__RDY,
        fifo$out$first,
        fifo$out$first__RDY,
        rule_enable[1:`l_class_OC_Fifo1_OC_0_RULE_COUNT],
        rule_ready[1:`l_class_OC_Fifo1_OC_0_RULE_COUNT]);
    assign fifo8$in$enq__ENA = say__ENA_internal;
    assign fifo8$in$enq_v = temp;
    assign fifo8$out$deq__ENA = respond__ENA_internal;
    assign indication$heard__ENA = respond__ENA_internal;
    assign indication$heard_meth = fifo8$out$first$a;
    assign indication$heard_v = fifo8$out$first$b;
    assign respond__RDY_internal = (fifo8$out$first__RDY & fifo8$out$deq__RDY) & indication$heard__RDY;
    assign rule_ready[0] = respond__RDY_internal;
    assign say__RDY = say__RDY_internal;
    assign say__RDY_internal = fifo8$in$enq__RDY;
    assign temp$a = say_meth;
    assign temp$b = say_v;
endmodule 

