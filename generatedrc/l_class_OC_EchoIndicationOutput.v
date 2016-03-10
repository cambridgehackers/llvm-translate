
`include "l_class_OC_EchoIndicationOutput.vh"

module l_class_OC_EchoIndicationOutput (
    input CLK,
    input nRST,
    input heard__ENA,
    input [31:0]heard_meth,
    input [31:0]heard_v,
    output heard__RDY,
    output pipe$enq__ENA,
    output [95:0]pipe$enq_v,
    input pipe$enq__RDY,
    input [`l_class_OC_EchoIndicationOutput_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_EchoIndicationOutput_RULE_COUNT:0]rule_ready);
    wire heard__RDY_internal;
    wire heard__ENA_internal = heard__ENA && heard__RDY_internal;
    assign heard__RDY = heard__RDY_internal;
    assign heard__RDY_internal = pipe$enq__RDY;
    assign ind$tag = 1;
    assign pipe$enq__ENA = heard__ENA_internal;
    assign pipe$enq_v = ind;

    always @( posedge CLK) begin
      if (!nRST) begin
      end // nRST
      else begin
        if (heard__ENA_internal) begin
            ind$data$heard$meth <= heard_meth;
            ind$data$heard$v <= heard_v;
        end; // End of heard
      end
    end // always @ (posedge CLK)
endmodule 

