
`include "l_class_OC_Echo.vh"

module l_class_OC_Echo (
    input CLK,
    input nRST,
    input say__ENA,
    input [31:0]say_v,
    output say__RDY,
    output ind$heard__ENA,
    output [31:0]ind$heard_heard_v,
    input ind$heard__RDY,
    input [`l_class_OC_Echo_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_Echo_RULE_COUNT:0]rule_ready);
    wire respond_rule__RDY_internal;
    wire respond_rule__ENA_internal = rule_enable[0] && respond_rule__RDY_internal;
    wire say__RDY_internal;
    wire say__ENA_internal = say__ENA && say__RDY_internal;
    wire fifo$in_enq__ENA;
    wire [31:0]fifo$in_enq_v;
    wire fifo$in_enq__RDY;
    wire fifo$out_deq__ENA;
    wire fifo$out_deq__RDY;
    wire [31:0]fifo$out_first;
    wire fifo$out_first__RDY;
    l_class_OC_Fifo1 fifo (
        CLK,
        nRST,
        fifo$in_enq__ENA,
        fifo$in_enq_v,
        fifo$in_enq__RDY,
        fifo$out_deq__ENA,
        fifo$out_deq__RDY,
        fifo$out_first,
        fifo$out_first__RDY,
        rule_enable[1:`l_class_OC_Fifo1_RULE_COUNT],
        rule_ready[1:`l_class_OC_Fifo1_RULE_COUNT]);
    reg[31:0] pipetemp;
    assign fifo$in$enq__ENA = say__ENA_internal;
    assign fifo$in$enq_v = say_v;
    assign fifo$out$deq__ENA = respond_rule__ENA_internal;
    assign ind$heard__ENA = respond_rule__ENA_internal;
    assign ind$heard_heard_v = fifo$out$first;
    assign respond_rule__RDY_internal = (fifo$out$first__RDY & fifo$out$deq__RDY) & ind$heard__RDY;
    assign rule_ready[0] = respond_rule__RDY_internal;
    assign say__RDY = say__RDY_internal;
    assign say__RDY_internal = fifo$in$enq__RDY;

    always @( posedge CLK) begin
      if (!nRST) begin
        pipetemp <= 0;
      end // nRST
    end // always @ (posedge CLK)
endmodule 

