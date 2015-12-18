module l_class_OC_Echo (
    input CLK,
    input nRST,
    input echoReq__ENA,
    input [31:0]echoReq_v,
    output echoReq__RDY,
    input respond_rule__ENA,
    output respond_rule__RDY,
    output ind$echo__ENA,
    output [31:0]ind$echo$v,
    input ind$echo__RDY);
wire fifo$CLK, fifo$nRST;
wire fifo$deq__ENA;
wire fifo$deq__RDY;
wire fifo$enq__ENA;
wire [31:0]fifo$enq_v;
wire fifo$enq__RDY;
wire [31:0]fifo$first;
wire fifo$first__RDY;
    l_class_OC_Fifo1 fifo (
        fifo$CLK,
        fifo$nRST,
        fifo$deq__ENA,
        fifo$deq__RDY,
        fifo$enq__ENA,
        fifo$enq_v,
        fifo$enq__RDY,
        fifo$first,
        fifo$first__RDY);
   reg[31:0] pipetemp;
    assign echoReq__RDY =         (fifo$enq__RDY);
    assign respond_rule__RDY =         (fifo$first__RDY) & (fifo$deq__RDY) & (ind$echo__RDY);
//        if (echoReq__ENA) begin
        fifo$enq__ENA = 1;
            fifo$enq_v = echoReq_v;
//        end; // End of echoReq

//        if (respond_rule__ENA) begin
        fifo$deq__ENA = 1;
        ind$echo__ENA = 1;
            ind$echo_v = (fifo$first);
//        end; // End of respond_rule

    always @( posedge CLK) begin
      if (!nRST) begin
        pipetemp <= 0;
      end
      else begin
      end // nRST
    end // always @ (posedge CLK)
endmodule 

//METAGUARD; echoReq__RDY;         (fifo$enq__RDY);
//METAGUARD; respond_rule__RDY;         (fifo$first__RDY) & (fifo$deq__RDY) & (ind$echo__RDY);
//METAINTERNAL; fifo; l_class_OC_Fifo1;
//METAEXTERNAL; ind; l_class_OC_EchoIndication;
//METAINVOKE; echoReq; :fifo$enq;
//METAINVOKE; respond_rule; :fifo$deq:ind$echo:fifo$first;
