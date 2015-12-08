//RDY:        echoReq__RDY = 1;
//RDY:        rule_respondexport__RDY = (this_echoReq__RDY);
//RDY:        rule_respond__RDY = (fifo_deq__RDY) & (fifo_first__RDY);
//RULE:   echoReq__ENA
//RULE:   rule_respondexport__ENA
//RULE:   rule_respond__ENA
module l_class_OC_Echo (
    input CLK,
    input nRST,
    output echoReq__RDY,
    input echoReq__ENA,
    input [31:0]echoReq_v,
    input rule_respondexport__ENA,
    output rule_respondexport__RDY,
    input rule_respond__ENA,
    output rule_respond__RDY,
    output   VERILOGunsigned VERILOG_long long ind_unused_data_to_flag_indication_echo,
    output ind_echo__ENA,
    output [31:0]ind_echo_v);
    l_class_OC_Fifo1 fifo (
        fifo_CLK,
        fifo_nRST,
        fifo_enq__ENA,
        fifo_enq_v,
        fifo_deq__RDY,
        fifo_deq__ENA,
        fifo_first__RDY,
        fifo_first,
        fifo_notEmpty);
   reg[31:0] pipetemp;
    always @( posedge CLK) begin
      if (!nRST) begin
      end
      else begin
        // Method: echoReq__RDY
        echoReq__RDY = 1;

        // Method: echoReq
        if (echoReq__ENA) begin
        fifo_enq__ENA = 1;
            fifo_enq_v = echoReq_v;
        end; // End of echoReq

        // Method: rule_respondexport
        if (rule_respondexport__ENA) begin
        this_echoReq__ENA = 1;
            this_echoReq_v = 99;
        end; // End of rule_respondexport

        // Method: rule_respondexport__RDY
        rule_respondexport__RDY = (this_echoReq__RDY);

        // Method: rule_respond
        if (rule_respond__ENA) begin
        fifo_deq__ENA = 1;
        ind_echo__ENA = 1;
            ind_echo_v = (fifo_first);
        end; // End of rule_respond

        // Method: rule_respond__RDY
        rule_respond__RDY = (fifo_deq__RDY) & (fifo_first__RDY);

      end; // nRST
    end; // always @ (posedge CLK)
endmodule 

