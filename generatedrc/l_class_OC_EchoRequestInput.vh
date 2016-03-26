`ifndef __l_class_OC_EchoRequestInput_VH__
`define __l_class_OC_EchoRequestInput_VH__

`define l_class_OC_EchoRequestInput_RULE_COUNT (0)

//METAEXTERNAL; request; l_class_OC_EchoRequest;
//METAREAD; pipe$enq; enq_v$tag == 1:enq_v$data$say$meth;enq_v$tag == 1:enq_v$data$say$v;enq_v$tag == 2:enq_v$data$say2$meth;enq_v$tag == 2:enq_v$data$say2$v;:enq_v$tag;
//METAINVOKE; pipe$enq; enq_v$tag == 1:request$say;enq_v$tag == 2:request$say2;
//METAGUARD; pipe$enq__RDY; request$say__RDY & request$say2__RDY;
`endif
