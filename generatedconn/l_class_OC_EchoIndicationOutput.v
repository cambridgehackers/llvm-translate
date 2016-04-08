
`include "l_class_OC_EchoIndicationOutput.vh"

module l_class_OC_EchoIndicationOutput (
    input CLK,
    input nRST,
    input indication$heard__ENA,
    input [31:0]indication$heard_meth,
    input [31:0]indication$heard_v,
    output indication$heard__RDY,
    output pipe$enq__ENA,
    output [95:0]pipe$enq_v,
    input pipe$enq__RDY,
    input [`l_class_OC_EchoIndicationOutput_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_EchoIndicationOutput_RULE_COUNT:0]rule_ready);
    wire indication$heard__RDY_internal;
    wire indication$heard__ENA_internal = indication$heard__ENA && indication$heard__RDY_internal;
    assign ind$tag = 1;
    assign indication$heard__RDY = indication$heard__RDY_internal;
    assign indication$heard__RDY_internal = pipe$enq__RDY;
    assign pipe$enq__ENA = indication$heard__ENA_internal;
    assign pipe$enq_v = ind;

    always @( posedge CLK) begin
      if (!nRST) begin
      end // nRST
      else begin
        if (indication$heard__ENA_internal) begin
            ind$data$heard$meth <= heard_meth;
            ind$data$heard$v <= heard_v;
        end; // End of indication$heard__ENA
      end
    end // always @ (posedge CLK)
endmodule 

