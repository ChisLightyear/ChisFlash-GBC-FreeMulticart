module MBC1(
	input [15:0] GB_A,
	input [7:0] GB_D,
	input GB_CSn,
	input GB_CLK,
	input GB_WRn,
	input GB_RDn,

	output [18:0] mbc_out
);
	wire [24:14] ROM_A;
	wire [18:13] RAM_A;
	wire ROM_CSn;
	wire RAM_CSn;
	
	assign mbc_out[10:0] = ROM_A[24:14];
	assign mbc_out[16:11] = RAM_A[18:13];
	assign mbc_out[17] = ROM_CSn;
	assign mbc_out[18] = RAM_CSn;
	
	reg [4:0] rom_bank;
	reg [1:0] ram_bank;
	reg bank_mode;
	reg ram_en = 1'b0;

	//wire rom_addr_en;
	wire ram_addr_en;
	wire rom_addr_lo;

	wire [15:0] gb_addr; 
	assign gb_addr[15:0] = {GB_A[15:12],12'b0};

	//assign rom_addr_en = (gb_addr >= 16'h0000)&(gb_addr <= 16'h7FFF);
	assign ram_addr_en = (gb_addr >= 16'hA000)&(gb_addr <= 16'hBFFF);
	assign rom_addr_lo = (gb_addr >= 16'h0000)&(gb_addr <= 16'h3FFF);

	assign ROM_CSn = GB_A[15];
	//assign ROM_CSn = rom_addr_en ? 0 : 1;
	assign RAM_CSn = (ram_addr_en & ram_en) ? 0 : 1; 
	
	wire [6:5] rom_bank_h;
	//assign rom_bank_h = bank_mode ? 0 : ram_bank[1:0];
	assign rom_bank_h = ram_bank[1:0];
	assign ROM_A[24:21]= 4'b0;
	assign ROM_A[20:14]= rom_addr_lo ? (bank_mode ? {ram_bank[1:0],5'b0}: 7'b0) : 
	
	(rom_bank[4:0] == 0) ? {rom_bank_h[6:5],4'b0001} :
	{rom_bank_h[6:5],rom_bank[4:0]}; 
	
	//assign RAM_A[18:13] = {~ram_bank[5],ram_bank[4:0]};
	assign RAM_A[18:13] = bank_mode ? {4'b1000,ram_bank[1:0]}: 6'b100000;
	
	//MBC1 ROM_BANK切换
	always @(posedge GB_WRn)
	begin
		case(gb_addr[15:0])
		16'h0000,
		16'h1000:ram_en <= (GB_D[3:0] == 4'hA) ? 1 : 0; 
		16'h2000,
		16'h3000:rom_bank[4:0] <= GB_D[4:0];
		16'h4000,
		16'h5000:ram_bank[1:0] <= GB_D[1:0];
		16'h6000,
		16'h7000:bank_mode <= GB_D[0];
		endcase
	end

endmodule