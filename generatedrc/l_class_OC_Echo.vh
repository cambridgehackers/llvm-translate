`ifndef __l_class_OC_Echo_VH__
`define __l_class_OC_Echo_VH__

`define l_class_OC_Echo_RULE_COUNT (2)

//METAREAD; delay_rule; :meth_temp;:v_temp;
//METAWRITE; delay_rule; :busy;:busy_delay;:meth_delay;:v_delay;
//METAEXCLUSIVE; ((busy != 0) & (busy_delay == 0)) != 0:delay_rule; (busy != 0) ^ 1:request$say; (busy != 0) ^ 1:request$say2; (busy_delay != 0) & indication$heard__RDY:respond_rule
//METABEFORE; delay_rule; :request$say; :request$say2
//METAGUARD; delay_rule; ((busy != 0) & (busy_delay == 0)) != 0;
//METAREAD; respond_rule; :meth_delay;:v_delay;
//METAWRITE; respond_rule; :busy_delay;
//METAINVOKE; respond_rule; :indication$heard;
//METABEFORE; respond_rule; :delay_rule
//METAGUARD; respond_rule; (busy_delay != 0) & indication$heard__RDY;
//METAWRITE; request$say; :busy;:meth_temp;:v_temp;
//METAWRITE; request$say2; :busy;:meth_temp;:v_temp;
//METAGUARD; request$say2; (busy != 0) ^ 1;
//METAGUARD; request$say; (busy != 0) ^ 1;
//METARULES; delay_rule; respond_rule
//METAEXTERNAL; indication; l_class_OC_EchoIndication;
`endif
