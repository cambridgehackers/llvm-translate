
`include "l_class_OC_IVector.vh"

module l_class_OC_IVector (
    input CLK,
    input nRST,
    input say__ENA,
    input [31:0]say_meth,
    input [31:0]say_v,
    output say__RDY,
    output ind$heard__ENA,
    output [31:0]ind$heard_heard_meth,
    output [31:0]ind$heard_heard_v,
    input ind$heard__RDY,
    input [`l_class_OC_IVector_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_IVector_RULE_COUNT:0]rule_ready);
    wire respond_rule__RDY_internal;
    wire respond_rule__ENA_internal = rule_enable[0] && respond_rule__RDY_internal;
    wire say__RDY_internal;
    wire say__ENA_internal = say__ENA && say__RDY_internal;
    wire fifo$in_enq__ENA;
    wire [703:0]fifo$in_enq_v;
    wire fifo$in_enq__RDY;
    wire fifo$out_deq__ENA;
    wire fifo$out_deq__RDY;
    wire [703:0]fifo$out_first;
    wire fifo$out_first__RDY;
    l_class_OC_FifoPong fifo (
        CLK,
        nRST,
        fifo$in_enq__ENA,
        fifo$in_enq_v,
        fifo$in_enq__RDY,
        fifo$out_deq__ENA,
        fifo$out_deq__RDY,
        fifo$out_first,
        fifo$out_first__RDY,
        rule_enable[1:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[1:`l_class_OC_FifoPong_RULE_COUNT]);
    reg[31:0] vsize;
    assign &fifo[1]$in_enq__ENA = say__ENA_internal;
    assign &fifo[1]$in_enq_v = temp;
    assign &fifo[1]$out_deq__ENA = respond_rule__ENA_internal;
    assign ind$heard__ENA = respond_rule__ENA_internal;
    assign ind$heard_heard_meth = &fifo[1]$out_first$a;
    assign ind$heard_heard_v = &fifo[1]$out_first$b;
    assign respond_rule__RDY_internal = ((&fifo[1]$out_first__RDY) & (&fifo[1]$out_deq__RDY)) & ind$heard__RDY;
    assign rule_ready[0] = respond_rule__RDY_internal;
    assign say__RDY = say__RDY_internal;
    assign say__RDY_internal = &fifo[1]$in_enq__RDY;

    always @( posedge CLK) begin
      if (!nRST) begin
        vsize <= 0;
      end // nRST
      else begin
        if (say__ENA_internal) begin
            temp$a <= say_meth;
            temp$b <= say_v;
        end; // End of say
      end
    end // always @ (posedge CLK)
endmodule 

