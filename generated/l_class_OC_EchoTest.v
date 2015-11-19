module l_class_OC_EchoTest (
    input CLK,
    input nRST,
    output drive__RDY,
    input drive__ENA__ENA);

  ;
   reg[31:0] x;
  always @( posedge CLK) begin
    if (!nRST) begin
    end
    else begin
        // Method: drive__RDY
    drive__RDY_tmp__1 = ((*((echo)->fifo)).enq__RDY);
        drive__RDY = drive__RDY_tmp__1;

        // Method: drive__ENA
        if (drive__ENA__ENA) begin
        ((*((echo)->fifo)).enq);
        end; // End of drive__ENA

    end; // nRST
  end; // always @ (posedge CLK)
endmodule 

