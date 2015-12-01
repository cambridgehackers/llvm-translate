module l_class_OC_EchoTest (
    input CLK,
    input nRST,
    input rule_drive__ENA,
    output rule_drive__RDY);

  ;
   reg[31:0] x;
  always @( posedge CLK) begin
    if (!nRST) begin
    end
    else begin
        // Method: rule_drive
        if (rule_drive__ENA) begin
        ((*(echo->fifo)).enq);
        end; // End of rule_drive

        // Method: rule_drive__RDY
    rule_drive__RDY_tmp__1 = ((*(echo->fifo)).enq__RDY);
        rule_drive__RDY = rule_drive__RDY_tmp__1;

    end; // nRST
  end; // always @ (posedge CLK)
endmodule 

