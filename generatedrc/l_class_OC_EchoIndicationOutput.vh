`ifndef __l_class_OC_EchoIndicationOutput_VH__
`define __l_class_OC_EchoIndicationOutput_VH__

`include "l_struct_OC_EchoIndication_data.vh"
`define l_class_OC_EchoIndicationOutput_RULE_COUNT (2)

//METARULES; output_rulee; output_ruleo
//METAEXTERNAL; pipe; l_class_OC_PipeIn_OC_0;
//METAGUARD; heard__RDY; (ind_busy != 0) ^ 1;
//METAGUARD; output_rulee__RDY; (((ind_busy != 0) & (even != 0)) != 0) & pipe$enq__RDY;
//METAGUARD; output_ruleo__RDY; (((ind_busy != 0) & (even == 0)) != 0) & pipe$enq__RDY;
//METAREAD; heard; :even;(even != 0) ^ 1:even;(even != 0) ^ 1:even;(even != 0) ^ 1:even;:even;:even;:even;
//METAWRITE; heard; even != 0:ind1$tag;even != 0:ind1$data$heard$meth;even != 0:ind1$data$heard$v;(even != 0) ^ 1:ind0$tag;(even != 0) ^ 1:ind0$data$heard$meth;(even != 0) ^ 1:ind0$data$heard$v;:ind_busy;:even;
//METAWRITE; output_rulee; :ind_busy;
//METAINVOKE; output_rulee; :pipe$enq;
//METAWRITE; output_ruleo; :ind_busy;
//METAINVOKE; output_ruleo; :pipe$enq;
`endif
