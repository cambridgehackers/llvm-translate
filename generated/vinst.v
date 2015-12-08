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
