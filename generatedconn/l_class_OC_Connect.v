
`include "l_class_OC_Connect.vh"

module l_class_OC_Connect (
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
    input [`l_class_OC_Connect_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_Connect_RULE_COUNT:0]rule_ready);
    wire respond__RDY_internal;
    wire respond__ENA_internal = rule_enable[0] && respond__RDY_internal;
    wire say__RDY_internal;
    wire say__ENA_internal = say__ENA && say__RDY_internal;
    wire fifo$out_deq__RDY;
    wire [63:0]fifo$out_first;
    wire fifo$out_first__RDY;
    l_class_OC_Fifo1_OC_2 fifo (
        CLK,
        nRST,
        say__ENA_internal,
        temp,
        say__RDY_internal,
        respond__ENA_internal,
        fifo$out_deq__RDY,
        fifo$out_first,
        fifo$out_first__RDY,
        rule_enable[1:`l_class_OC_Fifo1_OC_2_RULE_COUNT],
        rule_ready[1:`l_class_OC_Fifo1_OC_2_RULE_COUNT]);
    l_class_OC_EchoIndicationOutput lEchoIndicationOutput (
        CLK,
        nRST,
        rule_enable[1 + `l_class_OC_Fifo1_OC_2_RULE_COUNT:`l_class_OC_EchoIndicationOutput_RULE_COUNT],
        rule_ready[1 + `l_class_OC_Fifo1_OC_2_RULE_COUNT:`l_class_OC_EchoIndicationOutput_RULE_COUNT]);
    wire lEchoRequestInput$pipe_enq__ENA;
    wire [95:0]lEchoRequestInput$pipe_enq_v;
    wire lEchoRequestInput$pipe_enq__RDY;
    l_class_OC_EchoRequestInput lEchoRequestInput (
        CLK,
        nRST,
        lEchoRequestInput$pipe_enq__ENA,
        lEchoRequestInput$pipe_enq_v,
        lEchoRequestInput$pipe_enq__RDY,
        rule_enable[1 + `l_class_OC_Fifo1_OC_2_RULE_COUNT + `l_class_OC_EchoIndicationOutput_RULE_COUNT:`l_class_OC_EchoRequestInput_RULE_COUNT],
        rule_ready[1 + `l_class_OC_Fifo1_OC_2_RULE_COUNT + `l_class_OC_EchoIndicationOutput_RULE_COUNT:`l_class_OC_EchoRequestInput_RULE_COUNT]);
    l_class_OC_Echo lEcho (
        CLK,
        nRST,
        rule_enable[1 + `l_class_OC_Fifo1_OC_2_RULE_COUNT + `l_class_OC_EchoIndicationOutput_RULE_COUNT + `l_class_OC_EchoRequestInput_RULE_COUNT:`l_class_OC_Echo_RULE_COUNT],
        rule_ready[1 + `l_class_OC_Fifo1_OC_2_RULE_COUNT + `l_class_OC_EchoIndicationOutput_RULE_COUNT + `l_class_OC_EchoRequestInput_RULE_COUNT:`l_class_OC_Echo_RULE_COUNT]);
    wire lEchoIndicationInput_test$pipe_enq__ENA;
    wire [95:0]lEchoIndicationInput_test$pipe_enq_v;
    wire lEchoIndicationInput_test$pipe_enq__RDY;
    l_class_OC_EchoIndicationInput lEchoIndicationInput_test (
        CLK,
        nRST,
        lEchoIndicationInput_test$pipe_enq__ENA,
        lEchoIndicationInput_test$pipe_enq_v,
        lEchoIndicationInput_test$pipe_enq__RDY,
        rule_enable[1 + `l_class_OC_Fifo1_OC_2_RULE_COUNT + `l_class_OC_EchoIndicationOutput_RULE_COUNT + `l_class_OC_EchoRequestInput_RULE_COUNT + `l_class_OC_Echo_RULE_COUNT:`l_class_OC_EchoIndicationInput_RULE_COUNT],
        rule_ready[1 + `l_class_OC_Fifo1_OC_2_RULE_COUNT + `l_class_OC_EchoIndicationOutput_RULE_COUNT + `l_class_OC_EchoRequestInput_RULE_COUNT + `l_class_OC_Echo_RULE_COUNT:`l_class_OC_EchoIndicationInput_RULE_COUNT]);
    assign ind$heard__ENA = respond__ENA_internal;
    assign ind$heard_heard_meth = fifo$out_first$a;
    assign ind$heard_heard_v = fifo$out_first$b;
    assign respond__RDY_internal = (fifo$out_first__RDY & fifo$out_deq__RDY) & ind$heard__RDY;
    assign rule_ready[0] = respond__RDY_internal;
    assign say__RDY = say__RDY_internal;
    assign temp$a = say_meth;
    assign temp$b = say_v;
endmodule 

