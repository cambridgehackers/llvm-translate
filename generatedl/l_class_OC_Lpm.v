
`include "l_class_OC_Lpm.vh"

module l_class_OC_Lpm (
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
    input [`l_class_OC_Lpm_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_Lpm_RULE_COUNT:0]rule_ready);
    wire enter__RDY_internal;
    wire enter__ENA_internal = rule_enable[0] && enter__RDY_internal;
    wire exit__RDY_internal;
    wire exit__ENA_internal = rule_enable[1] && exit__RDY_internal;
    wire recirc__RDY_internal;
    wire recirc__ENA_internal = rule_enable[2] && recirc__RDY_internal;
    wire respond__RDY_internal;
    wire respond__ENA_internal = rule_enable[3] && respond__RDY_internal;
    wire say__RDY_internal;
    wire say__ENA_internal = say__ENA && say__RDY_internal;
    wire inQ$out$deq__RDY;
    wire [703:0]inQ$out$first;
    wire inQ$out$first__RDY;
    l_class_OC_Fifo1_OC_0 inQ (
        CLK,
        nRST,
        enter__ENA_internal,
        inQ$out$deq__RDY,
        say__ENA_internal,
        temp,
        say__RDY_internal,
        inQ$out$first,
        inQ$out$first__RDY,
        rule_enable[4:`l_class_OC_Fifo1_OC_0_RULE_COUNT],
        rule_ready[4:`l_class_OC_Fifo1_OC_0_RULE_COUNT]);
    wire fifo$out$deq__RDY;
    wire fifo$in$enq__RDY;
    wire fifo$out$first__RDY;
    l_class_OC_Fifo1_OC_0 fifo (
        CLK,
        nRST,
        exit__ENA_internal || recirc__ENA_internal,
        fifo$out$deq__RDY,
        enter__ENA_internal || recirc__ENA_internal,
        mem$req_v,
        fifo$in$enq__RDY,
        outQ$in$enq_v,
        fifo$out$first__RDY,
        rule_enable[4 + `l_class_OC_Fifo1_OC_0_RULE_COUNT:`l_class_OC_Fifo1_OC_0_RULE_COUNT],
        rule_ready[4 + `l_class_OC_Fifo1_OC_0_RULE_COUNT:`l_class_OC_Fifo1_OC_0_RULE_COUNT]);
    wire outQ$out$deq__RDY;
    wire [703:0]outQ$in$enq_v;
    wire outQ$in$enq__RDY;
    wire [703:0]outQ$out$first;
    wire outQ$out$first__RDY;
    l_class_OC_Fifo1_OC_0 outQ (
        CLK,
        nRST,
        respond__ENA_internal,
        outQ$out$deq__RDY,
        exit__ENA_internal,
        outQ$in$enq_v,
        outQ$in$enq__RDY,
        outQ$out$first,
        outQ$out$first__RDY,
        rule_enable[4 + `l_class_OC_Fifo1_OC_0_RULE_COUNT + `l_class_OC_Fifo1_OC_0_RULE_COUNT:`l_class_OC_Fifo1_OC_0_RULE_COUNT],
        rule_ready[4 + `l_class_OC_Fifo1_OC_0_RULE_COUNT + `l_class_OC_Fifo1_OC_0_RULE_COUNT:`l_class_OC_Fifo1_OC_0_RULE_COUNT]);
    wire mem$memdelay__ENA;
    wire mem$memdelay__RDY;
    wire [703:0]mem$v;
    wire mem$req__RDY;
    wire mem$resAccept__RDY;
    wire [703:0]mem$resValue;
    wire mem$resValue__RDY;
    l_class_OC_LpmMemory mem (
        CLK,
        nRST,
        enter__ENA_internal || recirc__ENA_internal,
        mem$v,
        mem$req__RDY,
        exit__ENA_internal || recirc__ENA_internal,
        mem$resAccept__RDY,
        mem$resValue,
        mem$resValue__RDY,
        rule_enable[4 + `l_class_OC_Fifo1_OC_0_RULE_COUNT + `l_class_OC_Fifo1_OC_0_RULE_COUNT + `l_class_OC_Fifo1_OC_0_RULE_COUNT:`l_class_OC_LpmMemory_RULE_COUNT],
        rule_ready[4 + `l_class_OC_Fifo1_OC_0_RULE_COUNT + `l_class_OC_Fifo1_OC_0_RULE_COUNT + `l_class_OC_Fifo1_OC_0_RULE_COUNT:`l_class_OC_LpmMemory_RULE_COUNT]);
    reg[31:0] doneCount;
    assign enter__RDY_internal = ((inQ$out$first__RDY & inQ$out$deq__RDY) & fifo$in$enq__RDY) & mem$req__RDY;
    assign exit__RDY_internal = (((((((doneCount % 5) != 0) ^ 1) & fifo$out$first__RDY) & mem$resValue__RDY) & mem$resAccept__RDY) & fifo$out$deq__RDY) & outQ$in$enq__RDY;
    assign indication$heard__ENA = respond__ENA_internal;
    assign indication$heard_meth = outQ$out$first$a;
    assign indication$heard_v = outQ$out$first$b;
    assign recirc__RDY_internal = (((((((((doneCount % 5) != 0) ^ 1) ^ 1) & fifo$out$first__RDY) & mem$resValue__RDY) & mem$resAccept__RDY) & fifo$out$deq__RDY) & fifo$in$enq__RDY) & mem$req__RDY;
    assign respond__RDY_internal = (outQ$out$first__RDY & outQ$out$deq__RDY) & indication$heard__RDY;
    assign rule_ready[0] = enter__RDY_internal;
    assign rule_ready[1] = exit__RDY_internal;
    assign rule_ready[2] = recirc__RDY_internal;
    assign rule_ready[3] = respond__RDY_internal;
    assign say__RDY = say__RDY_internal;
    assign temp$a = say_meth;
    assign temp$b = say_v;

    always @( posedge CLK) begin
      if (!nRST) begin
        doneCount <= 0;
      end // nRST
      else begin
        if (exit__RDY__ENA_internal) begin
            doneCount <= doneCount + 1;
        end; // End of exit__RDY
        if (recirc__RDY__ENA_internal) begin
            doneCount <= doneCount + 1;
        end; // End of recirc__RDY
      end
    end // always @ (posedge CLK)
endmodule 

