l_class_OC_Fifo1 (&echo->fifo) (
    (&echo->fifo)CLK,
    (&echo->fifo)nRST,
    (&echo->fifo)deq__RDY,
    (&echo->fifo)enq__RDY,
    (&echo->fifo)enq__ENA,
    (&echo->fifo)[31:0]enq_v,
    (&echo->fifo)deq__ENA,
    (&echo->fifo)first__RDY,
    (&echo->fifo)[31:0]first);
l_class_OC_Fifo1 (&fifo) (
    (&fifo)CLK,
    (&fifo)nRST,
    (&fifo)deq__RDY,
    (&fifo)enq__RDY,
    (&fifo)enq__ENA,
    (&fifo)[31:0]enq_v,
    (&fifo)deq__ENA,
    (&fifo)first__RDY,
    (&fifo)[31:0]first);
l_class_OC_EchoIndication (ind) (
    (ind)CLK,
    (ind)nRST,
    (ind)echo__ENA,
    (ind)[31:0]echo_v);
l_class_OC_Fifo1 echoTest$$echo$$fifo (
    echoTest$$echo$$fifoCLK,
    echoTest$$echo$$fifonRST,
    echoTest$$echo$$fifodeq__RDY,
    echoTest$$echo$$fifoenq__RDY,
    echoTest$$echo$$fifoenq__ENA,
    echoTest$$echo$$fifo[31:0]enq_v,
    echoTest$$echo$$fifodeq__ENA,
    echoTest$$echo$$fifofirst__RDY,
    echoTest$$echo$$fifo[31:0]first);
l_class_OC_EchoIndication echoTest$$echo$$ind (
    echoTest$$echo$$indCLK,
    echoTest$$echo$$indnRST,
    echoTest$$echo$$indecho__ENA,
    echoTest$$echo$$ind[31:0]echo_v);
