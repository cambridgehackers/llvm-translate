
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
    wire fifo$out$deq__RDY;
    wire [255:0]fifo$out$first;
    wire fifo$out$first__RDY;
    l_class_OC_Fifo1_OC_1 fifo (
        CLK,
        nRST,
        respond__ENA_internal,
        fifo$out$deq__RDY,
        say__ENA_internal,
        temp,
        say__RDY_internal,
        fifo$out$first,
        fifo$out$first__RDY,
        rule_enable[1:`l_class_OC_Fifo1_OC_1_RULE_COUNT],
        rule_ready[1:`l_class_OC_Fifo1_OC_1_RULE_COUNT]);
    reg[:0] fcounter;
    reg[1:0] counter;
    reg[10:0] gcounter;
    assign ind$heard__ENA = respond__ENA_internal;
    assign ind$heard_heard_meth = agg_2e_tmp;
    assign ind$heard_heard_v = agg_2e_tmp3;
    assign respond__RDY_internal = (fifo$out$first__RDY & fifo$out$deq__RDY) & ind$heard__RDY;
    assign rule_ready[0] = respond__RDY_internal;
    assign say__RDY = say__RDY_internal;
    assign temp$a = say_meth;
    assign temp$b = say_v;

    always @( posedge CLK) begin
      if (!nRST) begin
      end // nRST
      else begin
        if (respond__ENA_internal) begin
            agg_2e_tmp <= fifo$out$first$a;
            agg_2e_tmp3 <= fifo$out$first$b;
            gcounter <= gcounter + 1;
        end; // End of respond__ENA
      end
    end // always @ (posedge CLK)
endmodule 

