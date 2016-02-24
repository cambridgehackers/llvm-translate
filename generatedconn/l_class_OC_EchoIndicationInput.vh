`ifndef __l_class_OC_EchoIndicationInput_VH__
`define __l_class_OC_EchoIndicationInput_VH__

`define l_class_OC_EchoIndicationInput_RULE_COUNT (0)

//METAEXTERNAL; request; l_class_OC_EchoIndication;
//METAREAD; pipe_enq; :;pipe_enq_v$tag:pipe_enq_v$tag == 1;pipe_enq_v$data$heard$meth:;pipe_enq_v$tag:pipe_enq_v$tag == 1;pipe_enq_v$data$heard$v:;pipe_enq_v$tag;
//METAINVOKE; pipe_enq; :pipe_enq_v$tag == 1;request$heard;
//METAGUARD; pipe_enq__RDY; request$heard__RDY;
`endif
