module l_class_OC_Echo (
    input CLK,
    input nRST,
    input rule_respond__ENA,
    output rule_respond__RDY);
  ;
  ;
   reg[31:0] pipetemp;
  always @( posedge CLK) begin
    if (!nRST) begin
    end
    else begin
        // Method: rule_respond
        if (rule_respond__ENA) begin
        (&fifo)deq__ENA = 1;
        rule_respond_call = (&fifo)first;
        _ZN14EchoIndication4echoEi;
        end; // End of rule_respond

        // Method: rule_respond__RDY
    rule_respond__RDY_tmp__1 = (&fifo)deq__RDY;
    rule_respond__RDY_tmp__2 = (&fifo)first__RDY;
        rule_respond__RDY = (rule_respond__RDY_tmp__1 & rule_respond__RDY_tmp__2);

    end; // nRST
  end; // always @ (posedge CLK)
endmodule 

