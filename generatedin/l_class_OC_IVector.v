
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
    wire respond0__RDY_internal;
    wire respond0__ENA_internal = rule_enable[0] && respond0__RDY_internal;
    wire respond1__RDY_internal;
    wire respond1__ENA_internal = rule_enable[1] && respond1__RDY_internal;
    wire respond2__RDY_internal;
    wire respond2__ENA_internal = rule_enable[2] && respond2__RDY_internal;
    wire respond3__RDY_internal;
    wire respond3__ENA_internal = rule_enable[3] && respond3__RDY_internal;
    wire respond4__RDY_internal;
    wire respond4__ENA_internal = rule_enable[4] && respond4__RDY_internal;
    wire respond5__RDY_internal;
    wire respond5__ENA_internal = rule_enable[5] && respond5__RDY_internal;
    wire respond6__RDY_internal;
    wire respond6__ENA_internal = rule_enable[6] && respond6__RDY_internal;
    wire respond7__RDY_internal;
    wire respond7__ENA_internal = rule_enable[7] && respond7__RDY_internal;
    wire respond8__RDY_internal;
    wire respond8__ENA_internal = rule_enable[8] && respond8__RDY_internal;
    wire respond9__RDY_internal;
    wire respond9__ENA_internal = rule_enable[9] && respond9__RDY_internal;
    wire say__RDY_internal;
    wire say__ENA_internal = say__ENA && say__RDY_internal;
    wire fifo0$in_enq__ENA;
    wire [703:0]fifo0$in_enq_v;
    wire fifo0$in_enq__RDY;
    wire fifo0$out_deq__RDY;
    wire [703:0]fifo0$out_first;
    wire fifo0$out_first__RDY;
    l_class_OC_FifoPong fifo0 (
        CLK,
        nRST,
        fifo0$in_enq__ENA,
        fifo0$in_enq_v,
        fifo0$in_enq__RDY,
        respond0__ENA_internal,
        fifo0$out_deq__RDY,
        fifo0$out_first,
        fifo0$out_first__RDY,
        rule_enable[10:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo1$in_enq__ENA;
    wire [703:0]fifo1$in_enq_v;
    wire fifo1$in_enq__RDY;
    wire fifo1$out_deq__RDY;
    wire [703:0]fifo1$out_first;
    wire fifo1$out_first__RDY;
    l_class_OC_FifoPong fifo1 (
        CLK,
        nRST,
        fifo1$in_enq__ENA,
        fifo1$in_enq_v,
        fifo1$in_enq__RDY,
        respond1__ENA_internal,
        fifo1$out_deq__RDY,
        fifo1$out_first,
        fifo1$out_first__RDY,
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo2$in_enq__ENA;
    wire [703:0]fifo2$in_enq_v;
    wire fifo2$in_enq__RDY;
    wire fifo2$out_deq__RDY;
    wire [703:0]fifo2$out_first;
    wire fifo2$out_first__RDY;
    l_class_OC_FifoPong fifo2 (
        CLK,
        nRST,
        fifo2$in_enq__ENA,
        fifo2$in_enq_v,
        fifo2$in_enq__RDY,
        respond2__ENA_internal,
        fifo2$out_deq__RDY,
        fifo2$out_first,
        fifo2$out_first__RDY,
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo3$in_enq__ENA;
    wire [703:0]fifo3$in_enq_v;
    wire fifo3$in_enq__RDY;
    wire fifo3$out_deq__RDY;
    wire [703:0]fifo3$out_first;
    wire fifo3$out_first__RDY;
    l_class_OC_FifoPong fifo3 (
        CLK,
        nRST,
        fifo3$in_enq__ENA,
        fifo3$in_enq_v,
        fifo3$in_enq__RDY,
        respond3__ENA_internal,
        fifo3$out_deq__RDY,
        fifo3$out_first,
        fifo3$out_first__RDY,
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo4$in_enq__ENA;
    wire [703:0]fifo4$in_enq_v;
    wire fifo4$in_enq__RDY;
    wire fifo4$out_deq__RDY;
    wire [703:0]fifo4$out_first;
    wire fifo4$out_first__RDY;
    l_class_OC_FifoPong fifo4 (
        CLK,
        nRST,
        fifo4$in_enq__ENA,
        fifo4$in_enq_v,
        fifo4$in_enq__RDY,
        respond4__ENA_internal,
        fifo4$out_deq__RDY,
        fifo4$out_first,
        fifo4$out_first__RDY,
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo5$in_enq__ENA;
    wire [703:0]fifo5$in_enq_v;
    wire fifo5$in_enq__RDY;
    wire fifo5$out_deq__RDY;
    wire [703:0]fifo5$out_first;
    wire fifo5$out_first__RDY;
    l_class_OC_FifoPong fifo5 (
        CLK,
        nRST,
        fifo5$in_enq__ENA,
        fifo5$in_enq_v,
        fifo5$in_enq__RDY,
        respond5__ENA_internal,
        fifo5$out_deq__RDY,
        fifo5$out_first,
        fifo5$out_first__RDY,
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo6$in_enq__ENA;
    wire [703:0]fifo6$in_enq_v;
    wire fifo6$in_enq__RDY;
    wire fifo6$out_deq__RDY;
    wire [703:0]fifo6$out_first;
    wire fifo6$out_first__RDY;
    l_class_OC_FifoPong fifo6 (
        CLK,
        nRST,
        fifo6$in_enq__ENA,
        fifo6$in_enq_v,
        fifo6$in_enq__RDY,
        respond6__ENA_internal,
        fifo6$out_deq__RDY,
        fifo6$out_first,
        fifo6$out_first__RDY,
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo7$in_enq__ENA;
    wire [703:0]fifo7$in_enq_v;
    wire fifo7$in_enq__RDY;
    wire fifo7$out_deq__RDY;
    wire [703:0]fifo7$out_first;
    wire fifo7$out_first__RDY;
    l_class_OC_FifoPong fifo7 (
        CLK,
        nRST,
        fifo7$in_enq__ENA,
        fifo7$in_enq_v,
        fifo7$in_enq__RDY,
        respond7__ENA_internal,
        fifo7$out_deq__RDY,
        fifo7$out_first,
        fifo7$out_first__RDY,
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo8$in_enq__ENA;
    wire [703:0]fifo8$in_enq_v;
    wire fifo8$in_enq__RDY;
    wire fifo8$out_deq__RDY;
    wire [703:0]fifo8$out_first;
    wire fifo8$out_first__RDY;
    l_class_OC_FifoPong fifo8 (
        CLK,
        nRST,
        fifo8$in_enq__ENA,
        fifo8$in_enq_v,
        fifo8$in_enq__RDY,
        respond8__ENA_internal,
        fifo8$out_deq__RDY,
        fifo8$out_first,
        fifo8$out_first__RDY,
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo9$in_enq__ENA;
    wire [703:0]fifo9$in_enq_v;
    wire fifo9$in_enq__RDY;
    wire fifo9$out_deq__RDY;
    wire [703:0]fifo9$out_first;
    wire fifo9$out_first__RDY;
    l_class_OC_FifoPong fifo9 (
        CLK,
        nRST,
        fifo9$in_enq__ENA,
        fifo9$in_enq_v,
        fifo9$in_enq__RDY,
        respond9__ENA_internal,
        fifo9$out_deq__RDY,
        fifo9$out_first,
        fifo9$out_first__RDY,
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
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
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    reg[31:0] vsize;
    assign ind$heard__ENA = respond0__ENA_internal || respond1__ENA_internal || respond2__ENA_internal || respond3__ENA_internal || respond4__ENA_internal || respond5__ENA_internal || respond6__ENA_internal || respond7__ENA_internal || respond8__ENA_internal || respond9__ENA_internal;
    assign ind$heard_heard_meth = respond0__ENA_internal ? 0 : respond1__ENA_internal ? 1 : respond2__ENA_internal ? 2 : respond3__ENA_internal ? 3 : respond4__ENA_internal ? 4 : respond5__ENA_internal ? 5 : respond6__ENA_internal ? 6 : respond7__ENA_internal ? 7 : respond8__ENA_internal ? 8 : 9;
    assign ind$heard_heard_v = respond0__ENA_internal ? fifo0$out_first$b : respond1__ENA_internal ? fifo1$out_first$b : respond2__ENA_internal ? fifo2$out_first$b : respond3__ENA_internal ? fifo3$out_first$b : respond4__ENA_internal ? fifo4$out_first$b : respond5__ENA_internal ? fifo5$out_first$b : respond6__ENA_internal ? fifo6$out_first$b : respond7__ENA_internal ? fifo7$out_first$b : respond8__ENA_internal ? fifo8$out_first$b : fifo9$out_first$b;
    assign respond0__RDY_internal = (fifo0$out_first__RDY & fifo0$out_deq__RDY) & ind$heard__RDY;
    assign respond1__RDY_internal = (fifo1$out_first__RDY & fifo1$out_deq__RDY) & ind$heard__RDY;
    assign respond2__RDY_internal = (fifo2$out_first__RDY & fifo2$out_deq__RDY) & ind$heard__RDY;
    assign respond3__RDY_internal = (fifo3$out_first__RDY & fifo3$out_deq__RDY) & ind$heard__RDY;
    assign respond4__RDY_internal = (fifo4$out_first__RDY & fifo4$out_deq__RDY) & ind$heard__RDY;
    assign respond5__RDY_internal = (fifo5$out_first__RDY & fifo5$out_deq__RDY) & ind$heard__RDY;
    assign respond6__RDY_internal = (fifo6$out_first__RDY & fifo6$out_deq__RDY) & ind$heard__RDY;
    assign respond7__RDY_internal = (fifo7$out_first__RDY & fifo7$out_deq__RDY) & ind$heard__RDY;
    assign respond8__RDY_internal = (fifo8$out_first__RDY & fifo8$out_deq__RDY) & ind$heard__RDY;
    assign respond9__RDY_internal = (fifo9$out_first__RDY & fifo9$out_deq__RDY) & ind$heard__RDY;
    assign rule_ready[0] = respond0__RDY_internal;
    assign rule_ready[1] = respond1__RDY_internal;
    assign rule_ready[2] = respond2__RDY_internal;
    assign rule_ready[3] = respond3__RDY_internal;
    assign rule_ready[4] = respond4__RDY_internal;
    assign rule_ready[5] = respond5__RDY_internal;
    assign rule_ready[6] = respond6__RDY_internal;
    assign rule_ready[7] = respond7__RDY_internal;
    assign rule_ready[8] = respond8__RDY_internal;
    assign rule_ready[9] = respond9__RDY_internal;
    assign say__RDY = say__RDY_internal;
    assign say__RDY_internal = ((((((((fifo0$in_enq__RDY & fifo1$in_enq__RDY) & fifo2$in_enq__RDY) & fifo3$in_enq__RDY) & fifo4$in_enq__RDY) & fifo5$in_enq__RDY) & fifo6$in_enq__RDY) & fifo7$in_enq__RDY) & fifo8$in_enq__RDY) & fifo9$in_enq__RDY;
    assign say_meth == 0 ? &fifo0:say_meth == 1 ? &fifo1:say_meth == 2 ? &fifo2:say_meth == 3 ? &fifo3:say_meth == 4 ? &fifo4:say_meth == 5 ? &fifo5:say_meth == 6 ? &fifo6:say_meth == 7 ? &fifo7:say_meth == 8 ? &fifo8:&fifo9$in_enq__ENA = say__ENA_internal;
    assign say_meth == 0 ? &fifo0:say_meth == 1 ? &fifo1:say_meth == 2 ? &fifo2:say_meth == 3 ? &fifo3:say_meth == 4 ? &fifo4:say_meth == 5 ? &fifo5:say_meth == 6 ? &fifo6:say_meth == 7 ? &fifo7:say_meth == 8 ? &fifo8:&fifo9$in_enq_v = temp;
    assign temp$b = say_v;

    always @( posedge CLK) begin
      if (!nRST) begin
        vsize <= 0;
      end // nRST
    end // always @ (posedge CLK)
endmodule 

