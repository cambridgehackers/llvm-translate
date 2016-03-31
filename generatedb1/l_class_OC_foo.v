
`include "l_class_OC_foo.vh"

module l_class_OC_foo (
    input CLK,
    input nRST,
    input indication$heard__ENA,
    input [31:0]indication$meth,
    input [31:0]indication$v,
    output indication$heard__RDY,
    input [`l_class_OC_foo_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_foo_RULE_COUNT:0]rule_ready);
    wire indication$heard__RDY_internal;
    wire indication$heard__ENA_internal = indication$heard__ENA && indication$heard__RDY_internal;
    assign indication$heard__RDY = indication$heard__RDY_internal;
    assign indication$heard__RDY_internal = 1;
    assign meth_2e_addr = meth;
    assign this_2e_addr = this_2e_addr;
    assign v_2e_addr = v;

    always @( posedge CLK) begin
      if (!nRST) begin
      end // nRST
      else begin
        if (indication$heard__ENA_internal) begin
            stop_main_program <= 1;
        end; // End of indication$heard
      end
    end // always @ (posedge CLK)
endmodule 
