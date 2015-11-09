module l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 (
    input CLK,
    input nRST,
    input ENA__ENA,
    output RDY);

  ;
  always @( posedge CLK) begin
    if (!nRST) begin
    end
    else begin
        // Method: ENA
        if (ENA__ENA) begin
        end; // End of ENA

        // Method: RDY
        RDY = 1;

    end; // nRST
  end; // always @ (posedge CLK)
endmodule 

