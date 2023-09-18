// assuming clk is 50mhz - generates 800x600 @ 72hz VGA signal

module vga(clk, r, g, b, hsync, vsync, led);
	input clk;
	output r,g,b,hsync,vsync,led;
	reg r,g,b,hsync,vsync,led;
	
	reg[10:0] hcnt;
	reg[9:0] vcnt;
	//reg[21:0] ledcnt;
	
	reg[5:0] mcnt;
	reg[5:0] htmp;
	reg[5:0] vtmp;
	
	localparam H_VIS = 800;
	localparam H_FRONT_PORCH = 56;
	localparam H_SYNC_PULSE = 120;
	localparam H_BACK_PORCH = 64;
	localparam H_PERIOD = H_VIS + H_FRONT_PORCH + H_SYNC_PULSE + H_BACK_PORCH;
	localparam H_SYNC_PULSE_BEGIN = H_VIS + H_FRONT_PORCH;
	localparam H_SYNC_PULSE_END = H_SYNC_PULSE_BEGIN + H_SYNC_PULSE;
	
	localparam V_VIS = 600;
	localparam V_FRONT_PORCH = 37;
	localparam V_SYNC_PULSE = 6;
	localparam V_BACK_PORCH = 23;
	localparam V_PERIOD = V_VIS + V_FRONT_PORCH + V_SYNC_PULSE + V_BACK_PORCH;
	localparam V_SYNC_PULSE_BEGIN = V_VIS + V_FRONT_PORCH;
	localparam V_SYNC_PULSE_END = V_SYNC_PULSE_BEGIN + V_SYNC_PULSE;
	
	always @(posedge clk) begin
		reg chk;

		//ledcnt <= ledcnt + 1'b1;
		//led <= ledcnt[21];
		
		if (hcnt == (H_PERIOD-1)) begin
			hcnt <= 0;
			vcnt <= vcnt + 1'b1;
		end else hcnt <= hcnt + 1'b1;
		
		if (vcnt == (V_PERIOD-1)) begin
			vcnt <= 0;
			mcnt <= mcnt + 1'b1;
		end
		
		hsync <= ((hcnt >= H_SYNC_PULSE_BEGIN) && (hcnt < H_SYNC_PULSE_END));
		vsync <= ((vcnt >= V_SYNC_PULSE_BEGIN) && (vcnt < V_SYNC_PULSE_END));
		
		if (hcnt < H_VIS && vcnt < V_VIS) begin
			htmp <= hcnt[5:0] + mcnt[5:0];
			vtmp <= vcnt[5:0] + mcnt[5:0];
			chk <= htmp[5] ^ vtmp[5];
			r <= chk;
			g <= ~chk;
			b <= 0;
		end else begin
			r <= 0;
			g <= 0;
			b <= 0;
		end;
	end
endmodule
