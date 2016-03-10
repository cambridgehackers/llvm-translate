`ifndef __l_class_OC_EchoRequestInput_VH__
`define __l_class_OC_EchoRequestInput_VH__

`define l_class_OC_EchoRequestInput_RULE_COUNT (0)

//METAEXTERNAL; request; l_class_OC_EchoRequest;
//METAGUARD; enq__RDY; request$say__RDY;
//METAREAD; enq; :enq_v$tag;enq_v$tag == 1:enq_v$data$say$meth;:enq_v$tag;enq_v$tag == 1:enq_v$data$say$v;:enq_v$tag;
//METAINVOKE; enq; enq_v$tag == 1:request$say;
`endif
