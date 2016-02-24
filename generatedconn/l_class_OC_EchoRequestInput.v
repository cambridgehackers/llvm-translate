
`include "l_class_OC_EchoRequestInput.vh"

module l_class_OC_EchoRequestInput (
    input CLK,
    input nRST,
    input pipe_enq__ENA,
    input [95:0]pipe_enq_v,
    output pipe_enq__RDY,
    input [`l_class_OC_EchoRequestInput_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_EchoRequestInput_RULE_COUNT:0]rule_ready);
    wire pipe_enq__RDY_internal;
    wire pipe_enq__ENA_internal = pipe_enq__ENA && pipe_enq__RDY_internal;
    assign pipe_enq__RDY = pipe_enq__RDY_internal;
    assign pipe_enq__RDY_internal = request$say__RDY;
    assign request$say__ENA = pipe_enq__ENA_internal & pipe_enq_v$tag == 1;
    assign request$say_meth = pipe_enq_v$data$say$meth;
    assign request$say_v = pipe_enq_v$data$say$v;
endmodule 

