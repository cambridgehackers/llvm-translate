module l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 (
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
    RDY_tmp__1 = ((*((module)->fifo))[4]);
    RDY_tmp__2 = ((*((module)->fifo))[6]);
        RDY = (RDY_tmp__1 & RDY_tmp__2);

        // Method: ENA
        if (ENA__ENA) begin
        ((*((module)->fifo))[5]);
        ENA_call = ((*((module)->fifo))[7]);
        _ZN14EchoIndication4echoEi;
        end; // End of ENA

    end; // nRST
  end; // always @ (posedge CLK)
endmodule 

