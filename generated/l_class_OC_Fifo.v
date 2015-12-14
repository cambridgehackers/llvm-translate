module l_class_OC_Fifo (
    input CLK,
    input nRST,
    input deq__ENA,
    output deq__RDY,
    input enq__ENA,
    input [31:0]enq$v,
    output enq__RDY,
    output [31:0]first,
    output first__RDY);
    always @( posedge CLK) begin
      if (!nRST) begin
      end
      else begin
        // Method: deq
        if (deq__ENA) begin
        end; // End of deq

        // Method: deq__RDY
             deq__RDY =         0;

        // Method: enq
        if (enq__ENA) begin
        end; // End of enq

        // Method: enq__RDY
             enq__RDY =         0;

        // Method: first
             first =         0;

        // Method: first__RDY
             first__RDY =         0;

      end; // nRST
    end; // always @ (posedge CLK)
endmodule 

//METAGUARD; deq__RDY;         0;
//METAGUARD; enq__RDY;         0;
//METAGUARD; first__RDY;         0;
