
`include "l_class_OC_IVector.vh"

module l_class_OC_IVector (
    input CLK,
    input nRST,
    input say__ENA,
    input [63:0]say_meth.coerce,
    input [63:0]say_v.coerce,
    output say__RDY,
    output ind$heard__ENA,
    output [63:0]ind$heard_heard_meth.coerce,
    output [63:0]ind$heard_heard_v.coerce,
    input ind$heard__RDY,
    input [`l_class_OC_IVector_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_IVector_RULE_COUNT:0]rule_ready);
    wire respond__RDY_internal;
    wire respond__ENA_internal = rule_enable[0] && respond__RDY_internal;
    wire say__RDY_internal;
    wire say__ENA_internal = say__ENA && say__RDY_internal;
    wire fifo$out_deq__RDY;
    wire [767:0]fifo$out_first;
    wire fifo$out_first__RDY;
    l_class_OC_Fifo_OC_0 fifo (
        CLK,
        nRST,
        say__ENA_internal,
        temp,
        say__RDY_internal,
        respond__ENA_internal,
        fifo$out_deq__RDY,
        fifo$out_first,
        fifo$out_first__RDY,
        rule_enable[1:`l_class_OC_Fifo_OC_0_RULE_COUNT],
        rule_ready[1:`l_class_OC_Fifo_OC_0_RULE_COUNT]);
    reg[31:0] vsize;
    assign ind$heard__ENA = respond__ENA_internal;
    assign ind$heard_heard_meth.coerce = agg_2e_tmp$data;
    assign ind$heard_heard_v.coerce = agg_2e_tmp3$data;
    assign respond__RDY_internal = (fifo$out_first__RDY & fifo$out_deq__RDY) & ind$heard__RDY;
    assign rule_ready[0] = respond__RDY_internal;
    assign say__RDY = say__RDY_internal;
    assign temp$ValuePair__ENA = say__ENA_internal;

    always @( posedge CLK) begin
      if (!nRST) begin
        vsize <= 0;
      end // nRST
      else begin
        if (respond__ENA_internal) begin
            agg_2e_tmp <= fifo$out_first$a;
            agg_2e_tmp3 <= fifo$out_first$b;
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

