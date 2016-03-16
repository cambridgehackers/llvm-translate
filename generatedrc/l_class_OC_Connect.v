
`include "l_class_OC_Connect.vh"

module l_class_OC_Connect (
    input CLK,
    input nRST,
    input [`l_class_OC_Connect_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_Connect_RULE_COUNT:0]rule_ready);
    wire lEIO$heard__ENA;
    wire [31:0]lEIO$heard_meth;
    wire [31:0]lEIO$heard_v;
    wire lEIO$heard__RDY;
    wire lEIO$output_rulee__ENA;
    wire lEIO$output_rulee__RDY;
    wire lEIO$output_ruleo__ENA;
    wire lEIO$output_ruleo__RDY;
    l_class_OC_EchoIndicationOutput lEIO (
        CLK,
        nRST,
        lEIO$heard__ENA,
        lEIO$heard_meth,
        lEIO$heard_v,
        lEIO$heard__RDY,
        lEIO$pipe$enq__ENA,
        lEIO$pipe$enq_v,
        lEIO$pipe$enq__RDY,
        rule_enable[0:`l_class_OC_EchoIndicationOutput_RULE_COUNT],
        rule_ready[0:`l_class_OC_EchoIndicationOutput_RULE_COUNT]);
    wire lERI$enq__ENA;
    wire [95:0]lERI$enq_v;
    wire lERI$enq__RDY;
    l_class_OC_EchoRequestInput lERI (
        CLK,
        nRST,
        lERI$enq__ENA,
        lERI$enq_v,
        lERI$enq__RDY,
        lERI$request$say__ENA,
        lERI$request$say_meth,
        lERI$request$say_v,
        lERI$request$say__RDY,
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
    wire lERO_test$say__ENA;
    wire [31:0]lERO_test$say_meth;
    wire [31:0]lERO_test$say_v;
    wire lERO_test$say__RDY;
    l_class_OC_EchoRequestOutput lERO_test (
        CLK,
        nRST,
        lERO_test$say__ENA,
        lERO_test$say_meth,
        lERO_test$say_v,
        lERO_test$say__RDY,
        lERO_test$pipe$enq__ENA,
        lERO_test$pipe$enq_v,
        lERO_test$pipe$enq__RDY,
        rule_enable[0 + `l_class_OC_EchoIndicationOutput_RULE_COUNT + `l_class_OC_EchoRequestInput_RULE_COUNT + `l_class_OC_Echo_RULE_COUNT:`l_class_OC_EchoRequestOutput_RULE_COUNT],
        rule_ready[0 + `l_class_OC_EchoIndicationOutput_RULE_COUNT + `l_class_OC_EchoRequestInput_RULE_COUNT + `l_class_OC_Echo_RULE_COUNT:`l_class_OC_EchoRequestOutput_RULE_COUNT]);
    wire lEII_test$enq__ENA;
    wire [95:0]lEII_test$enq_v;
    wire lEII_test$enq__RDY;
    wire lEII_test$input_rule__ENA;
    wire lEII_test$input_rule__RDY;
    l_class_OC_EchoIndicationInput lEII_test (
        CLK,
        nRST,
        lEII_test$enq__ENA,
        lEII_test$enq_v,
        lEII_test$enq__RDY,
        lEII_test$indication$heard__ENA,
        lEII_test$indication$heard_meth,
        lEII_test$indication$heard_v,
        lEII_test$indication$heard__RDY,
        rule_enable[0 + `l_class_OC_EchoIndicationOutput_RULE_COUNT + `l_class_OC_EchoRequestInput_RULE_COUNT + `l_class_OC_Echo_RULE_COUNT + `l_class_OC_EchoRequestOutput_RULE_COUNT:`l_class_OC_EchoIndicationInput_RULE_COUNT],
        rule_ready[0 + `l_class_OC_EchoIndicationOutput_RULE_COUNT + `l_class_OC_EchoRequestInput_RULE_COUNT + `l_class_OC_Echo_RULE_COUNT + `l_class_OC_EchoRequestOutput_RULE_COUNT:`l_class_OC_EchoIndicationInput_RULE_COUNT]);
endmodule 

