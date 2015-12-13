module l_class_OC_Fifo1 (
    input CLK,
    input nRST,
    input deq__ENA,
    output deq__RDY,
    input enq__ENA,
    input [31:0]enq$v,
    output enq__RDY,
    output [31:0]first,
    output first__RDY);
   reg[31:0] element;
   reg full;
    always @( posedge CLK) begin
      if (!nRST) begin
      end
      else begin
        // Method: deq
        if (deq__ENA) begin
        full <= 0;
        end; // End of deq

        // Method: deq__RDY
             deq__RDY =         (full);

        // Method: enq
        if (enq__ENA) begin
        element <= enq$v;
        full <= 1;
        end; // End of enq

        // Method: enq__RDY
             enq__RDY =         ((full) ^ 1);

        // Method: first
             first =         (element);

        // Method: first__RDY
             first__RDY =         (full);

      end; // nRST
    end; // always @ (posedge CLK)
endmodule 

//RDY: deq__RDY;         (full);
//RDY: enq__RDY;         ((full) ^ 1);
//RDY: first__RDY;         (full);
//WRITE deq: :full
//READ enq: :enq$v
//WRITE enq: :element:full
//READ first: :element
