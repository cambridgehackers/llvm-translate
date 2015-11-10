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
    RDY_tmp__1 = ((*((module)->fifo)).deq__RDY);
    RDY_tmp__2 = ((*((module)->fifo)).first__RDY);
        RDY = (RDY_tmp__1 & RDY_tmp__2);

        // Method: ENA
        if (ENA__ENA) begin
        ((*((module)->fifo)).deq);
        ENA_call = ((*((module)->fifo)).first);
        _ZN14EchoIndication4echoEi;
        end; // End of ENA

    end; // nRST
  end; // always @ (posedge CLK)
endmodule 

