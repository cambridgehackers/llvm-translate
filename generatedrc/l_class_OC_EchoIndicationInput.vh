`ifndef __l_class_OC_EchoIndicationInput_VH__
`define __l_class_OC_EchoIndicationInput_VH__

`define l_class_OC_EchoIndicationInput_RULE_COUNT (1)

//METARULES; input_rule
//METAEXTERNAL; indication; l_class_OC_EchoIndication;
//METAGUARD; pipe$enq__RDY; (busy_delay != 0) ^ 1;
//METAGUARD; input_rule__RDY; (busy_delay != 0) & indication$heard__RDY;
//METAREAD; pipe$enq; enq_v$tag == 1:enq_v$data$heard$meth;enq_v$tag == 1:enq_v$data$heard$v;:enq_v$tag;:enq_v$tag;:enq_v$tag;
//METAWRITE; pipe$enq; enq_v$tag == 1:meth_delay;enq_v$tag == 1:v_delay;enq_v$tag == 1:busy_delay;
//METAREAD; input_rule; :meth_delay;:v_delay;
//METAWRITE; input_rule; :busy_delay;
//METAINVOKE; input_rule; :indication$heard;
`endif
