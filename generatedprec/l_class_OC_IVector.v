
`include "l_class_OC_IVector.vh"

module l_class_OC_IVector (
    input CLK,
    input nRST,
    input say__ENA,
    input [191:0]say_meth,
    input [191:0]say_v,
    output say__RDY,
    output ind$heard__ENA,
    output [191:0]ind$heard_heard_meth,
    output [191:0]ind$heard_heard_v,
    input ind$heard__RDY,
    input [`l_class_OC_IVector_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_IVector_RULE_COUNT:0]rule_ready);
    wire respond__RDY_internal;
    wire respond__ENA_internal = rule_enable[0] && respond__RDY_internal;
    wire say__RDY_internal;
    wire say__ENA_internal = say__ENA && say__RDY_internal;
    wire fifo$out_deq__RDY;
    wire [383:0]fifo$out_first;
    wire fifo$out_first__RDY;
    l_class_OC_Fifo1_OC_1 fifo (
        CLK,
        nRST,
        say__ENA_internal,
        temp,
        say__RDY_internal,
        respond__ENA_internal,
        fifo$out_deq__RDY,
        fifo$out_first,
        fifo$out_first__RDY,
        rule_enable[1:`l_class_OC_Fifo1_OC_1_RULE_COUNT],
        rule_ready[1:`l_class_OC_Fifo1_OC_1_RULE_COUNT]);
    reg[23:0] fcounter;
    reg[1:0] counter;
    reg[10:0] gcounter;
    assign agg_2e_tmp$__ENA = respond__ENA_internal;
    assign agg_2e_tmp$_arg = fifo$out_first$a;
    assign agg_2e_tmp3$__ENA = respond__ENA_internal;
    assign agg_2e_tmp3$_arg = fifo$out_first$b;
    assign ind$heard__ENA = respond__ENA_internal;
    assign ind$heard_heard_meth = agg_2e_tmp;
    assign ind$heard_heard_v = agg_2e_tmp3;
    assign respond__RDY_internal = (fifo$out_first__RDY & fifo$out_deq__RDY) & ind$heard__RDY;
    assign rule_ready[0] = respond__RDY_internal;
    assign say__RDY = say__RDY_internal;

    always @( posedge CLK) begin
      if (!nRST) begin
      end // nRST
      else begin
        if (respond__ENA_internal) begin
            gcounter <= gcounter + 1;
        end; // End of respond
      end
    end // always @ (posedge CLK)
endmodule 

