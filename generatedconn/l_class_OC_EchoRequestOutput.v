
`include "l_class_OC_EchoRequestOutput.vh"

module l_class_OC_EchoRequestOutput (
    input CLK,
    input nRST,
    input [`l_class_OC_EchoRequestOutput_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_EchoRequestOutput_RULE_COUNT:0]rule_ready);
endmodule 

