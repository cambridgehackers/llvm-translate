`ifndef __l_class_OC_EchoIndicationInput_VH__
`define __l_class_OC_EchoIndicationInput_VH__

`define l_class_OC_EchoIndicationInput_RULE_COUNT (1)

//METARULES; input_rule
//METAEXTERNAL; indication; l_class_OC_EchoIndication;
//METAGUARD; pipe$enq__RDY; (busy_delayi != 0) ^ 1;
//METAGUARD; input_rule__RDY; (busy_delayi != 0) & indication$heard__RDY;
//METAREAD; pipe$enq; enq_v$tag == 1:enq_v$data$heard$meth;enq_v$tag == 1:enq_v$data$heard$v;:enq_v$tag;:enq_v$tag;:enq_v$tag;
//METAWRITE; pipe$enq; enq_v$tag == 1:meth_delayi;enq_v$tag == 1:v_delayi;enq_v$tag == 1:busy_delayi;
//METAREAD; input_rule; :meth_delayi;:v_delayi;
//METAWRITE; input_rule; :busy_delayi;
//METAINVOKE; input_rule; :indication$heard;
`endif
