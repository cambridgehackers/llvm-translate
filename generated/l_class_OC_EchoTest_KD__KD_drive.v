module l_class_OC_EchoTest_KD__KD_drive (
    input CLK,
    input nRST,
    output RDY,
    input ENA__ENA);

  ;
  always @( posedge CLK) begin
    if (!nRST) begin
    end
    else begin
        // Method: RDY
    RDY_tmp__1 = ((*(((module)->echo)->fifo))[2]);
        RDY = RDY_tmp__1;

        // Method: ENA
        if (ENA__ENA) begin
        ((*(((module)->echo)->fifo))[3]);
        end; // End of ENA

    end; // nRST
  end; // always @ (posedge CLK)
endmodule 

