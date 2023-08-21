window.onload = () => {
INSERT_CODEGEN_HERE

	const ASSERTION_ERROR = (msg) => { throw new Error("ASSERTION ERROR: " + msg); }

	const DEBUG = false;
	const START_GOLD = 10;

	const BOARD_N_COLUMNS = 9;
	const BOARD_N_ROWS = 11;
	const BOARD =
	 "gg^...^gg"
	+"gg^...^gg"
	+"gg^...^gg"
	+"$~~~$^ggg"
	+"~g$g~gggg"
	+"ggg~$~ggg"
	+"gggg~g$g~"
	+"ggg^$~~~$"
	+"gg^...^gg"
	+"gg^...^gg"
	+"gg^...^gg";

	window.CTRLS = [
		["w","a","s","d",                                     "f", "g"], // player 0
		["ArrowUp", "ArrowLeft", "ArrowDown", "ArrowRight",   ",", "."], // player 1
	];

	const W=576,H=324;
	const GRID_SPACING = 28;

	const CANVAS = document.getElementById("c");

	let is_controling = true;

	const N_PLAYERS = 2;
	const N_CONTROLS = 6;
	let player_input = [[],[]];
	let player_press = [[],[]];
	let mk_key_handler = (is_down) => (ev) => {
		if (is_controling) {
			for (let player = 0; player < N_PLAYERS; player++) {
				for (let key = 0; key < N_CONTROLS; key++) {
					if (ev.key === window.CTRLS[player][key]) {
						player_input[player][key] = is_down;
						ev.preventDefault();
					}
				}
			}
		}
	};

	window.addEventListener("keydown", mk_key_handler(1));
	window.addEventListener("keyup", mk_key_handler(0));

	const BOARD_AT = (row,column) => { if (0<=row && row<BOARD_N_ROWS && 0<=column && column<BOARD_N_COLUMNS) return BOARD[row*BOARD_N_COLUMNS+column]; };
	const HGS = (GRID_SPACING/2)|0;
	const bitmap = new Uint8Array(W*H);

	const clear_bitmap = () => bitmap.fill(0);
	const clon = (v) => JSON.parse(JSON.stringify(v));
	const clamp = (v,v0,v1) => Math.max(v0,Math.min(v1,v));

	let current_palette_index;
	let translate_x,translate_y;
	const translate_reset = () => { translate_x=translate_y=0; };
	translate_reset();
	const put_pixel = (x,y) => {
		x += translate_x;
		y += translate_y;
		if (0 <= x && x < W && 0 <= y && y < H) {
			bitmap[y*W+x] = current_palette_index;
		}
	}

	const draw_hline = (y,x0,x1) => { for (let x = Math.min(x0,x1); x <= Math.max(x0,x1); x++) put_pixel(x,y); }
	const draw_vline = (x,y0,y1) => { for (let y = Math.min(y0,y1); y <= Math.max(y0,y1); y++) put_pixel(x,y); }
	const draw_dline = (x,y,dx,dy,n) => { for (let i = 0; i < n; i++) { put_pixel(x,y); x+=dx; y+=dy; } };
	const draw_box = (x0,y0,x1,y1) => {
		draw_hline(y0,x0,x1);
		draw_hline(y1,x0,x1);
		draw_vline(x0,y0,y1);
		draw_vline(x1,y0,y1);
	};

	const lpad = (s,n) => {
		while (s.length < n) s = " "+s;
		return s;
	};

	const lowbase_decode = (s) => {
		const n = s.length;
		const xs = new Uint8Array(n);
		for (let i = 0; i < n; i++) {
			xs[i] = s.charCodeAt(i) - "0".charCodeAt(0);
		}
		return xs;
	};

	const base64_decode = (a) => {
		const b = atob(a);
		const n = b.length;
		const d = new Uint8Array(n);
		for (let i = 0; i < n; i++) {
			d[i] = b.charCodeAt(i);
		}
		return d;
	};


	const pal_decode = (p64) => {
		const b = base64_decode(p64);
		let pal = [];
		let pi = 0;
		for (let i0 = 0; i0 < b.length; i0+=3) {
			let v = 0;
			for (i1 = 0; i1 < 3; i1++) {
				v += b[i0+i1] << (8*i1);
			}
			pal.push(pi++);
			pal.push(v);
		}
		return pal;
	};

	let pal = pal_decode(PAL);

	for (const f of [FONT0,PIECES]) {
		f[0] = lowbase_decode(f[0]);
	}

	const SAMURAI = 2;
	const ROOK = 0;
	const SHOGUN = 1;
	const HORSE = 3;
	const NINJA = 4;

	const GAMSTA = (() => {
		let current_player_index;
		let players_gold;
		let pieces = [];

		const _start_new_game = () => {
			current_player_index = 0;
			pieces = [];
			const push_pieces = (player_index) => {
				for (let pd of [
					[SHOGUN,1,4],
					[NINJA,2,3],
					[NINJA,2,5],
					[ROOK,0,0],
					[ROOK,0,8],
					[HORSE,2,2],
					[HORSE,2,7],
					[HORSE,0,4],
					[SAMURAI,2,0],
					[SAMURAI,2,1],
					[SAMURAI,3,5],
					[SAMURAI,3,6],
					[SAMURAI,3,7],
				]) {
					pd = pd.slice(0);
					if (player_index === 0) {
						pd[1] = BOARD_N_ROWS-1-pd[1];
						pd[2] = BOARD_N_COLUMNS-1-pd[2];
					}
					pieces.push([player_index, pd[0], pd[1], pd[2]]);
				}
			};
			push_pieces(0);
			push_pieces(1);
			players_gold = [START_GOLD,START_GOLD]
		};

		const _get_piece_at = (row, col) => {
			for (let piece of pieces) {
				if (piece[2] === row && piece[3] === col) {
					return piece;
				}
			}
		};

		_start_new_game();

		return {
			_start_new_game,
			_get_player_index: () => current_player_index,
			_get_pieces: () => pieces,
			_get_piece_at,
			_get_player_gold: (player_index) => players_gold[player_index],
		};
	})();

	const present = (() => {
		CANVAS.width = W;
		CANVAS.height = H;
		let ctx = CANVAS.getContext('2d');
		const img = ctx.createImageData(W,H);
		const imgdata = img.data;
		const palette = new Uint32Array(256);

		return (copperlist) => {
			let next_copper_scanline=0, rp=0, wp=0, copcursor=0;
			const copget = () => copperlist[copcursor++];
			for (let y = 0; y < H; y++) {
				if (y === next_copper_scanline) {
					for (;;) {
						const cmd = copget();
						if (cmd === undefined) {
							break
						} else if (0 <= cmd && cmd < 256) {
							palette[cmd] = copget();
						} else if (cmd === -1) {
							next_copper_scanline = copget();
							break;
						} else {
							ASSERTION_ERROR("unhandled cmd: " + cmd);
						}
					}
				}
				for (let x = 0; x < W; x++) {
					let pixel = palette[bitmap[rp++]];
					for (let c = 0; c < 3; c++, pixel >>= 8) imgdata[wp++] = pixel & 0xff;
					imgdata[wp++] = 0xff;
				}
			}
			ctx.putImageData(img, 0, 0);
		};
	})();

	let rqfr = window.requestAnimationFrame;
	let last_player_input = clon(player_input);

	const CURSOR = 1;
	let now;
	let st = {
		_state: CURSOR,
		_cursor_row: 0,   _cursor_column: 0,
		_info_lines: [],
		_action_ts: 0,
	};
	st._act = () => st._action_ts = now;
	st._update_cursor = () => {
		const bb = BOARD_AT(st._cursor_row, st._cursor_column);
		let xs = [];
		const push = (c,s) => xs.push([c,s]);

		switch (bb) {
		case "g":
			push(1, "Plains");
			break;
		case "^":
			push(2, "Mountains");
			push(2, "The high ground");
			break;
		case "$":
			push(3, "Gold Point");
			push(3, "Earn by placing unit here");
			push(3, " more thru ROOK-support");
			break;
		case ".":
			push(4, "House of the Shogun");
			break;
		case "~" :
			push(5, "River");
			push(5, "Units cannot rest here");
			push(5, " but may move across");
			break;
		}

		const piece = GAMSTA._get_piece_at(st._cursor_row, st._cursor_column);
		if (piece) {
			xs.push([]);
			const c = [6,7][piece[0]];
			switch (piece[1]) {
			case SAMURAI: xs.push([c, "Samurai"]); break;
			case ROOK: xs.push([c, "Castle"]); break;
			case SHOGUN: xs.push([c, "Shogun"]); break;
			case HORSE: xs.push([c, "Horse"]); break;
			case NINJA: xs.push([c, "Ninja"]); break;
			}
		}

		st._info_lines = xs;
	};
	st._update_cursor();

	let render; render = () => {
		for (let i = 0; i < N_PLAYERS; i++) {
			for (let j = 0; j < N_CONTROLS; j++) {
				player_press[i][j] = player_input[i][j] && !last_player_input[i][j];
			}
		}
		last_player_input = clon(player_input);

		now = Date.now();
		const gold_seq = (Math.floor(now * 0.035) % 90) - 45;
		clear_bitmap();

		let current_sprite_set, current_sprite_set_map;
		let current_font = FONT0;

		const put_cc = (x0,y0,cc) => {
			let font = current_font;
			const data = font[0];
			const w = font[1];
			const h = font[2];
			const adv = w*h;
			font = font.slice(3);
			let data_cursor = 0;
			while (font.length > 0) {
				const c0 = font[0];
				const c1 = font[1];
				if (c0 <= cc && cc <= c1) {
					data_cursor += adv*(cc-c0);
					for (let y = 0; y < h; y++) {
						for (let x = 0; x < w; x++) {
							if (data[data_cursor]) {
								put_pixel(x0+x,y0+y);
							}
							data_cursor++;
						}
					}
					break;
				} else {
					data_cursor += adv * (c1-c0+1);
				}
				font = font.slice(2);
			}
		};

		const put_string = (x,y,str) => {
			let font = current_font;
			for (let i = 0; i < str.length; i++) put_cc(x+i*font[1],y,str.charCodeAt(i));
		};

		const put_sprite = (x,y,index) => {
			const data = current_sprite_set[0];
			let ss = current_sprite_set.slice(1);
			let dp = 0;
			while (dp < data.length) {
				const w = ss[1];
				const h = ss[2];
				if (ss[0] === index) {
					const ax = ss[3];
					const ay = ss[4];
					const x0 = x-ax;
					const y0 = y-ay;
					const x1 = x0 + w;
					const y1 = y0 + h;
					for (let py = y0; py < y1; py++) {
						for (let px = x0; px < x1; px++) {
							const pix = data[dp++];
							if (pix > 0) {
								current_palette_index = current_sprite_set_map[pix-1];
								put_pixel(px,py);
							}
						}
					}
					break;
				}
				ss = ss.slice(5);
				dp += w*h;
			}
		};


		const board_get_coord = (row, column) => {
			return [
				W/2 + (column - ((BOARD_N_COLUMNS/2)|0)) * GRID_SPACING + 50,
				HGS + row    * GRID_SPACING + 10,
			];
		};

		for (let row = 0; row < BOARD_N_ROWS; row++) {
			for (let column = 0; column < BOARD_N_COLUMNS; column++) {
				const
					is_left_border = column === 0,
					is_top_border = row === 0,
					is_right_border = column === BOARD_N_COLUMNS-1,
					is_bottom_border = row === BOARD_N_ROWS-1;
				const is_top_or_bottom_border = is_top_border || is_bottom_border;
				[ translate_x,translate_y ] = board_get_coord(row,column);
				const draw_kn_line = (i,k,n) => {
					const [dx,dy] = [[-1,-1],[1,-1],[1,1],[-1,1]][i];
					draw_dline(dx*k,dy*k,dx,dy,n);
				};
				const draw_kn_star = (k,n) => { for (let i = 0; i < 4; i++) draw_kn_line(i,k,n); }

				const point = BOARD_AT(row, column);
				switch (point) {
				case "g": { // GREEN
					current_palette_index = 1;
					if (!is_left_border) draw_hline(0,0,-HGS+1);
					if (!is_right_border) draw_hline(0,0,HGS-1);
					if (!is_top_border) draw_vline(0,0,-HGS+1);
					if (!is_bottom_border) draw_vline(0,0,HGS-1);
				} break;
				case "^": { // MOUNTAINS
					const mountains = [0,0,1,2,3,2,1,2,1,0,0,1,2,3,2,1,0,0,1,2,3,2,1,2,1,0,0];
					current_palette_index = 2;
					// TODO
					let ymid = 0;
					for (let xi = 0; xi < GRID_SPACING; xi++) {
						let x = xi-HGS+1;
						let y = mountains[xi];
						// XXX if left/right border mountains, but... maybe don't do that?
						// it looks weird and makes mountains harder to spot?
						//if (is_left_border && x < 0) continue;
						//if (is_right_border && x > 0) continue;
						put_pixel(x, -y);
						if (x === 0) ymid = y;
					}
					if (!is_top_border) draw_vline(0,-ymid-2,-HGS+1);
					if (!is_bottom_border) draw_vline(0,1,HGS-1);
				} break;
				case "$": { // GOLD
					current_palette_index = 3;
					// TODO
					if (!is_left_border) draw_hline(0,0,-HGS+1);
					if (!is_right_border) draw_hline(0,0,HGS-1);
					if (!is_top_border) draw_vline(0,0,-HGS+1);
					if (!is_bottom_border) draw_vline(0,0,HGS-1);
					for (let i = 0; i < 4; i++) {
						const k = 6-Math.min(4,Math.max(2, Math.abs(column*2 - gold_seq + i*2)));
						const n = 3;
						draw_kn_line(i,k,n);
					}
				} break;
				case ".": { // HOUSE
					// TODO
					const house_column = (BOARD_N_COLUMNS-1)/2;
					const house_row0 = 1;
					const house_row1 = BOARD_N_ROWS-2;
					const house_row = Math.abs(house_row0-row) < Math.abs(house_row1-row) ? house_row0 : house_row1;
					const dhouse_row    = row    - house_row;
					const dhouse_column = column - house_column;
					current_palette_index = 4;
					if (dhouse_column >= 0) draw_hline(0,0,-HGS+1);
					if (dhouse_column <= 0) draw_hline(0,0,HGS-1);
					if (dhouse_row >= 0) draw_vline(0,0,-HGS+1);
					if (dhouse_row <= 0) draw_vline(0,0,HGS-1);
					if (dhouse_row === 0 && dhouse_column === 0) {
						const k = 2;
						draw_kn_star(k,HGS-k);
					} else {
						if (dhouse_column && dhouse_row) {
							const k = 2;
							const dx = Math.sign(-dhouse_column);
							const dy = Math.sign(-dhouse_row);
							draw_dline(k*dx,k*dy,dx,dy,HGS-k);
						}

						/*
						const map_point = (point) => {
							switch (other) {
							case "g": case "~": case "$": return 1;
							case "^": return 2;
							}
						};
						*/
						const map_point = (point) => point === "^" ? 2 : point ? 1 : 0;

						if (dhouse_column) {
							current_palette_index = map_point(BOARD_AT(row, column+dhouse_column));
							draw_hline(0, 2*dhouse_column, (HGS-1)*dhouse_column);
						}

						if (dhouse_row && !is_top_or_bottom_border) {
							current_palette_index = map_point(BOARD_AT(row+dhouse_row, column));
							draw_vline(0, 2*dhouse_row, (HGS-1)*dhouse_row);
						}
					}
				} break;
				case "~": { // RIVER
					current_palette_index = 1;
					{
						const k = 8;
						if (!is_left_border) draw_hline(0,-k,-HGS+1);
						if (!is_right_border) draw_hline(0,k,HGS-1);
						if (!is_top_border) draw_vline(0,-k,-HGS+1);
						if (!is_bottom_border) draw_vline(0,k,HGS-1);
					}

					current_palette_index = 5;
					for (let drow = -1; drow <= 1; drow++) {
						for (let dcol = -1; dcol <= 1; dcol++) {
							if (BOARD_AT(row+drow, column+dcol) === "~") {
								const fat = 1;
								for (let dy = -fat; dy <= fat; dy++) {
									for (dx = -fat; dx <= fat; dx++) {
										if (dcol === 0) {
											draw_vline(dx,dy,(HGS-1)*drow);
										} else if (drow === 0) {
											draw_hline(dy,dx,(HGS-1)*dcol);
										} else {
											draw_dline(dx,dy,dcol,drow,HGS);
										}
									}
								}
							}
						}
					}

				} break;
				default: ASSERTION_ERROR("unhandled board point: " + point);
				}
			}
		}
		translate_reset();

		current_sprite_set = PIECES;
		for (let player_index = 0; player_index < 2; player_index++) {
			current_sprite_set_map = [[6,7],[7,6]][player_index];
			for (const p of GAMSTA._get_pieces()) {
				if (p[0] !== player_index) continue;
				const [sx,sy] = board_get_coord(p[2],p[3]);
				put_sprite(sx, sy+4, p[1]);
			}

			let xc = W-8*13;
			let yc = board_get_coord([(BOARD_N_ROWS-1)/2,0][player_index], 0)[1];
			const ys = 12;
			const playcol = [6,7][player_index];
			current_palette_index = playcol;
			const our_turn = GAMSTA._get_player_index() === player_index;
			put_string(xc-(our_turn?8:0),yc,(our_turn?"\x04":"")+"Player " + ["LIGHT","DARK"][player_index] + (our_turn?"\x02":""));
			yc+=ys;
			current_palette_index = 3;
			put_string(xc,yc,"Gold " + GAMSTA._get_player_gold(player_index));
			yc+=ys;
			if (our_turn) {
				current_palette_index = [6,2][(now>>9)&1];
				put_string(xc,yc,"Your TURN");
			}
			yc+=ys;

			{
				const press = player_press[player_index];
				const input = player_input[player_index];
				for (let i = 0; i < N_CONTROLS; i++) {
					const is_press = press[i];
					const is_down = input[i];
					current_palette_index = is_down ? 6 : 2;
					let ctrl = window.CTRLS[player_index][i];
					const mapping = {".":"Period",",":"Comma"};
					const m = mapping[ctrl];
					if (m) ctrl = m;
					if (ctrl.length === 1) ctrl = ctrl.toUpperCase();
					put_string(xc, yc, "\x01\x02\x03\x04\x05\x06"[i] + lpad(ctrl, 12));
					yc+=ys;
				}
			}
		}

		{
			 switch (st._state) {
			 case CURSOR: {
			 	const press = player_press[GAMSTA._get_player_index()];
				let prev_row = st._cursor_row;
				let prev_column = st._cursor_column;
				if (press[0]) st._cursor_row--;
				if (press[1]) st._cursor_column--;
				if (press[2]) st._cursor_row++;
				if (press[3]) st._cursor_column++;
				st._cursor_row = clamp(st._cursor_row, 0, BOARD_N_ROWS-1);
				st._cursor_column = clamp(st._cursor_column, 0, BOARD_N_COLUMNS-1);
				if (st._cursor_row !== prev_row || st._cursor_column !== prev_column) {
					st._act();
					st._update_cursor();
				}
				const [bx,by] = board_get_coord(st._cursor_row, st._cursor_column);
				const m = Math.round(22 + 1-clamp((now-st._action_ts)*0.006,0,1)*6);
				current_palette_index = ((now>>7)&1) ? 3 : 7;
				draw_box(bx-m,by-m,bx+m,by+m)

				let xc = 8;
				let yc = 8;
				for (let line of st._info_lines) {
					let s = undefined;
					if (line.length === 2) {
						[current_palette_index,s] = line;
					}
					if (s) put_string(xc, yc, s);
					yc += 12;
				}
			 } break;
			 default: ASSERTION_ERROR("unhandled state");
			 }
		}

		present([
			...pal,
		]);
		rqfr(render);
	};
	rqfr(render);
};
