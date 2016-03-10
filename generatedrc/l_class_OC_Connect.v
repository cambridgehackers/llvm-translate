
`include "l_class_OC_Connect.vh"

module l_class_OC_Connect (
    input CLK,
    input nRST,
    input [`l_class_OC_Connect_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_Connect_RULE_COUNT:0]rule_ready);
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
        lEchoIndicationOutput$pipe$enq__ENA,
        lEchoIndicationOutput$pipe$enq_v,
        lEchoIndicationOutput$pipe$enq__RDY,
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
        lEchoRequestInput$request$say__ENA,
        lEchoRequestInput$request$say_meth,
        lEchoRequestInput$request$say_v,
        lEchoRequestInput$request$say__RDY,
        rule_enable[0 + `l_class_OC_EchoIndicationOutput_RULE_COUNT:`l_class_OC_EchoRequestInput_RULE_COUNT],
        rule_ready[0 + `l_class_OC_EchoIndicationOutput_RULE_COUNT:`l_class_OC_EchoRequestInput_RULE_COUNT]);
    wire lEcho$delay_rule__ENA;
    wire lEcho$delay_rule__RDY;
    wire lEcho$respond_rule__ENA;
    wire lEcho$respond_rule__RDY;
    wire lEcho$say__ENA;
    wire [31:0]lEcho$say_meth;
    wire [31:0]lEcho$say_v;
    wire lEcho$say__RDY;
    l_class_OC_Echo lEcho (
        CLK,
        nRST,
        lEcho$say__ENA,
        lEcho$say_meth,
        lEcho$say_v,
        lEcho$say__RDY,
        lEcho$indication$heard__ENA,
        lEcho$indication$heard_meth,
        lEcho$indication$heard_v,
        lEcho$indication$heard__RDY,
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
        lEchoRequestOutput_test$pipe$enq__ENA,
        lEchoRequestOutput_test$pipe$enq_v,
        lEchoRequestOutput_test$pipe$enq__RDY,
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
endmodule 

