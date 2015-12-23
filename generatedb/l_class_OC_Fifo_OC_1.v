module l_class_OC_Fifo_OC_1 (
    input CLK,
    input nRST,
    input deq__ENA,
    output deq__RDY,
    input enq__ENA,
    input enq_v,
    output enq__RDY,
    output first,
    output first__RDY);
    wire deq__RDY_internal;
    wire deq__ENA_internal = deq__ENA && deq__RDY_internal;
    assign deq__RDY = deq__RDY_internal;
    wire enq__RDY_internal;
    wire enq__ENA_internal = enq__ENA && enq__RDY_internal;
    assign enq__RDY = enq__RDY_internal;
    assign deq__RDY_internal = 0;
    assign enq__RDY_internal = 0;
    assign first = 0;
    assign first__RDY_internal = 0;
endmodule 

//METAGUARD; deq__RDY;         0;
//METAGUARD; enq__RDY;         0;
//METAGUARD; first__RDY;         0;
