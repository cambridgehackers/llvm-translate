module l_class_OC_EchoTest (
    input CLK,
    input nRST,
    output rule_drive__RDY,
    input rule_drive__ENA,
    output   VERILOG_class l_class_OC_Echo echo);
   reg[31:0] x;
  always @( posedge CLK) begin
    if (!nRST) begin
    end
    else begin
        // Method: rule_drive__RDY
        rule_drive__RDY = (echo->fifo_enq__RDY);

        // Method: rule_drive
        if (rule_drive__ENA) begin
        echo->fifo_enq__ENA = 1;
            echo->fifo_enq_v = 22;
        end; // End of rule_drive

    end; // nRST
  end; // always @ (posedge CLK)
endmodule 

