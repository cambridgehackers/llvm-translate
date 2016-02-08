
`include "l_class_OC_IVector.vh"

module l_class_OC_IVector (
    input CLK,
    input nRST,
    input say__ENA,
    input [31:0]say_meth,
    input [31:0]say_v,
    output say__RDY,
    output ind$heard__ENA,
    output [31:0]ind$heard_heard_meth,
    output [31:0]ind$heard_heard_v,
    input ind$heard__RDY,
    input [`l_class_OC_IVector_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_IVector_RULE_COUNT:0]rule_ready);
    wire respond_rule__RDY_internal;
    wire respond_rule__ENA_internal = rule_enable[0] && respond_rule__RDY_internal;
    wire say__RDY_internal;
    wire say__ENA_internal = say__ENA && say__RDY_internal;
    wire fifo0$in_enq__ENA;
    wire [703:0]fifo0$in_enq_v;
    wire fifo0$in_enq__RDY;
    wire fifo0$out_deq__ENA;
    wire fifo0$out_deq__RDY;
    wire [703:0]fifo0$out_first;
    wire fifo0$out_first__RDY;
    l_class_OC_FifoPong fifo0 (
        CLK,
        nRST,
        fifo0$in_enq__ENA,
        fifo0$in_enq_v,
        fifo0$in_enq__RDY,
        fifo0$out_deq__ENA,
        fifo0$out_deq__RDY,
        fifo0$out_first,
        fifo0$out_first__RDY,
        rule_enable[1:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[1:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo1$out_deq__RDY;
    wire [703:0]fifo1$out_first;
    wire fifo1$out_first__RDY;
    l_class_OC_FifoPong fifo1 (
        CLK,
        nRST,
        say__ENA_internal,
        temp,
        say__RDY_internal,
        respond_rule__ENA_internal,
        fifo1$out_deq__RDY,
        fifo1$out_first,
        fifo1$out_first__RDY,
        rule_enable[1 + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[1 + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo2$in_enq__ENA;
    wire [703:0]fifo2$in_enq_v;
    wire fifo2$in_enq__RDY;
    wire fifo2$out_deq__ENA;
    wire fifo2$out_deq__RDY;
    wire [703:0]fifo2$out_first;
    wire fifo2$out_first__RDY;
    l_class_OC_FifoPong fifo2 (
        CLK,
        nRST,
        fifo2$in_enq__ENA,
        fifo2$in_enq_v,
        fifo2$in_enq__RDY,
        fifo2$out_deq__ENA,
        fifo2$out_deq__RDY,
        fifo2$out_first,
        fifo2$out_first__RDY,
        rule_enable[1 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[1 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo3$in_enq__ENA;
    wire [703:0]fifo3$in_enq_v;
    wire fifo3$in_enq__RDY;
    wire fifo3$out_deq__ENA;
    wire fifo3$out_deq__RDY;
    wire [703:0]fifo3$out_first;
    wire fifo3$out_first__RDY;
    l_class_OC_FifoPong fifo3 (
        CLK,
        nRST,
        fifo3$in_enq__ENA,
        fifo3$in_enq_v,
        fifo3$in_enq__RDY,
        fifo3$out_deq__ENA,
        fifo3$out_deq__RDY,
        fifo3$out_first,
        fifo3$out_first__RDY,
        rule_enable[1 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[1 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo4$in_enq__ENA;
    wire [703:0]fifo4$in_enq_v;
    wire fifo4$in_enq__RDY;
    wire fifo4$out_deq__ENA;
    wire fifo4$out_deq__RDY;
    wire [703:0]fifo4$out_first;
    wire fifo4$out_first__RDY;
    l_class_OC_FifoPong fifo4 (
        CLK,
        nRST,
        fifo4$in_enq__ENA,
        fifo4$in_enq_v,
        fifo4$in_enq__RDY,
        fifo4$out_deq__ENA,
        fifo4$out_deq__RDY,
        fifo4$out_first,
        fifo4$out_first__RDY,
        rule_enable[1 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[1 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo5$in_enq__ENA;
    wire [703:0]fifo5$in_enq_v;
    wire fifo5$in_enq__RDY;
    wire fifo5$out_deq__ENA;
    wire fifo5$out_deq__RDY;
    wire [703:0]fifo5$out_first;
    wire fifo5$out_first__RDY;
    l_class_OC_FifoPong fifo5 (
        CLK,
        nRST,
        fifo5$in_enq__ENA,
        fifo5$in_enq_v,
        fifo5$in_enq__RDY,
        fifo5$out_deq__ENA,
        fifo5$out_deq__RDY,
        fifo5$out_first,
        fifo5$out_first__RDY,
        rule_enable[1 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[1 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo6$in_enq__ENA;
    wire [703:0]fifo6$in_enq_v;
    wire fifo6$in_enq__RDY;
    wire fifo6$out_deq__ENA;
    wire fifo6$out_deq__RDY;
    wire [703:0]fifo6$out_first;
    wire fifo6$out_first__RDY;
    l_class_OC_FifoPong fifo6 (
        CLK,
        nRST,
        fifo6$in_enq__ENA,
        fifo6$in_enq_v,
        fifo6$in_enq__RDY,
        fifo6$out_deq__ENA,
        fifo6$out_deq__RDY,
        fifo6$out_first,
        fifo6$out_first__RDY,
        rule_enable[1 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[1 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo7$in_enq__ENA;
    wire [703:0]fifo7$in_enq_v;
    wire fifo7$in_enq__RDY;
    wire fifo7$out_deq__ENA;
    wire fifo7$out_deq__RDY;
    wire [703:0]fifo7$out_first;
    wire fifo7$out_first__RDY;
    l_class_OC_FifoPong fifo7 (
        CLK,
        nRST,
        fifo7$in_enq__ENA,
        fifo7$in_enq_v,
        fifo7$in_enq__RDY,
        fifo7$out_deq__ENA,
        fifo7$out_deq__RDY,
        fifo7$out_first,
        fifo7$out_first__RDY,
        rule_enable[1 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[1 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo8$in_enq__ENA;
    wire [703:0]fifo8$in_enq_v;
    wire fifo8$in_enq__RDY;
    wire fifo8$out_deq__ENA;
    wire fifo8$out_deq__RDY;
    wire [703:0]fifo8$out_first;
    wire fifo8$out_first__RDY;
    l_class_OC_FifoPong fifo8 (
        CLK,
        nRST,
        fifo8$in_enq__ENA,
        fifo8$in_enq_v,
        fifo8$in_enq__RDY,
        fifo8$out_deq__ENA,
        fifo8$out_deq__RDY,
        fifo8$out_first,
        fifo8$out_first__RDY,
        rule_enable[1 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[1 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo9$in_enq__ENA;
    wire [703:0]fifo9$in_enq_v;
    wire fifo9$in_enq__RDY;
    wire fifo9$out_deq__ENA;
    wire fifo9$out_deq__RDY;
    wire [703:0]fifo9$out_first;
    wire fifo9$out_first__RDY;
    l_class_OC_FifoPong fifo9 (
        CLK,
        nRST,
        fifo9$in_enq__ENA,
        fifo9$in_enq_v,
        fifo9$in_enq__RDY,
        fifo9$out_deq__ENA,
        fifo9$out_deq__RDY,
        fifo9$out_first,
        fifo9$out_first__RDY,
        rule_enable[1 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[1 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo10$in_enq__ENA;
    wire [703:0]fifo10$in_enq_v;
    wire fifo10$in_enq__RDY;
    wire fifo10$out_deq__ENA;
    wire fifo10$out_deq__RDY;
    wire [703:0]fifo10$out_first;
    wire fifo10$out_first__RDY;
    l_class_OC_FifoPong fifo10 (
        CLK,
        nRST,
        fifo10$in_enq__ENA,
        fifo10$in_enq_v,
        fifo10$in_enq__RDY,
        fifo10$out_deq__ENA,
        fifo10$out_deq__RDY,
        fifo10$out_first,
        fifo10$out_first__RDY,
        rule_enable[1 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[1 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    reg[31:0] vsize;
    assign ind$heard__ENA = respond_rule__ENA_internal;
    assign ind$heard_heard_meth = fifo1$out_first$a;
    assign ind$heard_heard_v = fifo1$out_first$b;
    assign respond_rule__RDY_internal = (fifo1$out_first__RDY & fifo1$out_deq__RDY) & ind$heard__RDY;
    assign rule_ready[0] = respond_rule__RDY_internal;
    assign say__RDY = say__RDY_internal;

    always @( posedge CLK) begin
      if (!nRST) begin
        vsize <= 0;
      end // nRST
      else begin
        if (say__ENA_internal) begin
            temp$a <= say_meth;
            temp$b <= say_v;
        end; // End of say
      end
    end // always @ (posedge CLK)
endmodule 

