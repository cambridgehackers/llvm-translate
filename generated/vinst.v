    l_class_OC_Fifo1 echoTest$$echo$$fifo (
        echoTest$$echo$$fifo_CLK,
        echoTest$$echo$$fifo_nRST,
        echoTest$$echo$$fifo_enq__ENA,
        echoTest$$echo$$fifo_enq_v,
        echoTest$$echo$$fifo_deq__RDY,
        echoTest$$echo$$fifo_deq__ENA,
        echoTest$$echo$$fifo_first__RDY,
        echoTest$$echo$$fifo_first,
        echoTest$$echo$$fifo_notEmpty);
    l_class_OC_EchoIndication echoTest$$echo$$ind (
        echoTest$$echo$$ind_CLK,
        echoTest$$echo$$ind_nRST,
        echoTest$$echo$$ind_echo__ENA,
        echoTest$$echo$$ind_echo_v);
    l_class_OC_Fifo1 fifo (
        fifo_CLK,
        fifo_nRST,
        fifo_enq__ENA,
        fifo_enq_v,
        fifo_deq__RDY,
        fifo_deq__ENA,
        fifo_first__RDY,
        fifo_first,
        fifo_notEmpty);
    l_class_OC_EchoIndication ind (
        ind_CLK,
        ind_nRST,
        ind_echo__ENA,
        ind_echo_v);
    l_class_OC_Fifo1 this (
        this_CLK,
        this_nRST,
        this_enq__ENA,
        this_enq_v,
        this_deq__RDY,
        this_deq__ENA,
        this_first__RDY,
        this_first,
        this_notEmpty);
