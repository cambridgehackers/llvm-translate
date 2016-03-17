
`include "l_class_OC_FifoPong.vh"

module l_class_OC_FifoPong (
    input CLK,
    input nRST,
    input out$deq__ENA,
    output out$deq__RDY,
    input in$enq__ENA,
    input [703:0]in$enq_v,
    output in$enq__RDY,
    output [703:0]out$first,
    output out$first__RDY,
    input [`l_class_OC_FifoPong_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_FifoPong_RULE_COUNT:0]rule_ready);
    wire out$deq__RDY_internal;
    wire out$deq__ENA_internal = out$deq__ENA && out$deq__RDY_internal;
    wire in$enq__RDY_internal;
    wire in$enq__ENA_internal = in$enq__ENA && in$enq__RDY_internal;
    wire element1$out$deq__ENA;
    wire element1$out$deq__RDY;
    wire element1$in$enq__ENA;
    wire [703:0]element1$in$enq_v;
    wire element1$in$enq__RDY;
    wire [703:0]element1$out$first;
    wire element1$out$first__RDY;
    l_class_OC_Fifo1_OC_3 element1 (
        CLK,
        nRST,
        element1$out$deq__ENA,
        element1$out$deq__RDY,
        element1$in$enq__ENA,
        element1$in$enq_v,
        element1$in$enq__RDY,
        element1$out$first,
        element1$out$first__RDY,
        rule_enable[0:`l_class_OC_Fifo1_OC_3_RULE_COUNT],
        rule_ready[0:`l_class_OC_Fifo1_OC_3_RULE_COUNT]);
    wire element2$out$deq__ENA;
    wire element2$out$deq__RDY;
    wire element2$in$enq__ENA;
    wire [703:0]element2$in$enq_v;
    wire element2$in$enq__RDY;
    wire [703:0]element2$out$first;
    wire element2$out$first__RDY;
    l_class_OC_Fifo1_OC_3 element2 (
        CLK,
        nRST,
        element2$out$deq__ENA,
        element2$out$deq__RDY,
        element2$in$enq__ENA,
        element2$in$enq_v,
        element2$in$enq__RDY,
        element2$out$first,
        element2$out$first__RDY,
        rule_enable[0 + `l_class_OC_Fifo1_OC_3_RULE_COUNT:`l_class_OC_Fifo1_OC_3_RULE_COUNT],
        rule_ready[0 + `l_class_OC_Fifo1_OC_3_RULE_COUNT:`l_class_OC_Fifo1_OC_3_RULE_COUNT]);
    reg pong;
    assign element18$in$enq__ENA = in$enq__ENA_internal & pong ^ 1;
    assign element18$out$deq__ENA = out$deq__ENA_internal & pong ^ 1;
    assign element28$in$enq__ENA = in$enq__ENA_internal & pong;
    assign element28$in$enq_v = element18$in$enq_v;
    assign element28$out$deq__ENA = out$deq__ENA_internal & pong;
    assign in$enq__RDY = in$enq__RDY_internal;
    assign in$enq__RDY_internal = (element28$in$enq__RDY | (pong ^ 1)) & (element18$in$enq__RDY | pong);
    assign out$deq__RDY = out$deq__RDY_internal;
    assign out$deq__RDY_internal = (element28$out$deq__RDY | (pong ^ 1)) & (element18$out$deq__RDY | pong);
    assign out$first__RDY_internal = (element28$out$first__RDY | (pong ^ 1)) & (element18$out$first__RDY | pong);

    always @( posedge CLK) begin
      if (!nRST) begin
        pong <= 0;
      end // nRST
      else begin
        if (out$deq__ENA_internal) begin
            pong <= pong ^ 1;
        end; // End of out$deq
      end
    end // always @ (posedge CLK)
endmodule 

