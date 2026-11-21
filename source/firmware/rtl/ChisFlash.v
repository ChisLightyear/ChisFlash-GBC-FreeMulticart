// 主模块：ChisFlash程序卡控制器
// 实现Game Boy程序卡的多种MBC（Memory Bank Controller）功能和多卡选择
module ChisFlash(
	input  [15:0] GB_A,        // Game Boy地址总线
	inout  [7:0]  GB_D,        // Game Boy数据总线
	input         GB_CS,       // Game Boy芯片选择
	input         GB_CLK,      // Game Boy时钟
	input         nGB_WR,      // Game Boy写使能（低电平有效）
	input         GB_RD,       // Game Boy读使能

	inout        GB_RST,      // Game Boy复位信号（低电平有效）
	output [24:14] ROM_A,      // ROM地址总线（输出到NOR Flash）
	output [18:13] RAM_A,      // RAM地址总线
	output        nROM_CS,     // ROM芯片选择（低电平有效）
	output        nRAM_CS,     // RAM芯片选择（低电平有效）
	// RTC相关信号
	input         sck,         // RTC串行时钟
	input         mosi,        // RTC主出从入数据
	input         cs,          // RTC片选
	output        miso,        // RTC主入从出数据
	input         rtc_clk,     // RTC时钟
	
	output        motor        // 振动电机控制信号（用于MBC5r）
);	
	//assign motor = 0;          // 注释掉的振动电机禁用指令

	localparam  	TYPE_MBC0 = 4'd0;
	localparam  	TYPE_MBC1 = 4'd1;
	localparam  	TYPE_MBC2 = 4'd2;
	localparam  	TYPE_MBC3 = 4'd3;
	localparam  	TYPE_MBC5_MENU = 4'd4;
	localparam  	TYPE_MBC5 = 4'd5;
	localparam  	TYPE_MBC5_RUMBLE = 4'd6;
	// 控制寄存器定义
	reg         game_sel_en = 1'b0;  // 合卡模式使能：1表示已从菜单进入程序
	reg         gamestart_flag = 1'b0;  // 程序启动标志，防止重复复位
	reg  [15:0] addr_latch; 
	always @(negedge nGB_WR) begin
		addr_latch <= GB_A;
	end

//******************************************MBC切换逻辑***********************************************
	reg  [3:0]  get_mbc_type = TYPE_MBC5_MENU;  // MBC类型寄存器（默认MBC5）
	wire [3:0]  mbc_idx = (game_sel_en) ? get_mbc_type : TYPE_MBC5_MENU;  // MBC类型索引（进入程序时使用get_mbc_type，菜单时使用4'd7）

	// MBC模块信号定义
	wire [18:0] mbc0_out;   // MBC1模块输出
	wire [18:0] mbc1_out;   // MBC1模块输出
	wire [18:0] mbc5menu_out;   // MBC5模块输出
	wire [18:0] mbc5_out;   // MBC5模块输出
	wire [18:0] mbc5r_out;  // MBC5r模块输出（带振动）
	reg menu_mode = 1'b0;  // RAM bank寄存器，bit6用于进入菜单模式
	wire [18:0] mbc_out;       // MBC模块输出的总信号
	wire [24:14] mbc_rom_bank;    // MBC模块输出的ROM bank地址
	wire [18:13] mbc_ram_a;    // MBC模块输出的RAM bank地址

	// MBC模块选择逻辑
	// 根据mbc_idx选择不同的MBC模块输出
	assign mbc_out[18:0] = 
		(mbc_idx == TYPE_MBC1) ? mbc1_out :  // MBC1控制器
		//mbc_idx == TYPE_MBC2 ? mbc2_out : // MBC2控制器
		//mbc_idx == TYPE_MBC3 ? mbc3_out : // MBC3控制器
		(mbc_idx == TYPE_MBC5_MENU) ? mbc5menu_out :  // MBC5menu（菜单专用）
		(mbc_idx == TYPE_MBC5) ? mbc5_out :   // MBC5控制器
		(mbc_idx == TYPE_MBC5_RUMBLE) ? mbc5r_out :  // MBC5r控制器（带振动）
		mbc0_out;                             // 默认值
	
	// 从MBC输出信号中提取各部分信息
	assign mbc_rom_bank[24:14] = mbc_out[10:0];   // 提取ROM bank地址
	assign mbc_ram_a[18:13] = mbc_out[16:11];   // 提取RAM bank地址
	assign nROM_CS = mbc_out[17];               // 提取ROM片选信号
	//wire dout_en =    mbc_out[19];                 // 提取数据输出使能信号
	//assign GB_D[7:0] = dout_en ? mbc_out[27:20] : 8'hzz;  // 三态数据输出
	
	// MBC模块实例化
	MBC0 mbc0_unit(GB_A[15:0],GB_D[7:0],GB_CS,GB_CLK,nGB_WR,GB_RD,mbc0_out);  // MBC1控制器实例

	MBC1 mbc1_unit(GB_A[15:0],GB_D[7:0],GB_CS,GB_CLK,nGB_WR,GB_RD,mbc1_out);  // MBC1控制器实例
	
	//wire [18:0] mbc2_out;   // MBC2模块输出
	//MBC2 mbc2_unit(GB_A[15:0],GB_D[7:0],GB_CS,GB_CLK,nGB_WR,GB_RD,mbc2_out);  // MBC2控制器实例
	
	//wire [18:0] mbc3_out;  // MBC3模块输出（注释掉）
	//MBC3 mbc3_unit(osc_sig,GB_A[15:0],GB_D[7:0],GB_CS,GB_CLK,nGB_WR,GB_RD,mbc3_out,
	//sck,mosi,cs,miso,rtc_clk);  // MBC3控制器实例（带RTC功能）
	
	MBC5menu mbc5menu_unit(GB_A[15:0],GB_D[7:0],GB_CS,GB_CLK,nGB_WR,GB_RD,mbc5menu_out,menu_mode);  // MBC5控制器实例
	
	MBC5 mbc5_unit(GB_A[15:0],GB_D[7:0],GB_CS,GB_CLK,nGB_WR,GB_RD,mbc5_out);  // MBC5控制器实例

	MBC5r mbc5r_unit(GB_A[15:0],GB_D[7:0],GB_CS,GB_CLK,nGB_WR,GB_RD,mbc5r_out,motor);  // MBC5r控制器实例（带振动功能）

//******************************************合卡逻辑***********************************************
	// ROM地址映射
	reg [10:0] rom_start_bank = 11'd0; // 基准bank寄存器（11位，支持2048个bank）
	reg [5:0] ram_start_bank = 6'd0; 
	reg [2:0]  rom_bank_lock = 3'b0;  
	assign ROM_A[24:14] = (game_sel_en)  ? // 外层条件：是否启用合卡模式
		rom_start_bank[10:0] + mbc_rom_bank[24:14]: // 合卡模式：输出偏移后的bank
		mbc_rom_bank[24:14];     // 单卡模式：输出原始bank

	// 菜单直接访问物理 SRAM；游戏仅能访问分配给它的连续区间。
	reg [4:0] ram_bank_count = 5'd0;
	reg ram_load_active = 1'b0;
	reg ram_map_invalid = 1'b0;
	wire [3:0] logical_ram_bank = mbc_ram_a[16:13];
	wire [6:0] next_physical_bank = {1'b0, ram_start_bank} + {2'b00, ram_bank_count};
	// 与原固件一致，游戏选择超过区间的 bank 时停在本游戏最后一个 bank。
	wire [3:0] game_ram_offset = ({1'b0, logical_ram_bank} < ram_bank_count) ?
		logical_ram_bank : ram_bank_count[3:0] - 4'd1;
	assign RAM_A = game_sel_en ? ram_start_bank + {2'b00, game_ram_offset} : mbc_ram_a;
	// 菜单命令占用 A000/B000；发送命令时不能同时写到 SRAM。
	assign nRAM_CS = mbc_out[18] || menu_mode ||
		(game_sel_en && (rom_bank_lock != 3'd4 || ram_map_invalid || ram_bank_count == 0));

	// 合卡选择逻辑
	// 仅在菜单模式下（menu_mode == 1）响应命令
	always @(posedge nGB_WR)
	begin
		if(menu_mode == 1'b1 && gamestart_flag == 1'b0) // 仅在菜单模式下响应
		begin
			case(addr_latch[15:12])
				4'hA: begin
					game_sel_en <= GB_D[0];  // 0:关闭合卡模式, 1:开启合卡模式
				end
				4'hB: begin
					if(rom_bank_lock == 3'b0) begin
						rom_start_bank[7:0] <= GB_D[7:0];
						rom_bank_lock <= 3'b001;
					end
					else if(rom_bank_lock == 3'b001) begin
						rom_start_bank[10:8] <= GB_D[2:0];
						rom_bank_lock <= 3'b010;
					end
					else if(rom_bank_lock == 3'b010) begin
						case(GB_D[7:0])
							8'h00			   : get_mbc_type[3:0] <= TYPE_MBC0; 
							8'h01, 8'h02, 8'h03: get_mbc_type[3:0] <= TYPE_MBC1; 
							8'h1C, 8'h1D, 8'h1E: get_mbc_type[3:0] <= TYPE_MBC5_RUMBLE;
							default: 		     get_mbc_type[3:0] <= TYPE_MBC5;
						endcase
						rom_bank_lock <= 3'b011;	
					end
					else if(rom_bank_lock == 3'b011) begin
						if (GB_D == 8'hFF) begin
							if (!ram_load_active) begin
								// 沿用无显式映射时的共享区 60～63。
								ram_start_bank <= 6'd60;
								ram_bank_count <= 5'd4;
							end
							ram_load_active <= 1'b0;
							rom_bank_lock <= 3'd4;
						end else if (!ram_load_active && GB_D == 8'hFE) begin
							ram_load_active <= 1'b1;
						end else if (ram_load_active) begin
							// 以七位计算下一物理地址，避免 63 后回绕到 0。
							if (GB_D[7:6] != 0 || ram_bank_count >= 5'd16 ||
								(ram_bank_count != 0 && ({1'b0, GB_D[5:0]} != next_physical_bank ||
								(ram_start_bank < 6'd60 && GB_D[5:0] >= 6'd60)))) begin
								ram_map_invalid <= 1'b1;
							end else begin
								if (ram_bank_count == 0)
									ram_start_bank <= GB_D[5:0];
								ram_bank_count <= ram_bank_count + 1'b1;
							end
						end else begin
							ram_map_invalid <= 1'b1;
						end
					end
				end
			endcase
		end
	end

	// RAM bank寄存器控制逻辑
	// 仅在非合卡模式下响应GB_D[6]设置，合卡模式下强制置0
	// 实际是rambank[6]
	always @(posedge nGB_WR)
	begin
		case(addr_latch[15:12])
		// MBC RAM bank设置地址
		4'h4,
		4'h5:	
		begin
			if (!game_sel_en)    begin      // 非合卡模式
				menu_mode <= GB_D[6] ; // 响应菜单模式请求
			end else  begin                     // 合卡模式
				menu_mode <= 1'b0 ;     // 已退出菜单，解锁ram
			end
		end
		endcase
	end



//******************************************复位逻辑***********************************************


	// 振荡器实例化
	wire osc_sig;         // 振荡器输出信号（用于复位计时）
	OSC	OSC_inst (
		.oscena ( 1'b1 ),  // 振荡器使能（始终开启）
		.osc ( osc_sig )   // 振荡器输出
		);

	// 复位信号控制
	reg RST_reg = 1'b1;        // 复位寄存器
	assign GB_RST = RST_reg;// ? 1'bz : 1'b0;  // 复位信号（低电平有效）
	reg [15:0] rst_counter;    // 复位计数器
	reg rst_active;            // 复位激活标志

	// 程序启动复位逻辑
	// 在进入程序最后一刻产生重启信号（同时会关闭menu_mode）

	wire rst_clk = (!nGB_WR) && (addr_latch[15:12] == 4'h4) && (menu_mode == 1'b1)  && (gamestart_flag == 1'b0);
	
	// 使用振荡器信号作为复位计数器时钟
	always @(posedge osc_sig) begin
		if (rst_clk && game_sel_en == 1'b1) begin
			rst_active <= 1'b1;      // 激活复位
			rst_counter <= 0;        // 清零计数器
			RST_reg <= 1'b0;        // 拉低RST
		end else if (rst_active && game_sel_en == 1'b1) begin
			if (rst_counter >= 16'd182032) begin  // 4.194304MHz时钟下计数42016次 ≈ 10ms
				RST_reg <= 1'b1;    // 恢复高电平
				rst_active <= 1'b0;  // 结束复位
				gamestart_flag <= 1'b1;
			end else begin
				rst_counter <= rst_counter + 1;  // 递增计数器
			end
		end
	end

endmodule
