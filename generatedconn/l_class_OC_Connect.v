
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
    wire say__RDY_internal;
    wire say__ENA_internal = say__ENA && say__RDY_internal;
    wire lEchoIndicationOutput$heard__ENA;
    wire [31:0]lEchoIndicationOutput$heard_meth;
    wire [31:0]lEchoIndicationOutput$heard_v;
    wire lEchoIndicationOutput$heard__RDY;
    l_class_OC_EchoIndicationOutput lEchoIndicationOutput (
        CLK,
        nRST,
        lEchoIndicationOutput$heard__ENA,
        lEchoIndicationOutput$heard_meth,
        lEchoIndicationOutput$heard_v,
        lEchoIndicationOutput$heard__RDY,
        rule_enable[0:`l_class_OC_EchoIndicationOutput_RULE_COUNT],
        rule_ready[0:`l_class_OC_EchoIndicationOutput_RULE_COUNT]);
    wire lEchoRequestInput$enq__ENA;
    wire [95:0]lEchoRequestInput$enq_v;
    wire lEchoRequestInput$enq__RDY;
    l_class_OC_EchoRequestInput lEchoRequestInput (
        CLK,
        nRST,
        lEchoRequestInput$enq__ENA,
        lEchoRequestInput$enq_v,
        lEchoRequestInput$enq__RDY,
        rule_enable[0 + `l_class_OC_EchoIndicationOutput_RULE_COUNT:`l_class_OC_EchoRequestInput_RULE_COUNT],
        rule_ready[0 + `l_class_OC_EchoIndicationOutput_RULE_COUNT:`l_class_OC_EchoRequestInput_RULE_COUNT]);
    l_class_OC_Echo lEcho (
        CLK,
        nRST,
        rule_enable[0 + `l_class_OC_EchoIndicationOutput_RULE_COUNT + `l_class_OC_EchoRequestInput_RULE_COUNT:`l_class_OC_Echo_RULE_COUNT],
        rule_ready[0 + `l_class_OC_EchoIndicationOutput_RULE_COUNT + `l_class_OC_EchoRequestInput_RULE_COUNT:`l_class_OC_Echo_RULE_COUNT]);
    wire lEchoRequestOutput_test$say__ENA;
    wire [31:0]lEchoRequestOutput_test$say_meth;
    wire [31:0]lEchoRequestOutput_test$say_v;
    wire lEchoRequestOutput_test$say__RDY;
    l_class_OC_EchoRequestOutput lEchoRequestOutput_test (
        CLK,
        nRST,
        lEchoRequestOutput_test$say__ENA,
        lEchoRequestOutput_test$say_meth,
        lEchoRequestOutput_test$say_v,
        lEchoRequestOutput_test$say__RDY,
        rule_enable[0 + `l_class_OC_EchoIndicationOutput_RULE_COUNT + `l_class_OC_EchoRequestInput_RULE_COUNT + `l_class_OC_Echo_RULE_COUNT:`l_class_OC_EchoRequestOutput_RULE_COUNT],
        rule_ready[0 + `l_class_OC_EchoIndicationOutput_RULE_COUNT + `l_class_OC_EchoRequestInput_RULE_COUNT + `l_class_OC_Echo_RULE_COUNT:`l_class_OC_EchoRequestOutput_RULE_COUNT]);
    wire lEchoIndicationInput_test$enq__ENA;
    wire [95:0]lEchoIndicationInput_test$enq_v;
    wire lEchoIndicationInput_test$enq__RDY;
    l_class_OC_EchoIndicationInput lEchoIndicationInput_test (
        CLK,
        nRST,
        lEchoIndicationInput_test$enq__ENA,
        lEchoIndicationInput_test$enq_v,
        lEchoIndicationInput_test$enq__RDY,
        rule_enable[0 + `l_class_OC_EchoIndicationOutput_RULE_COUNT + `l_class_OC_EchoRequestInput_RULE_COUNT + `l_class_OC_Echo_RULE_COUNT + `l_class_OC_EchoRequestOutput_RULE_COUNT:`l_class_OC_EchoIndicationInput_RULE_COUNT],
        rule_ready[0 + `l_class_OC_EchoIndicationOutput_RULE_COUNT + `l_class_OC_EchoRequestInput_RULE_COUNT + `l_class_OC_Echo_RULE_COUNT + `l_class_OC_EchoRequestOutput_RULE_COUNT:`l_class_OC_EchoIndicationInput_RULE_COUNT]);
    assign lEchoRequestOutput_test$request_say__ENA = say__ENA_internal;
    assign lEchoRequestOutput_test$request_say_meth = say_meth;
    assign lEchoRequestOutput_test$request_say_v = say_v;
    assign say__RDY = say__RDY_internal;
    assign say__RDY_internal = lEchoRequestOutput_test$request_say__RDY;
endmodule 

