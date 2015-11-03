module l_class_OC_Fifo1 (
    input CLK,
    input nRST,
    output first__RDY,
    output enq__RDY,
    input enq__ENA,
    input [31:0]enq_v,
    output deq__RDY,
    input deq__ENA,
    output [31:0]first,
    output notFull);

   reg[31:0] element;
   reg full;
  always @( posedge CLK) begin
    if (!nRST) begin
    end
    else begin
        // Method: first__RDY
        first__RDY = ((full) ^ 1);

        // Method: enq__RDY
    enq__RDY_call = ((*(this))[10]);
        enq__RDY = enq__RDY_call;

        // Method: enq
        if (enq__ENA) begin
        element <= enq_v;
        full <= 1;
        end; // End of enq

        // Method: deq__RDY
        deq__RDY = ((full) ^ 1);

        // Method: deq
        if (deq__ENA) begin
        full <= 0;
        end; // End of deq

        // Method: first
        first = (element);

        // Method: notFull
        notFull = ((full) ^ 1);

    end; // nRST
  end; // always @ (posedge CLK)
endmodule 

