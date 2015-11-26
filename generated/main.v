module top(input CLK, input nRST);
  always @( posedge CLK) begin
    if (!nRST) then begin
    end
    else begin
//processing _ZN14EchoIndication4echoEi
        printf((("Heard an echo: %d\n")), _ZN14EchoIndication4echoEi_v);
        stop_main_program <= 1;

//processing printf

    end; // nRST
  end; // always @ (posedge CLK)
endmodule 

