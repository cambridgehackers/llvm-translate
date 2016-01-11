
`include "l_class_OC_IVector.vh"

module l_class_OC_IVector (
    input CLK,
    input nRST,
    input say__ENA,
    input [31:0]say_meth,
    input [31:0]say_v,
    output say__RDY,
    output fifo$in_enq__ENA,
    output [703:0]fifo$in_enq_in_enq_v,
    input fifo$in_enq__RDY,
    output fifo$out_deq__ENA,
    input fifo$out_deq__RDY,
    input [703:0]fifo$out_first,
    input fifo$out_first__RDY,
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
    reg[31:0] vsize;
    assign fifo$in_enq__ENA = say__ENA_internal;
    assign fifo$in_enq_v = temp;
    assign fifo$out_deq__ENA = respond_rule__ENA_internal;
    assign ind$heard__ENA = respond_rule__ENA_internal;
    assign ind$heard_heard_meth = fifo$out_first$a;
    assign ind$heard_heard_v = fifo$out_first$b;
    assign respond_rule__RDY_internal = (fifo$out_first__RDY & fifo$out_deq__RDY) & ind$heard__RDY;
    assign rule_ready[0] = respond_rule__RDY_internal;
    assign say__RDY = say__RDY_internal;
    assign say__RDY_internal = fifo$in_enq__RDY;

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

