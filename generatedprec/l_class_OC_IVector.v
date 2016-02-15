
`include "l_class_OC_IVector.vh"

module l_class_OC_IVector (
    input CLK,
    input nRST,
    input say__ENA,
    input [127:0]say_meth,
    input [127:0]say_v,
    output say__RDY,
    output ind$heard__ENA,
    output [127:0]ind$heard_heard_meth,
    output [127:0]ind$heard_heard_v,
    input ind$heard__RDY,
    input [`l_class_OC_IVector_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_IVector_RULE_COUNT:0]rule_ready);
    wire respond__RDY_internal;
    wire respond__ENA_internal = rule_enable[0] && respond__RDY_internal;
    wire say__RDY_internal;
    wire say__ENA_internal = say__ENA && say__RDY_internal;
    wire fifo$out_deq__RDY;
    wire [255:0]fifo$out_first;
    wire fifo$out_first__RDY;
    l_class_OC_Fifo_OC_1 fifo (
        CLK,
        nRST,
        say__ENA_internal,
        temp,
        say__RDY_internal,
        respond__ENA_internal,
        fifo$out_deq__RDY,
        fifo$out_first,
        fifo$out_first__RDY,
        rule_enable[1:`l_class_OC_Fifo_OC_1_RULE_COUNT],
        rule_ready[1:`l_class_OC_Fifo_OC_1_RULE_COUNT]);
    wire [`l_class_OC_FixedPointV_RULE_COUNT:0] counter$rule_ready;
    l_class_OC_FixedPointV counter (
        CLK,
        nRST,
        counter$  reg data,
        rule_enable[1 + `l_class_OC_Fifo_OC_1_RULE_COUNT:`l_class_OC_FixedPointV_RULE_COUNT],
        counter$rule_ready);
    l_class_OC_FixedPointV gcounter (
        CLK,
        nRST,
        gcounter$  reg data,
        rule_enable[1 + `l_class_OC_Fifo_OC_1_RULE_COUNT + `l_class_OC_FixedPointV_RULE_COUNT:`l_class_OC_FixedPointV_RULE_COUNT],
        rule_ready[1 + `l_class_OC_Fifo_OC_1_RULE_COUNT + `l_class_OC_FixedPointV_RULE_COUNT:`l_class_OC_FixedPointV_RULE_COUNT]);
    assign agg_2e_tmp$~FixedPoint__ENA = respond__ENA_internal;
    assign agg_2e_tmp3$~FixedPoint__ENA = respond__ENA_internal;
    assign fifo$out_first$__ENA = respond__ENA_internal;
    assign ind$heard__ENA = respond__ENA_internal;
    assign ind$heard_heard_meth = agg_2e_tmp;
    assign ind$heard_heard_v = agg_2e_tmp3;
    assign respond__RDY_internal = (fifo$out_first__RDY & fifo$out_deq__RDY) & ind$heard__RDY;
    assign rule_ready[0] = respond__RDY_internal;
    assign rule_ready[1 + `l_class_OC_Fifo_OC_1_RULE_COUNT:`l_class_OC_FixedPointV_RULE_COUNT] = counter$rule_ready;
    assign say__RDY = say__RDY_internal;
    assign temp$__ENA = say__ENA_internal || say__ENA_internal || say__ENA_internal;

    always @( posedge CLK) begin
      if (!nRST) begin
      end // nRST
      else begin
        if (respond__ENA_internal) begin
            agg_2e_tmp <= fifo$out_first$a;
            agg_2e_tmp3 <= fifo$out_first$b;
        end; // End of respond
        if (say__ENA_internal) begin
            temp$a <= say_meth;
            temp$b <= say_v;
        end; // End of say
      end
    end // always @ (posedge CLK)
endmodule 

