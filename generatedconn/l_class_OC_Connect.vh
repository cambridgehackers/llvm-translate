`ifndef __l_class_OC_Connect_VH__
`define __l_class_OC_Connect_VH__

`include "l_class_OC_Echo.vh"
`include "l_class_OC_EchoIndicationInput.vh"
`include "l_class_OC_EchoIndicationOutput.vh"
`include "l_class_OC_EchoRequestInput.vh"
`include "l_class_OC_EchoRequestOutput.vh"
`include "l_class_OC_Fifo1_OC_2.vh"
`define l_class_OC_Connect_RULE_COUNT (1 + `l_class_OC_Fifo1_OC_2_RULE_COUNT + `l_class_OC_EchoIndicationOutput_RULE_COUNT + `l_class_OC_EchoRequestInput_RULE_COUNT + `l_class_OC_Echo_RULE_COUNT + `l_class_OC_EchoRequestOutput_RULE_COUNT + `l_class_OC_EchoIndicationInput_RULE_COUNT)

//METAINTERNAL; fifo; l_class_OC_Fifo1_OC_2;
//METAEXTERNAL; ind; l_class_OC_ConnectIndication;
//METAINTERNAL; lEchoIndicationOutput; l_class_OC_EchoIndicationOutput;
//METAINTERNAL; lEchoRequestInput; l_class_OC_EchoRequestInput;
//METAINTERNAL; lEcho; l_class_OC_Echo;
//METAINTERNAL; lEchoRequestOutput_test; l_class_OC_EchoRequestOutput;
//METAINTERNAL; lEchoIndicationInput_test; l_class_OC_EchoIndicationInput;
//METAINVOKE; respond; :;fifo$out_first:;fifo$out_deq:;ind$heard;
//METAGUARD; respond__RDY; (fifo$out_first__RDY & fifo$out_deq__RDY) & ind$heard__RDY;
//METAINVOKE; say; :;lEchoRequestOutput_test$request_say;
//METAGUARD; say__RDY; lEchoRequestOutput_test$request_say__RDY;
`endif
