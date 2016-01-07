
`include "l_class_OC_FifoPong.vh"

module l_class_OC_FifoPong (
    input CLK,
    input nRST,
    input in_enq__ENA,
    input [63:0]in_enq_v,
    output in_enq__RDY,
    input out_deq__ENA,
    output out_deq__RDY,
    input out_first__ENA,
    output out_first__RDY,
    input [`l_class_OC_FifoPong_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_FifoPong_RULE_COUNT:0]rule_ready);
    wire in_enq__RDY_internal;
    wire in_enq__ENA_internal = in_enq__ENA && in_enq__RDY_internal;
    wire out_deq__RDY_internal;
    wire out_deq__ENA_internal = out_deq__ENA && out_deq__RDY_internal;
    wire out_first__RDY_internal;
    wire out_first__ENA_internal = out_first__ENA && out_first__RDY_internal;
    wire element1$in_enq__RDY;
    wire element1$out_deq__RDY;
    wire element1$out_first__RDY;
    l_class_OC_Fifo1_OC_3 element1 (
        CLK,
        nRST,
        in_enq__ENA_internal & pong ^ 1,
        in_enq_v,
        element1$in_enq__RDY,
        out_deq__ENA_internal & pong ^ 1,
        element1$out_deq__RDY,
        out_first__ENA_internal & pong ^ 1,
        element1$out_first__RDY,
        rule_enable[0:`l_class_OC_Fifo1_OC_3_RULE_COUNT],
        rule_ready[0:`l_class_OC_Fifo1_OC_3_RULE_COUNT]);
    wire element2$in_enq__RDY;
    wire element2$out_deq__RDY;
    wire element2$out_first__RDY;
    l_class_OC_Fifo1_OC_3 element2 (
        CLK,
        nRST,
        in_enq__ENA_internal & pong,
        in_enq_v,
        element2$in_enq__RDY,
        out_deq__ENA_internal & pong,
        element2$out_deq__RDY,
        out_first__ENA_internal & pong,
        element2$out_first__RDY,
        rule_enable[0 + `l_class_OC_Fifo1_OC_3_RULE_COUNT:`l_class_OC_Fifo1_OC_3_RULE_COUNT],
        rule_ready[0 + `l_class_OC_Fifo1_OC_3_RULE_COUNT:`l_class_OC_Fifo1_OC_3_RULE_COUNT]);
    reg pong;
    assign in_enq__RDY = in_enq__RDY_internal;
    assign in_enq__RDY_internal = (element2$in_enq__RDY | (pong ^ 1)) & (element1$in_enq__RDY | pong);
    assign out_deq__RDY = out_deq__RDY_internal;
    assign out_deq__RDY_internal = (element2$out_deq__RDY | (pong ^ 1)) & (element1$out_deq__RDY | pong);
    assign out_first__RDY = out_first__RDY_internal;
    assign out_first__RDY_internal = (element2$out_first__RDY | (pong ^ 1)) & (element1$out_first__RDY | pong);

    always @( posedge CLK) begin
      if (!nRST) begin
        pong <= 0;
      end // nRST
      else begin
        if (out_deq__ENA_internal) begin
            pong <= pong ^ 1;
        end; // End of out_deq
      end
    end // always @ (posedge CLK)
endmodule 

