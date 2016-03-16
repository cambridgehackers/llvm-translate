
`include "l_class_OC_foo.vh"

module l_class_OC_foo (
    input CLK,
    input nRST,
    input [`l_class_OC_foo_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_foo_RULE_COUNT:0]rule_ready);
endmodule 

