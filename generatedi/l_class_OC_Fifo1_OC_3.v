
`include "l_class_OC_Fifo1_OC_3.vh"

module l_class_OC_Fifo1_OC_3 (
    input CLK,
    input nRST,
    input in_enq__ENA,
    input [63:0]in_enq_v,
    output in_enq__RDY,
    input out_deq__ENA,
    output out_deq__RDY,
    input out_first__ENA,
    output out_first__RDY,
    input [`l_class_OC_Fifo1_OC_3_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_Fifo1_OC_3_RULE_COUNT:0]rule_ready);
    wire in_enq__RDY_internal;
    wire in_enq__ENA_internal = in_enq__ENA && in_enq__RDY_internal;
    wire out_deq__RDY_internal;
    wire out_deq__ENA_internal = out_deq__ENA && out_deq__RDY_internal;
    wire out_first__RDY_internal;
    wire out_first__ENA_internal = out_first__ENA && out_first__RDY_internal;
    l_struct_OC_ValuePair element (
        CLK,
        nRST,
        rule_enable[0:`l_struct_OC_ValuePair_RULE_COUNT],
        rule_ready[0:`l_struct_OC_ValuePair_RULE_COUNT]);
    reg full;
    assign agg_2e_result$_ = out_first__ENA_internal ? &element : out_first__ENA_internal ? 88 : out_first__ENA_internal ? 4 : 0;
    assign agg_2e_result$__ENA = out_first__ENA_internal;
    assign in_enq__RDY = in_enq__RDY_internal;
    assign in_enq__RDY_internal = full ^ 1;
    assign out_deq__RDY = out_deq__RDY_internal;
    assign out_deq__RDY_internal = full;
    assign out_first__RDY = out_first__RDY_internal;
    assign out_first__RDY_internal = full;

    always @( posedge CLK) begin
      if (!nRST) begin
        full <= 0;
      end // nRST
      else begin
        if (in_enq__ENA_internal) begin
            full <= 1;
        end; // End of in_enq
        if (out_deq__ENA_internal) begin
            full <= 0;
        end; // End of out_deq
      end
    end // always @ (posedge CLK)
endmodule 
