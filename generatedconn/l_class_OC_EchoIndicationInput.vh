`ifndef __l_class_OC_EchoIndicationInput_VH__
`define __l_class_OC_EchoIndicationInput_VH__

`define l_class_OC_EchoIndicationInput_RULE_COUNT (0)

//METAREAD; pipe$enq; enq_v$tag == 1:enq_v$data$heard$meth;enq_v$tag == 1:enq_v$data$heard$v;:enq_v$tag;
//METAINVOKE; pipe$enq; enq_v$tag == 1:indication$heard;
//METAGUARD; pipe$enq; indication$heard__RDY;
//METAEXTERNAL; indication; l_class_OC_EchoIndication;
`endif
