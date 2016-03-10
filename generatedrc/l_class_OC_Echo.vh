`ifndef __l_class_OC_Echo_VH__
`define __l_class_OC_Echo_VH__

`define l_class_OC_Echo_RULE_COUNT (1)

//METARULES; respond_rule
//METAEXTERNAL; indication; l_class_OC_EchoIndication;
//METAGUARD; respond_rule__RDY; (busy != 0) & indication$heard__RDY;
//METAGUARD; say__RDY; (busy != 0) ^ 1;
//METAREAD; respond_rule; :meth_temp;:v_temp;
//METAWRITE; respond_rule; :busy;
//METAINVOKE; respond_rule; :indication$heard;
//METAWRITE; say; :meth_temp;:v_temp;:busy;
`endif
