
`include "l_class_OC_Fifo2.vh"

module l_class_OC_Fifo2 (
    input CLK,
    input nRST,
    input out$deq__ENA,
    output out$deq__RDY,
    input in$enq__ENA,
    input [703:0]in$enq_v,
    output in$enq__RDY,
    output [703:0]out$first,
    output out$first__RDY,
    input [`l_class_OC_Fifo2_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_Fifo2_RULE_COUNT:0]rule_ready);
    wire out$deq__RDY_internal;
    wire out$deq__ENA_internal = out$deq__ENA && out$deq__RDY_internal;
    wire in$enq__RDY_internal;
    wire in$enq__ENA_internal = in$enq__ENA && in$enq__RDY_internal;
    reg[31:0] rindex;
    reg[31:0] windex;
    assign in$enq__RDY = in$enq__RDY_internal;
    assign in$enq__RDY_internal = ((windex + 1) % 2) != rindex;
    assign out$deq__RDY = out$deq__RDY_internal;
    assign out$deq__RDY_internal = rindex != windex;
    assign out$first = *(rindex == 0 ? element0:&element1);
    assign out$first__RDY_internal = rindex != windex;

    always @( posedge CLK) begin
      if (!nRST) begin
        rindex <= 0;
        windex <= 0;
      end // nRST
      else begin
        if (out$deq__ENA_internal) begin
            rindex <= (rindex + 1) % 2;
        end; // End of out$deq
        if (in$enq__ENA_internal) begin
            *(windex == 0 ? element0:&element1) <= enq_v;
            windex <= (windex + 1) % 2;
        end; // End of in$enq
      end
    end // always @ (posedge CLK)
endmodule 

