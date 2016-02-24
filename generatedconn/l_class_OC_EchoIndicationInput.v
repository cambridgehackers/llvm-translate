
`include "l_class_OC_EchoIndicationInput.vh"

module l_class_OC_EchoIndicationInput (
    input CLK,
    input nRST,
    input pipe_enq__ENA,
    input [95:0]pipe_enq_v,
    output pipe_enq__RDY,
    input [`l_class_OC_EchoIndicationInput_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_EchoIndicationInput_RULE_COUNT:0]rule_ready);
    wire pipe_enq__RDY_internal;
    wire pipe_enq__ENA_internal = pipe_enq__ENA && pipe_enq__RDY_internal;
    assign pipe_enq__RDY = pipe_enq__RDY_internal;
    assign pipe_enq__RDY_internal = request$heard__RDY;
    assign request$heard__ENA = pipe_enq__ENA_internal & pipe_enq_v$tag == 1;
    assign request$heard_meth = pipe_enq_v$data$heard$meth;
    assign request$heard_v = pipe_enq_v$data$heard$v;
endmodule 

