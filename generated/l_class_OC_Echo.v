module l_class_OC_Echo (
    input CLK,
    input nRST,
    output rule_respond__RDY,
    input rule_respond__ENA,
    output   VERILOG_class l_class_OC_EchoIndication ind);
  VERILOG_class l_class_OC_Fifo1 fifo;
   reg[31:0] pipetemp;
  always @( posedge CLK) begin
    if (!nRST) begin
    end
    else begin
        // Method: rule_respond__RDY
    rule_respond__RDY_tmp__1 = fifo_deq__RDY;
    rule_respond__RDY_tmp__2 = fifo_first__RDY;
        rule_respond__RDY = (rule_respond__RDY_tmp__1 & rule_respond__RDY_tmp__2);

        // Method: rule_respond
        if (rule_respond__ENA) begin
        fifo_deq__ENA = 1;
        rule_respond_call = fifo_first;
        ind_echo__ENA = 1;
            ind_echo_v = rule_respond_call;
        end; // End of rule_respond

    end; // nRST
  end; // always @ (posedge CLK)
endmodule 

