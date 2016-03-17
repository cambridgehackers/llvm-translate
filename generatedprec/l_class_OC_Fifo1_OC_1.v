
`include "l_class_OC_Fifo1_OC_1.vh"

module l_class_OC_Fifo1_OC_1 (
    input CLK,
    input nRST,
    input out$deq__ENA,
    output out$deq__RDY,
    input in$enq__ENA,
    input [383:0]in$enq_v,
    output in$enq__RDY,
    output [383:0]out$first,
    output out$first__RDY,
    input [`l_class_OC_Fifo1_OC_1_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_Fifo1_OC_1_RULE_COUNT:0]rule_ready);
    wire out$deq__RDY_internal;
    wire out$deq__ENA_internal = out$deq__ENA && out$deq__RDY_internal;
    wire in$enq__RDY_internal;
    wire in$enq__ENA_internal = in$enq__ENA && in$enq__RDY_internal;
    reg[383:0] element;
    reg full;
    assign first$_ = element;
    assign first$__ENA = out$first__ENA_internal;
    assign in$enq__RDY = in$enq__RDY_internal;
    assign in$enq__RDY_internal = full ^ 1;
    assign out$deq__RDY = out$deq__RDY_internal;
    assign out$deq__RDY_internal = full;
    assign out$first__RDY_internal = full;

    always @( posedge CLK) begin
      if (!nRST) begin
        element <= 0;
        full <= 0;
      end // nRST
      else begin
        if (out$deq__ENA_internal) begin
            full <= 0;
        end; // End of out$deq
        if (in$enq__ENA_internal) begin
            full <= 1;
        end; // End of in$enq
      end
    end // always @ (posedge CLK)
endmodule 

