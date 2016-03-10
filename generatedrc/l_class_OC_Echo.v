
`include "l_class_OC_Echo.vh"

module l_class_OC_Echo (
    input CLK,
    input nRST,
    input say__ENA,
    input [31:0]say_meth,
    input [31:0]say_v,
    output say__RDY,
    output indication$heard__ENA,
    output [31:0]indication$heard_meth,
    output [31:0]indication$heard_v,
    input indication$heard__RDY,
    input [`l_class_OC_Echo_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_Echo_RULE_COUNT:0]rule_ready);
    wire delay_rule__RDY_internal;
    wire delay_rule__ENA_internal = rule_enable[0] && delay_rule__RDY_internal;
    wire respond_rule__RDY_internal;
    wire respond_rule__ENA_internal = rule_enable[1] && respond_rule__RDY_internal;
    wire say__RDY_internal;
    wire say__ENA_internal = say__ENA && say__RDY_internal;
    reg[31:0] busy;
    reg[31:0] meth_temp;
    reg[31:0] v_temp;
    reg[31:0] busy_delay;
    reg[31:0] meth_delay;
    reg[31:0] v_delay;
    assign delay_rule__RDY_internal = busy != 0;
    assign indication$heard__ENA = respond_rule__ENA_internal;
    assign indication$heard_meth = meth_delay;
    assign indication$heard_v = v_delay;
    assign respond_rule__RDY_internal = (busy_delay != 0) & indication$heard__RDY;
    assign rule_ready[0] = delay_rule__RDY_internal;
    assign rule_ready[1] = respond_rule__RDY_internal;
    assign say__RDY = say__RDY_internal;
    assign say__RDY_internal = (busy != 0) ^ 1;

    always @( posedge CLK) begin
      if (!nRST) begin
        busy <= 0;
        meth_temp <= 0;
        v_temp <= 0;
        busy_delay <= 0;
        meth_delay <= 0;
        v_delay <= 0;
      end // nRST
      else begin
        if (delay_rule__ENA_internal) begin
            busy <= 0;
            busy_delay <= 1;
            meth_delay <= meth_temp;
            v_delay <= v_temp;
        end; // End of delay_rule
        if (respond_rule__ENA_internal) begin
            busy_delay <= 0;
        end; // End of respond_rule
        if (say__ENA_internal) begin
            busy <= 1;
            meth_temp <= say_meth;
            v_temp <= say_v;
        end; // End of say
      end
    end // always @ (posedge CLK)
endmodule 

