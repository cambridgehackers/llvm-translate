
`include "l_class_OC_EchoIndicationInput.vh"

module l_class_OC_EchoIndicationInput (
    input CLK,
    input nRST,
    input enq__ENA,
    input [95:0]enq_v,
    output enq__RDY,
    input [`l_class_OC_EchoIndicationInput_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_EchoIndicationInput_RULE_COUNT:0]rule_ready);
    wire input_rule__RDY_internal;
    wire input_rule__ENA_internal = rule_enable[0] && input_rule__RDY_internal;
    wire enq__RDY_internal;
    wire enq__ENA_internal = enq__ENA && enq__RDY_internal;
    reg[31:0] busy_delay;
    reg[31:0] meth_delay;
    reg[31:0] v_delay;
    assign enq__RDY = enq__RDY_internal;
    assign enq__RDY_internal = (busy_delay != 0) ^ 1;
    assign input_rule__RDY_internal = (busy_delay != 0) & request$heard__RDY;
    assign request$heard__ENA = input_rule__ENA_internal;
    assign request$heard_meth = meth_delay;
    assign request$heard_v = v_delay;
    assign rule_ready[0] = input_rule__RDY_internal;

    always @( posedge CLK) begin
      if (!nRST) begin
        busy_delay <= 0;
        meth_delay <= 0;
        v_delay <= 0;
      end // nRST
      else begin
        if (enq__ENA_internal) begin
            if (enq_v$tag == 1)
            meth_delay <= enq_v$data$heard$meth;
            if (enq_v$tag == 1)
            v_delay <= enq_v$data$heard$v;
            if (enq_v$tag == 1)
            busy_delay <= 1;
        end; // End of enq
        if (input_rule__ENA_internal) begin
            busy_delay <= 0;
        end; // End of input_rule
      end
    end // always @ (posedge CLK)
endmodule 

