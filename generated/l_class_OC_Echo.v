module l_class_OC_Echo (
    input CLK,
    input nRST,
    input echoReq__ENA,
    input [31:0]echoReq_v,
    output echoReq__RDY,
    input rule_respond__ENA,
    output rule_respond__RDY,
    output   unsigned VERILOG_long long ind_unused_data_to_flag_indication_echo,
    output ind_echo__ENA,
    output [31:0]ind_echo_v);
    l_class_OC_Fifo1 fifo (
        fifo_CLK,
        fifo_nRST,
        fifo_deq__ENA,
        fifo_deq__RDY,
        fifo_enq__ENA,
        fifo_enq_v,
        fifo_enq__RDY,
        fifo_first,
        fifo_first__RDY);
//INTERNAL l_class_OC_Fifo1 fifo
   reg[31:0] pipetemp;
    always @( posedge CLK) begin
      if (!nRST) begin
      end
      else begin
        // Method: echoReq
        if (echoReq__ENA) begin
        fifo_enq__ENA = 1;
            fifo_enq_v = echoReq_v;
        end; // End of echoReq

        // Method: echoReq__RDY
            echoReq__RDY = (fifo_enq__RDY);

        // Method: rule_respond
        if (rule_respond__ENA) begin
        fifo_deq__ENA = 1;
        ind_echo__ENA = 1;
            ind_echo_v = (fifo_first);
        end; // End of rule_respond

        // Method: rule_respond__RDY
            rule_respond__RDY = (fifo_first__RDY) & (fifo_deq__RDY);

      end; // nRST
    end; // always @ (posedge CLK)
endmodule 

//RDY:            echoReq__RDY = (fifo_enq__RDY);
//RDY:            rule_respond__RDY = (fifo_first__RDY) & (fifo_deq__RDY);
//RULE:   echoReq__ENA
//RULE:   rule_respond__ENA
//INTERNAL l_class_OC_Fifo1 fifo
//READ echoReq: :echoReq_v
//WRITE echoReq: :fifo_enq_v
//READ rule_respond: :fifo_first
//WRITE rule_respond: :ind_echo_v
