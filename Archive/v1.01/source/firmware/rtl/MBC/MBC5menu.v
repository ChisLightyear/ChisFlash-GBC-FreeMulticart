module MBC5menu(
	input [15:0] GB_A,
	input [7:0] GB_D,
	input GB_CSn,
	input GB_CLK,
	input GB_WRn,
	input GB_RDn,

	output [18:0] mbc_out,
	input menu_mode
);
	wire [24:14] ROM_A;
	wire [18:13] RAM_A;
	wire ROM_CSn;
	wire RAM_CSn;
	
	assign mbc_out[10:0] = ROM_A[24:14];
	assign mbc_out[16:11] = RAM_A[18:13];
	assign mbc_out[17] = ROM_CSn;
	assign mbc_out[18] = RAM_CSn;

	reg [10:0] rom_bank;
	reg [5:0] ram_bank;
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


	assign ROM_A[24:14]= (GB_A[15:12] < 4'h4) ? 11'b0 : rom_bank[10:0]; 
	assign RAM_A[18:13] = ram_bank[5:0];

	//assign motor = ram_bank[3];
	//assign motor = 1'bz;
	//MBC5 ROM_BANK
	always @(posedge GB_WRn)
	begin
		case(gb_addr[15:0])
		16'h0000,
		16'h1000:ram_en <= (GB_D[3:0] == 4'hA) ? 1 : 0; 
		16'h2000:rom_bank[7:0] <= GB_D[7:0];
		16'h3000:rom_bank[10:8] <= GB_D[2:0];
		16'h4000,
		16'h5000:ram_bank[5:0] <= GB_D[5:0];
		endcase
	end

endmodule