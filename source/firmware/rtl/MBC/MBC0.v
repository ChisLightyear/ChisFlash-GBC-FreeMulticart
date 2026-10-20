module MBC0(
	input [15:0] GB_A,
	input [7:0] GB_D,
	input GB_CSn,
	input GB_CLK,
	input GB_WRn,
	input GB_RDn,

	output [19:0] mbc_out

);

	assign mbc_out[10:0] = {10'b0,GB_A[14]}; // ROM_A[24:14]
	assign mbc_out[16:11] = {6'b0}; // RAM_A[18:13]
	assign mbc_out[17] = GB_A[15]; // ROM_CSn
	assign mbc_out[18] = 1'b1; // RAM_CSn
	assign mbc_out[19] = 1'b0;

endmodule