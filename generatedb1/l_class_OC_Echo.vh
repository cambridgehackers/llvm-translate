`ifndef __l_class_OC_Echo_VH__
`define __l_class_OC_Echo_VH__

`define l_class_OC_Echo_RULE_COUNT (2)

//METAEXCLUSIVE; delay_rule; request$say; request$say2; respond_rule
//METABEFORE; delay_rule; :request$say; :request$say2
//METAGUARD; delay_rule; ((busy != 0) & (busy_delay == 0)) != 0;
//METAINVOKE; respond_rule; :indication$heard;
//METABEFORE; respond_rule; :delay_rule
//METAGUARD; respond_rule; (busy_delay != 0) & indication$heard__RDY;
//METAGUARD; request$say2; (busy != 0) ^ 1;
//METAGUARD; request$say; (busy != 0) ^ 1;
//METABEFORE; x2y; :y2x
//METAGUARD; x2y; 1;
//METABEFORE; y2x; :x2y
//METAGUARD; y2x; 1;
//METAGUARD; y2xnull; 1;
//METARULES; delay_rule; respond_rule
//METAEXTERNAL; indication; l_class_OC_EchoIndication;
`endif
