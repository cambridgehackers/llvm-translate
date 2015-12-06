l_class_OC_Fifo1 echo->fifo (
    echo->fifoCLK,
    echo->fifonRST,
    echo->fifodeq__RDY,
    echo->fifoenq__RDY,
    echo->fifoenq__ENA,
    echo->fifo[31:0]enq_v,
    echo->fifodeq__ENA,
    echo->fifofirst__RDY,
    echo->fifo[31:0]first);
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
l_class_OC_Fifo1 fifo (
    fifoCLK,
    fifonRST,
    fifodeq__RDY,
    fifoenq__RDY,
    fifoenq__ENA,
    fifo[31:0]enq_v,
    fifodeq__ENA,
    fifofirst__RDY,
    fifo[31:0]first);
l_class_OC_EchoIndication ind (
    indCLK,
    indnRST,
    indecho__ENA,
    ind[31:0]echo_v);
