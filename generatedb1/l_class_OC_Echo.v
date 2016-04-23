
`include "l_class_OC_Echo.vh"

module l_class_OC_Echo (
    input CLK,
    input nRST,
    input request$say__ENA,
    input [31:0]request$say_meth,
    input [31:0]request$say_v,
    input request$say2__ENA,
    input [31:0]request$say2_meth,
    input [31:0]request$say2_v,
    output request$say2__RDY,
    output request$say__RDY,
    input x2y__ENA,
    output x2y__RDY,
    input y2x__ENA,
    output y2x__RDY,
    input y2xnull__ENA,
    output y2xnull__RDY,
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
    wire request$say__RDY_internal;
    wire request$say__ENA_internal = request$say__ENA && request$say__RDY_internal;
    wire request$say2__RDY_internal;
    wire request$say2__ENA_internal = request$say2__ENA && request$say2__RDY_internal;
    wire x2y__RDY_internal;
    wire x2y__ENA_internal = x2y__ENA && x2y__RDY_internal;
    wire y2x__RDY_internal;
    wire y2x__ENA_internal = y2x__ENA && y2x__RDY_internal;
    wire y2xnull__RDY_internal;
    wire y2xnull__ENA_internal = y2xnull__ENA && y2xnull__RDY_internal;
    reg[31:0] busy;
    reg[31:0] meth_temp;
    reg[31:0] v_temp;
    reg[31:0] busy_delay;
    reg[31:0] meth_delay;
    reg[31:0] v_delay;
    reg[31:0] x;
    reg[31:0] y;
    assign delay_rule__RDY_internal = ((busy != 0) & (busy_delay == 0)) != 0;
    assign indication$heard__ENA = respond_rule__ENA_internal;
    assign indication$heard_meth = meth_delay;
    assign indication$heard_v = v_delay;
    assign request$say2__RDY = request$say2__RDY_internal;
    assign request$say2__RDY_internal = (busy != 0) ^ 1;
    assign request$say__RDY = request$say__RDY_internal;
    assign request$say__RDY_internal = (busy != 0) ^ 1;
    assign respond_rule__RDY_internal = (busy_delay != 0) & indication$heard__RDY;
    assign rule_ready[0] = delay_rule__RDY_internal;
    assign rule_ready[1] = respond_rule__RDY_internal;
    assign x2y__RDY = x2y__RDY_internal;
    assign x2y__RDY_internal = 1;
    assign y2x__RDY = y2x__RDY_internal;
    assign y2x__RDY_internal = 1;
    assign y2xnull__RDY = y2xnull__RDY_internal;
    assign y2xnull__RDY_internal = 1;

    always @( posedge CLK) begin
      if (!nRST) begin
        busy <= 0;
        meth_temp <= 0;
        v_temp <= 0;
        busy_delay <= 0;
        meth_delay <= 0;
        v_delay <= 0;
        x <= 0;
        y <= 0;
      end // nRST
      else begin
        if (delay_rule__ENA_internal) begin
            busy <= 0;
            busy_delay <= 1;
            meth_delay <= meth_temp;
            v_delay <= v_temp;
        end; // End of delay_rule__ENA
        if (respond_rule__ENA_internal) begin
            busy_delay <= 0;
        end; // End of respond_rule__ENA
        if (request$say__ENA_internal) begin
            meth_temp <= say_meth;
            v_temp <= say_v;
            busy <= 1;
        end; // End of request$say__ENA
        if (request$say2__ENA_internal) begin
            meth_temp <= say2_meth;
            v_temp <= say2_v;
            busy <= 1;
        end; // End of request$say2__ENA
        if (x2y__ENA_internal) begin
            y <= x;
        end; // End of x2y__ENA
        if (y2x__ENA_internal) begin
            x <= y;
        end; // End of y2x__ENA
      end
    end // always @ (posedge CLK)
endmodule 

