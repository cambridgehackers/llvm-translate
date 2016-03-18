`ifndef __l_class_OC_Echo_VH__
`define __l_class_OC_Echo_VH__

`define l_class_OC_Echo_RULE_COUNT (2)

//METARULES; delay_rule; respond_rule
//METAEXTERNAL; indication; l_class_OC_EchoIndication;
//METAGUARD; delay_rule__RDY; ((busy != 0) & (busy_delay == 0)) != 0;
//METAGUARD; respond_rule__RDY; (busy_delay != 0) & indication$heard__RDY;
//METAGUARD; request$say2__RDY; (busy != 0) ^ 1;
//METAGUARD; request$say__RDY; (busy != 0) ^ 1;
//METAREAD; delay_rule; :meth_temp;:v_temp;
//METAWRITE; delay_rule; :busy;:busy_delay;:meth_delay;:v_delay;
//METAREAD; respond_rule; :meth_delay;:v_delay;
//METAWRITE; respond_rule; :busy_delay;
//METAINVOKE; respond_rule; :indication$heard;
//METAWRITE; request$say; :meth_temp;:v_temp;:busy;
//METAWRITE; request$say2; :meth_temp;:v_temp;:busy;
`endif
