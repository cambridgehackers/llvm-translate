
`include "l_class_OC_EchoRequestOutput.vh"

module l_class_OC_EchoRequestOutput (
    input CLK,
    input nRST,
    input request$say__ENA,
    input [31:0]request$say_meth,
    input [31:0]request$say_v,
    output request$say__RDY,
    output pipe$enq__ENA,
    output [95:0]pipe$enq_v,
    input pipe$enq__RDY,
    input [`l_class_OC_EchoRequestOutput_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_EchoRequestOutput_RULE_COUNT:0]rule_ready);
    wire request$say__RDY_internal;
    wire request$say__ENA_internal = request$say__ENA && request$say__RDY_internal;
    assign ind$tag = 1;
    assign pipe$enq__ENA = request$say__ENA_internal;
    assign pipe$enq_v = ind;
    assign request$say__RDY = request$say__RDY_internal;
    assign request$say__RDY_internal = pipe$enq__RDY;

    always @( posedge CLK) begin
      if (!nRST) begin
      end // nRST
      else begin
        if (request$say__ENA_internal) begin
            ind$data$say$meth <= say_meth;
            ind$data$say$v <= say_v;
        end; // End of request$say
      end
    end // always @ (posedge CLK)
endmodule 

