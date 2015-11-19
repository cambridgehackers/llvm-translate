module l_class_OC_Echo (
    input CLK,
    input nRST,
    output respond__RDY,
    input respond__ENA__ENA);

  ;
  ;
   reg[31:0] pipetemp;
  always @( posedge CLK) begin
    if (!nRST) begin
    end
    else begin
        // Method: respond__RDY
    respond__RDY_tmp__1 = ((*(fifo)).deq__RDY);
    respond__RDY_tmp__2 = ((*(fifo)).first__RDY);
        respond__RDY = (respond__RDY_tmp__1 & respond__RDY_tmp__2);

        // Method: respond__ENA
        if (respond__ENA__ENA) begin
        ((*(fifo)).deq);
        respond__ENA_call = ((*(fifo)).first);
        _ZN14EchoIndication4echoEi;
        end; // End of respond__ENA

    end; // nRST
  end; // always @ (posedge CLK)
endmodule 

