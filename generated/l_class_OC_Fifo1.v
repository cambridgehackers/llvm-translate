//RDY:        deq__RDY = (this_notEmpty);
//RDY:        first__RDY = (this_notEmpty);
//RULE:   enq__ENA
//RULE:   deq__ENA
module l_class_OC_Fifo1 (
    input CLK,
    input nRST,
    input enq__ENA,
    input [31:0]enq_v,
    output deq__RDY,
    input deq__ENA,
    output first__RDY,
    output [31:0]first,
    output notEmpty);
   reg[31:0] element;
   reg full;
    always @( posedge CLK) begin
      if (!nRST) begin
      end
      else begin
        // Method: enq
        if (enq__ENA) begin
        element <= enq_v;
        full <= 1;
        end; // End of enq

        // Method: deq__RDY
        deq__RDY = (this_notEmpty);

        // Method: deq
        if (deq__ENA) begin
        full <= 0;
        end; // End of deq

        // Method: first__RDY
        first__RDY = (this_notEmpty);

        // Method: first
        first = (element);

        // Method: notEmpty
        notEmpty = (full);

      end; // nRST
    end; // always @ (posedge CLK)
endmodule 

