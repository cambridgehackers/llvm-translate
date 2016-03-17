
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
    wire fifo0$out$deq__RDY;
    wire fifo0$in$enq__ENA;
    wire [703:0]fifo0$in$enq_v;
    wire fifo0$in$enq__RDY;
    wire [703:0]fifo0$out$first;
    wire fifo0$out$first__RDY;
    l_class_OC_FifoPong fifo0 (
        CLK,
        nRST,
        respond0__ENA_internal,
        fifo0$out$deq__RDY,
        fifo0$in$enq__ENA,
        fifo0$in$enq_v,
        fifo0$in$enq__RDY,
        fifo0$out$first,
        fifo0$out$first__RDY,
        rule_enable[10:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo1$out$deq__RDY;
    wire fifo1$in$enq__ENA;
    wire [703:0]fifo1$in$enq_v;
    wire fifo1$in$enq__RDY;
    wire [703:0]fifo1$out$first;
    wire fifo1$out$first__RDY;
    l_class_OC_FifoPong fifo1 (
        CLK,
        nRST,
        respond1__ENA_internal,
        fifo1$out$deq__RDY,
        fifo1$in$enq__ENA,
        fifo1$in$enq_v,
        fifo1$in$enq__RDY,
        fifo1$out$first,
        fifo1$out$first__RDY,
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo2$out$deq__RDY;
    wire fifo2$in$enq__ENA;
    wire [703:0]fifo2$in$enq_v;
    wire fifo2$in$enq__RDY;
    wire [703:0]fifo2$out$first;
    wire fifo2$out$first__RDY;
    l_class_OC_FifoPong fifo2 (
        CLK,
        nRST,
        respond2__ENA_internal,
        fifo2$out$deq__RDY,
        fifo2$in$enq__ENA,
        fifo2$in$enq_v,
        fifo2$in$enq__RDY,
        fifo2$out$first,
        fifo2$out$first__RDY,
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo3$out$deq__RDY;
    wire fifo3$in$enq__ENA;
    wire [703:0]fifo3$in$enq_v;
    wire fifo3$in$enq__RDY;
    wire [703:0]fifo3$out$first;
    wire fifo3$out$first__RDY;
    l_class_OC_FifoPong fifo3 (
        CLK,
        nRST,
        respond3__ENA_internal,
        fifo3$out$deq__RDY,
        fifo3$in$enq__ENA,
        fifo3$in$enq_v,
        fifo3$in$enq__RDY,
        fifo3$out$first,
        fifo3$out$first__RDY,
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo4$out$deq__RDY;
    wire fifo4$in$enq__ENA;
    wire [703:0]fifo4$in$enq_v;
    wire fifo4$in$enq__RDY;
    wire [703:0]fifo4$out$first;
    wire fifo4$out$first__RDY;
    l_class_OC_FifoPong fifo4 (
        CLK,
        nRST,
        respond4__ENA_internal,
        fifo4$out$deq__RDY,
        fifo4$in$enq__ENA,
        fifo4$in$enq_v,
        fifo4$in$enq__RDY,
        fifo4$out$first,
        fifo4$out$first__RDY,
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo5$out$deq__RDY;
    wire fifo5$in$enq__ENA;
    wire [703:0]fifo5$in$enq_v;
    wire fifo5$in$enq__RDY;
    wire [703:0]fifo5$out$first;
    wire fifo5$out$first__RDY;
    l_class_OC_FifoPong fifo5 (
        CLK,
        nRST,
        respond5__ENA_internal,
        fifo5$out$deq__RDY,
        fifo5$in$enq__ENA,
        fifo5$in$enq_v,
        fifo5$in$enq__RDY,
        fifo5$out$first,
        fifo5$out$first__RDY,
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo6$out$deq__RDY;
    wire fifo6$in$enq__ENA;
    wire [703:0]fifo6$in$enq_v;
    wire fifo6$in$enq__RDY;
    wire [703:0]fifo6$out$first;
    wire fifo6$out$first__RDY;
    l_class_OC_FifoPong fifo6 (
        CLK,
        nRST,
        respond6__ENA_internal,
        fifo6$out$deq__RDY,
        fifo6$in$enq__ENA,
        fifo6$in$enq_v,
        fifo6$in$enq__RDY,
        fifo6$out$first,
        fifo6$out$first__RDY,
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo7$out$deq__RDY;
    wire fifo7$in$enq__ENA;
    wire [703:0]fifo7$in$enq_v;
    wire fifo7$in$enq__RDY;
    wire [703:0]fifo7$out$first;
    wire fifo7$out$first__RDY;
    l_class_OC_FifoPong fifo7 (
        CLK,
        nRST,
        respond7__ENA_internal,
        fifo7$out$deq__RDY,
        fifo7$in$enq__ENA,
        fifo7$in$enq_v,
        fifo7$in$enq__RDY,
        fifo7$out$first,
        fifo7$out$first__RDY,
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo8$out$deq__RDY;
    wire fifo8$in$enq__ENA;
    wire [703:0]fifo8$in$enq_v;
    wire fifo8$in$enq__RDY;
    wire [703:0]fifo8$out$first;
    wire fifo8$out$first__RDY;
    l_class_OC_FifoPong fifo8 (
        CLK,
        nRST,
        respond8__ENA_internal,
        fifo8$out$deq__RDY,
        fifo8$in$enq__ENA,
        fifo8$in$enq_v,
        fifo8$in$enq__RDY,
        fifo8$out$first,
        fifo8$out$first__RDY,
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo9$out$deq__RDY;
    wire fifo9$in$enq__ENA;
    wire [703:0]fifo9$in$enq_v;
    wire fifo9$in$enq__RDY;
    wire [703:0]fifo9$out$first;
    wire fifo9$out$first__RDY;
    l_class_OC_FifoPong fifo9 (
        CLK,
        nRST,
        respond9__ENA_internal,
        fifo9$out$deq__RDY,
        fifo9$in$enq__ENA,
        fifo9$in$enq_v,
        fifo9$in$enq__RDY,
        fifo9$out$first,
        fifo9$out$first__RDY,
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    wire fifo10$out$deq__ENA;
    wire fifo10$out$deq__RDY;
    wire fifo10$in$enq__ENA;
    wire [703:0]fifo10$in$enq_v;
    wire fifo10$in$enq__RDY;
    wire [703:0]fifo10$out$first;
    wire fifo10$out$first__RDY;
    l_class_OC_FifoPong fifo10 (
        CLK,
        nRST,
        fifo10$out$deq__ENA,
        fifo10$out$deq__RDY,
        fifo10$in$enq__ENA,
        fifo10$in$enq_v,
        fifo10$in$enq__RDY,
        fifo10$out$first,
        fifo10$out$first__RDY,
        rule_enable[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT],
        rule_ready[10 + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT + `l_class_OC_FifoPong_RULE_COUNT:`l_class_OC_FifoPong_RULE_COUNT]);
    reg[31:0] vsize;
    assign ind$heard__ENA = respond0__ENA_internal || respond1__ENA_internal || respond2__ENA_internal || respond3__ENA_internal || respond4__ENA_internal || respond5__ENA_internal || respond6__ENA_internal || respond7__ENA_internal || respond8__ENA_internal || respond9__ENA_internal;
    assign ind$heard_heard_meth = respond0__ENA_internal ? 0 : respond1__ENA_internal ? 1 : respond2__ENA_internal ? 2 : respond3__ENA_internal ? 3 : respond4__ENA_internal ? 4 : respond5__ENA_internal ? 5 : respond6__ENA_internal ? 6 : respond7__ENA_internal ? 7 : respond8__ENA_internal ? 8 : 9;
    assign ind$heard_heard_v = respond0__ENA_internal ? fifo0$out$first$b : respond1__ENA_internal ? fifo1$out$first$b : respond2__ENA_internal ? fifo2$out$first$b : respond3__ENA_internal ? fifo3$out$first$b : respond4__ENA_internal ? fifo4$out$first$b : respond5__ENA_internal ? fifo5$out$first$b : respond6__ENA_internal ? fifo6$out$first$b : respond7__ENA_internal ? fifo7$out$first$b : respond8__ENA_internal ? fifo8$out$first$b : fifo9$out$first$b;
    assign respond0__RDY_internal = (fifo0$out$first__RDY & fifo0$out$deq__RDY) & ind$heard__RDY;
    assign respond1__RDY_internal = (fifo1$out$first__RDY & fifo1$out$deq__RDY) & ind$heard__RDY;
    assign respond2__RDY_internal = (fifo2$out$first__RDY & fifo2$out$deq__RDY) & ind$heard__RDY;
    assign respond3__RDY_internal = (fifo3$out$first__RDY & fifo3$out$deq__RDY) & ind$heard__RDY;
    assign respond4__RDY_internal = (fifo4$out$first__RDY & fifo4$out$deq__RDY) & ind$heard__RDY;
    assign respond5__RDY_internal = (fifo5$out$first__RDY & fifo5$out$deq__RDY) & ind$heard__RDY;
    assign respond6__RDY_internal = (fifo6$out$first__RDY & fifo6$out$deq__RDY) & ind$heard__RDY;
    assign respond7__RDY_internal = (fifo7$out$first__RDY & fifo7$out$deq__RDY) & ind$heard__RDY;
    assign respond8__RDY_internal = (fifo8$out$first__RDY & fifo8$out$deq__RDY) & ind$heard__RDY;
    assign respond9__RDY_internal = (fifo9$out$first__RDY & fifo9$out$deq__RDY) & ind$heard__RDY;
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
    assign say__RDY_internal = ((((((((fifo0$in$enq__RDY & fifo1$in$enq__RDY) & fifo2$in$enq__RDY) & fifo3$in$enq__RDY) & fifo4$in$enq__RDY) & fifo5$in$enq__RDY) & fifo6$in$enq__RDY) & fifo7$in$enq__RDY) & fifo8$in$enq__RDY) & fifo9$in$enq__RDY;
    assign say_meth == 0 ? &fifo0:say_meth == 1 ? &fifo1:say_meth == 2 ? &fifo2:say_meth == 3 ? &fifo3:say_meth == 4 ? &fifo4:say_meth == 5 ? &fifo5:say_meth == 6 ? &fifo6:say_meth == 7 ? &fifo7:say_meth == 8 ? &fifo8:&fifo9$in$enq__ENA = say__ENA_internal;
    assign say_meth == 0 ? &fifo0:say_meth == 1 ? &fifo1:say_meth == 2 ? &fifo2:say_meth == 3 ? &fifo3:say_meth == 4 ? &fifo4:say_meth == 5 ? &fifo5:say_meth == 6 ? &fifo6:say_meth == 7 ? &fifo7:say_meth == 8 ? &fifo8:&fifo9$in$enq_v = temp;
    assign temp$b = say_v;

    always @( posedge CLK) begin
      if (!nRST) begin
        vsize <= 0;
      end // nRST
    end // always @ (posedge CLK)
endmodule 

