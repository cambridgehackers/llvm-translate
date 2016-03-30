`ifndef __l_class_OC_EchoRequestOutput_VH__
`define __l_class_OC_EchoRequestOutput_VH__

`define l_class_OC_EchoRequestOutput_RULE_COUNT (0)

//METAWRITE; request$say; :ind$data$say$meth;:ind$data$say$v;
//METAINVOKE; request$say; :pipe$enq;
//METAEXCLUSIVE; request$say; request$say2
//METAWRITE; request$say2; :ind$data$say2$meth;:ind$data$say2$v;
//METAINVOKE; request$say2; :pipe$enq;
//METAGUARD; request$say2; pipe$enq__RDY;
//METAGUARD; request$say; pipe$enq__RDY;
//METAEXTERNAL; pipe; l_class_OC_PipeIn;
`endif
