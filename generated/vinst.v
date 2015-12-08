    l_class_OC_Fifo1 Vthis->fifo (
        Vthis->fifo_CLK,
        Vthis->fifo_nRST,
        Vthis->fifo_enq__RDY,
        Vthis->fifo_enq__ENA,
        Vthis->fifo_enq_v,
        Vthis->fifo_deq__RDY,
        Vthis->fifo_deq__ENA,
        Vthis->fifo_first__RDY,
        Vthis->fifo_first);
    l_class_OC_EchoIndication Vthis->ind (
        Vthis->ind_CLK,
        Vthis->ind_nRST,
        Vthis->ind_echo__ENA,
        Vthis->ind_echo_v);
    l_class_OC_Fifo1 fifo (
        fifo_CLK,
        fifo_nRST,
        fifo_enq__RDY,
        fifo_enq__ENA,
        fifo_enq_v,
        fifo_deq__RDY,
        fifo_deq__ENA,
        fifo_first__RDY,
        fifo_first);
    l_class_OC_EchoIndication ind (
        ind_CLK,
        ind_nRST,
        ind_echo__ENA,
        ind_echo_v);
