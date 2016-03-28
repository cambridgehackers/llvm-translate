`ifndef __l_class_OC_EchoIndicationInput_VH__
`define __l_class_OC_EchoIndicationInput_VH__

`define l_class_OC_EchoIndicationInput_RULE_COUNT (1)

//METAREAD; pipe$enq; enq_v$tag == 1:enq_v$data$heard$meth;enq_v$tag == 1:enq_v$data$heard$v;:enq_v$tag;
//METAWRITE; pipe$enq; enq_v$tag == 1:busy_delayi;enq_v$tag == 1:meth_delayi;enq_v$tag == 1:v_delayi;
//METAEXCLUSIVE; pipe$enq; :input_rule
//METAGUARD; pipe$enq; (busy_delayi != 0) ^ 1;
//METAREAD; input_rule; :meth_delayi;:v_delayi;
//METAWRITE; input_rule; :busy_delayi;
//METAINVOKE; input_rule; :indication$heard;
//METAGUARD; input_rule; (busy_delayi != 0) & indication$heard__RDY;
//METARULES; input_rule
//METAEXTERNAL; indication; l_class_OC_EchoIndication;
`endif
