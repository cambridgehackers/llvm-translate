
`include "l_class_OC_EchoIndicationOutput.vh"

module l_class_OC_EchoIndicationOutput (
    input CLK,
    input nRST,
    input [`l_class_OC_EchoIndicationOutput_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_EchoIndicationOutput_RULE_COUNT:0]rule_ready);
endmodule 

