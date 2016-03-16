
`include "l_class_OC_foo.vh"

module l_class_OC_foo (
    input CLK,
    input nRST,
    input heard__ENA,
    input [31:0]meth,
    input [31:0]v,
    output heard__RDY,
    input [`l_class_OC_foo_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_foo_RULE_COUNT:0]rule_ready);
    wire heard__RDY_internal;
    wire heard__ENA_internal = heard__ENA && heard__RDY_internal;
    assign heard__RDY = heard__RDY_internal;
    assign heard__RDY_internal = 1;
    assign meth_2e_addr = meth;
    assign this_2e_addr = this;
    assign v_2e_addr = v;

    always @( posedge CLK) begin
      if (!nRST) begin
      end // nRST
      else begin
        if (heard__ENA_internal) begin
            stop_main_program <= 1;
        end; // End of heard
      end
    end // always @ (posedge CLK)
endmodule 

