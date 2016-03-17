`ifndef __l_class_OC_EchoIndicationOutput_VH__
`define __l_class_OC_EchoIndicationOutput_VH__

`define l_class_OC_EchoIndicationOutput_RULE_COUNT (0)

//METAEXTERNAL; pipe; l_class_OC_PipeIn_OC_0;
//METAGUARD; indication$heard__RDY; pipe$enq__RDY;
//METAWRITE; indication$heard; :ind$data$heard$meth;:ind$data$heard$v;
//METAINVOKE; indication$heard; :pipe$enq;
`endif
