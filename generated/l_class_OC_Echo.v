module l_class_OC_Echo (
    input CLK,
    input nRST,
    output rule_respond__RDY,
    input rule_respond__ENA);
  VERILOG_class l_class_OC_Fifo1 fifo;
  VERILOG_class l_class_OC_EchoIndication *ind;
   reg[31:0] pipetemp;
  always @( posedge CLK) begin
    if (!nRST) begin
    end
    else begin
        // Method: rule_respond__RDY
    rule_respond__RDY_tmp__1 = (&fifo)deq__RDY;
    rule_respond__RDY_tmp__2 = (&fifo)first__RDY;
        rule_respond__RDY = (rule_respond__RDY_tmp__1 & rule_respond__RDY_tmp__2);

        // Method: rule_respond
        if (rule_respond__ENA) begin
        (&fifo)deq__ENA = 1;
        rule_respond_call = (&fifo)first;
        (ind)echo__ENA = 1;
            (ind)echo_v = rule_respond_call;
        end; // End of rule_respond

    end; // nRST
  end; // always @ (posedge CLK)
endmodule 

