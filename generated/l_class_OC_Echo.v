
`include "l_class_OC_Echo.vh"

module l_class_OC_Echo (
    input CLK,
    input nRST,
    input respond_rule__ENA,
    output respond_rule__RDY,
    input say__ENA,
    input [31:0]say_v,
    output say__RDY,
    output ind$heard__ENA,
    output [31:0]ind$heard_heard_v,
    input ind$heard__RDY,
    input [`l_class_OC_Echo_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_Echo_RULE_COUNT:0]rule_ready);
    wire respond_rule__RDY_internal;
    wire respond_rule__ENA_internal = respond_rule__ENA && respond_rule__RDY_internal;
    assign respond_rule__RDY = respond_rule__RDY_internal;
    wire say__RDY_internal;
    wire say__ENA_internal = say__ENA && say__RDY_internal;
    assign say__RDY = say__RDY_internal;
    wire fifo$out_deq__RDY;
    wire fifo$out_first__RDY;
    wire [`l_class_OC_Fifo1_RULE_COUNT:0] fifo$rule_enable;
    wire [`l_class_OC_Fifo1_RULE_COUNT:0] fifo$rule_ready;
    l_class_OC_Fifo1 fifo (
        CLK,
        nRST,
        say__ENA_internal,
        say_v,
        say__RDY,
        respond_rule__ENA_internal,
        fifo$out_deq__RDY,
        ind$heard_heard_v,
        fifo$out_first__RDY,
        fifo$rule_enable,
        fifo$rule_ready);
    reg[31:0] pipetemp;
    assign ind$heard__ENA = respond_rule__ENA_internal;
    assign respond_rule__RDY_internal = (fifo$out_first__RDY & fifo$out_deq__RDY) & ind$heard__RDY;
    assign {fifo$rule_enable, respond_rule_ENA} = rule_enable;
    assign rule_ready = {fifo$rule_ready, respond_rule__RDY};

    always @( posedge CLK) begin
      if (!nRST) begin
        pipetemp <= 0;
      end // nRST
    end // always @ (posedge CLK)
endmodule 

