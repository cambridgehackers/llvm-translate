module l_class_OC_Echo (
    input CLK,
    input nRST,
    input echoReq__ENA,
    input [31:0]echoReq$v,
    output echoReq__RDY,
    input respond_rule__ENA,
    output respond_rule__RDY,
    output   unsigned VERILOG_long long ind$unused_data_to_flag_indication_echo,
    output ind$echo__ENA,
    output [31:0]ind$echo$v);
    l_class_OC_Fifo1 fifo (
        fifo$CLK,
        fifo$nRST,
        fifo$deq__ENA,
        fifo$deq__RDY,
        fifo$enq__ENA,
        fifo$enq$v,
        fifo$enq__RDY,
        fifo$first,
        fifo$first__RDY);
   reg[31:0] pipetemp;
    always @( posedge CLK) begin
      if (!nRST) begin
      end
      else begin
        // Method: echoReq
        if (echoReq__ENA) begin
        fifo$enq__ENA = 1;
            fifo$enq$v = echoReq$v;
        end; // End of echoReq

        // Method: echoReq__RDY
             echoReq__RDY =         (fifo$enq__RDY);

        // Method: respond_rule
        if (respond_rule__ENA) begin
        fifo$deq__ENA = 1;
        ind$echo__ENA = 1;
            ind$echo$v = (fifo$first);
        end; // End of respond_rule

        // Method: respond_rule__RDY
             respond_rule__RDY =         (fifo$first__RDY) & (fifo$deq__RDY);

      end; // nRST
    end; // always @ (posedge CLK)
endmodule 

//RDY echoReq__RDY:         (fifo$enq__RDY);
//RDY respond_rule__RDY:         (fifo$first__RDY) & (fifo$deq__RDY);
//INTERNAL fifo: l_class_OC_Fifo1
//EXTERNAL ind: l_class_OC_EchoIndication
//READ echoReq: (true):echoReq$v
//WRITE echoReq: (true):fifo$enq$v
//READ respond_rule: (true):fifo$first
//WRITE respond_rule: (true):ind$echo$v
