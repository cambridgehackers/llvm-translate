//RDY:        rule_drive__RDY = (echo->fifo_enq__RDY);
//RULE:   rule_drive__ENA
module l_class_OC_EchoTest (
    input CLK,
    input nRST,
    input rule_drive__ENA,
    output rule_drive__RDY,
    output   VERILOG_class l_class_OC_Fifo1 echo_fifo,
    output   VERILOG_class l_class_OC_EchoIndication *echo_ind,
    output    reg[31:0] echo_pipetemp,
    input echo_rule_respond__RDY,
    output echo_rule_respond__ENA);
   reg[31:0] x;
    always @( posedge CLK) begin
      if (!nRST) begin
      end
      else begin
        // Method: rule_drive
        if (rule_drive__ENA) begin
        echo->fifo_enq__ENA = 1;
            echo->fifo_enq_v = 22;
        end; // End of rule_drive

        // Method: rule_drive__RDY
        rule_drive__RDY = (echo->fifo_enq__RDY);

      end; // nRST
    end; // always @ (posedge CLK)
endmodule 

