//RDY:        rule_respond__RDY = (fifo_deq__RDY) & (fifo_first__RDY);
//RULE:   rule_respond__ENA
module l_class_OC_Echo (
    input CLK,
    input nRST,
    output rule_respond__RDY,
    input rule_respond__ENA,
    output ind_echo__ENA,
    output [31:0]ind_echo_v);
    l_class_OC_Fifo1 fifo (
        fifo_CLK,
        fifo_nRST,
        fifo_deq__RDY,
        fifo_enq__RDY,
        fifo_enq__ENA,
        fifo_enq_v,
        fifo_deq__ENA,
        fifo_first__RDY,
        fifo_first);
   reg[31:0] pipetemp;
    always @( posedge CLK) begin
      if (!nRST) begin
      end
      else begin
        // Method: rule_respond__RDY
        rule_respond__RDY = (fifo_deq__RDY) & (fifo_first__RDY);

        // Method: rule_respond
        if (rule_respond__ENA) begin
        fifo_deq__ENA = 1;
        ind_echo__ENA = 1;
            ind_echo_v = (fifo_first);
        end; // End of rule_respond

      end; // nRST
    end; // always @ (posedge CLK)
endmodule 

