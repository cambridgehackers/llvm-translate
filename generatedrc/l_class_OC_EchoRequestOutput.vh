`ifndef __l_class_OC_EchoRequestOutput_VH__
`define __l_class_OC_EchoRequestOutput_VH__

`define l_class_OC_EchoRequestOutput_RULE_COUNT (0)

//METAINVOKE; request$say2; :pipe$enq;
//METAEXCLUSIVE; request$say2; request$say
//METAGUARD; request$say2; pipe$enq__RDY;
//METAINVOKE; request$say; :pipe$enq;
//METAGUARD; request$say; pipe$enq__RDY;
//METAEXTERNAL; pipe; l_class_OC_PipeIn;
`endif
