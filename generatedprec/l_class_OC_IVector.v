
`include "l_class_OC_IVector.vh"

module l_class_OC_IVector (
    input CLK,
    input nRST,
    input say__ENA,
    input say_meth.coerce,
    input say_v.coerce,
    output say__RDY,
    output ind$heard__ENA,
    output ind$heard_heard_meth.coerce,
    output ind$heard_heard_v.coerce,
    input ind$heard__RDY,
    input [`l_class_OC_IVector_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_IVector_RULE_COUNT:0]rule_ready);
    wire respond__RDY_internal;
    wire respond__ENA_internal = rule_enable[0] && respond__RDY_internal;
    wire say__RDY_internal;
    wire say__ENA_internal = say__ENA && say__RDY_internal;
    wire fifo$out_deq__RDY;
    wire [127:0]fifo$out_first;
    wire fifo$out_first__RDY;
    l_class_OC_Fifo_OC_1 fifo (
        CLK,
        nRST,
        say__ENA_internal,
        *((temp)$),
        *((temp)$),
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
    assign ind$heard__ENA = respond__ENA_internal;
    assign ind$heard_heard_meth.coerce = *(agg_2e_tmp$data);
    assign ind$heard_heard_v.coerce = *(agg_2e_tmp3$data);
    assign respond__RDY_internal = (fifo$out_first__RDY & fifo$out_deq__RDY) & ind$heard__RDY;
    assign rule_ready[0] = respond__RDY_internal;
    assign rule_ready[1 + `l_class_OC_Fifo_OC_1_RULE_COUNT:`l_class_OC_FixedPointV_RULE_COUNT] = counter$rule_ready;
    assign say__RDY = say__RDY_internal;
    assign temp$__ENA = say__ENA_internal;

    always @( posedge CLK) begin
      if (!nRST) begin
      end // nRST
      else begin
        if (respond__ENA_internal) begin
            (temp)$ <= ;
            agg_2e_tmp <= temp$a;
            agg_2e_tmp3 <= temp$b;
        end; // End of respond
        if (say__ENA_internal) begin
            meth$data <= say_meth_2e_coerce;
            temp$a <= meth;
            temp$b <= v;
            v$data <= say_v_2e_coerce;
        end; // End of say
      end
    end // always @ (posedge CLK)
endmodule 

