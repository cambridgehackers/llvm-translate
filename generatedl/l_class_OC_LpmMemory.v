
`include "l_class_OC_LpmMemory.vh"

module l_class_OC_LpmMemory (
    input CLK,
    input nRST,
    input req__ENA,
    input [703:0]v,
    output req__RDY,
    input resAccept__ENA,
    output resAccept__RDY,
    output [703:0]resValue,
    output resValue__RDY,
    input [`l_class_OC_LpmMemory_RULE_COUNT:0]rule_enable,
    output [`l_class_OC_LpmMemory_RULE_COUNT:0]rule_ready);
    wire memdelay__RDY_internal;
    wire memdelay__ENA_internal = rule_enable[0] && memdelay__RDY_internal;
    wire req__RDY_internal;
    wire req__ENA_internal = req__ENA && req__RDY_internal;
    wire resAccept__RDY_internal;
    wire resAccept__ENA_internal = resAccept__ENA && resAccept__RDY_internal;
    reg[31:0] delayCount;
    reg[703:0] saved;
    assign memdelay__RDY_internal = delayCount > 1;
    assign req__RDY = req__RDY_internal;
    assign req__RDY_internal = delayCount == 0;
    assign resAccept__RDY = resAccept__RDY_internal;
    assign resAccept__RDY_internal = delayCount == 1;
    assign resValue = saved;
    assign resValue__RDY_internal = delayCount == 1;
    assign rule_ready[0] = memdelay__RDY_internal;

    always @( posedge CLK) begin
      if (!nRST) begin
        delayCount <= 0;
        saved <= 0;
      end // nRST
      else begin
        if (memdelay__ENA_internal) begin
            delayCount <= delayCount - 1;
        end; // End of memdelay
        if (req__ENA_internal) begin
            delayCount <= 4;
            saved <= v;
        end; // End of req
        if (resAccept__ENA_internal) begin
            delayCount <= 0;
        end; // End of resAccept
      end
    end // always @ (posedge CLK)
endmodule 

