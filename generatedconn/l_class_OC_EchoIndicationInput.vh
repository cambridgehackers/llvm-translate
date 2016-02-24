`ifndef __l_class_OC_EchoIndicationInput_VH__
`define __l_class_OC_EchoIndicationInput_VH__

`define l_class_OC_EchoIndicationInput_RULE_COUNT (0)

//METAEXTERNAL; request; l_class_OC_EchoIndication;
//METAREAD; enq; :;enq_v$tag:enq_v$tag == 1;enq_v$data$heard$meth:;enq_v$tag:enq_v$tag == 1;enq_v$data$heard$v:;enq_v$tag;
//METAINVOKE; enq; :enq_v$tag == 1;request$heard;
//METAGUARD; enq__RDY; request$heard__RDY;
`endif
