
var FTX = function () {
	var N = 32;

	var signal = [];

	for (var i = 0; i < N; i++) {
		signal.push(i == N/2 ? 1 : 0);
		//signal.push(Math.sin(i/10));
	}

	var real = [];
	var imag = [];

	var forward = function () {
		real = [];
		imag = [];
		for (var k = 0; k < N; k++) {
			var Zr = 0;
			var Zi = 0;
			for (var n = 0; n < N; n++) {
				var x = signal[n];
				var phi = -2 * Math.PI * k * (n/N);
				Zr += x * Math.cos(phi);
				Zi += x * Math.sin(phi);
			}
			real.push(Zr);
			imag.push(Zi);
		}
	};

	var inverse = function () {
	};

	forward();

	this.signal_interpolate = function (x) {
		var Z = 0;
		for (var k = 0; k < N; k++) {
			var phi = 2 * Math.PI * k * (x/N);
			Z += real[k] * Math.cos(phi);
		}
		return Z/N;
	};

	this.get_signal = function () {
		return signal;
	};

	this.get_real = function () {
		return real;
	};

	this.get_imag = function () {
		return imag;
	};

	this.set_signal = function (index, value) {
		if (index >= 0 && index < N) {
			signal[index] = value;
		}
		forward();
	};

	this.set_real = function (index, value) {
		inverse();
	};

	this.set_imag = function (index, value) {
		inverse();
	};
};

window.onload = function () {
	var screen = document.getElementById('screen');
	var ctx = screen.getContext('2d');
	if (!ctx) {
		alert("no canvas context!");
		return;
	}

	var ftx = new FTX();

	var get_area = function (w, h, i) {
		var spacing = 32;

		var ah = (h - spacing * 4) / 3;

		return {
			x: spacing,
			y: spacing * (i+1) + ah * i,
			w: w - spacing * 2,
			h: ah
		};
	};

	var selected_type = "signal";
	var selected_index = 32;
	var width;
	var height;

	var area_margin = 12;

	var draw = function () {
		width = window.innerWidth;
		height = window.innerHeight;
		screen.width = width;
		screen.height = height;

		ctx.fillStyle = 'black';
		ctx.fillRect(0, 0, width, height);

		ctx.fillStyle = 'white';

		var a;
		a = get_area(width, height, 0);

		ctx.fillStyle = selected_type === "signal" ? '#002030' : '#001020';
		ctx.fillRect(a.x, a.y, a.w, a.h);
		var lw = 0.5;
		var signal = ftx.get_signal();
		ctx.strokeStyle = '#aa2030';
		ctx.beginPath();
		var osmp = 16;
		for (var i = 0; i < (signal.length-1)*osmp+1; i++) {
			var x = i / osmp;
			var r = ftx.signal_interpolate(x);
			var x = a.x + a.w/(signal.length*2) + (i/osmp)*(a.w/signal.length);
			var dy = -r * (a.h - area_margin*2) * 0.5;
			var y = a.y + a.h/2 + dy;

			if (i === 0) {
				ctx.moveTo(x, y);
			} else {
				ctx.lineTo(x, y);
			}
		}
		ctx.stroke();
		ctx.fillStyle = '#304050';
		ctx.beginPath();
		ctx.rect(a.x+a.w/(signal.length*2), a.y + a.h/2, a.w-(a.w/signal.length), lw);
		ctx.fill();
		for (var i = 0; i < signal.length; i++) {
			var r = signal[i];
			var dy = -r * (a.h - area_margin*2) * 0.5;
			var cx = a.x + a.w/(signal.length*2) + i*(a.w/signal.length);
			var cy = a.y + a.h/2 + dy;

			var selected = selected_type === "signal" && selected_index == i;
			ctx.fillStyle = selected ? '#ffffff' : '#bbaa99';
			ctx.beginPath();
			ctx.rect(cx-lw/2, a.y + a.h/2, lw, dy);
			ctx.fill();
			ctx.beginPath();
			ctx.arc(cx, cy, selected ? 4 : 3, 0, Math.PI*2, false);
			ctx.fill();
		}

		var draw_complex = function (a, type, vs) {
			ctx.fillStyle = selected_type === type ? '#002030' : '#001020';
			ctx.fillRect(a.x, a.y, a.w, a.h);

			var max = 1;
			for (var i = 0; i < vs.length; i++) {
				var abs = Math.abs(vs[i]);
				if (abs > max) max = abs;
			}

			for (var i = 0; i < vs.length; i++) {
				var r = vs[i];
				var dy = -r/max * (a.h - area_margin*2) * 0.5;
				var cx = a.x + a.w/(signal.length*2) + i*(a.w/signal.length);
				var cy = a.y + a.h/2;

				var selected = selected_type === type && selected_index == i;
				ctx.fillStyle = selected ? '#ffffff' : '#bbaa99';
				ctx.beginPath();
				ctx.rect(cx - 10, cy, 20, dy);
				ctx.fill();
			}

			a = get_area(width, height, 2);
		};

		draw_complex(get_area(width, height, 1), "real", ftx.get_real());
		draw_complex(get_area(width, height, 2), "imag", ftx.get_imag());

	};

	draw();

	var in_area = function (x, y, a) {
		return x >= a.x && x < (a.x+a.w) && y >= a.y && y < (a.y+a.h);
	};

	var act = (function () {
		var drag = false;
		var lock_index = null;
		var lock_type = null;
		return function (x, y, act) {
			var type = null;
			var index = 0;
			var value = 0;

			if (x !== null) {
				var a0 = get_area(width, height, 0);
				var a1 = get_area(width, height, 1);
				var a2 = get_area(width, height, 2);

				if (lock_type === "signal" || in_area(x, y, a0)) {
					type = "signal";
					var n = ftx.get_signal().length;

					index = parseInt((x - a0.x) / (a0.w / n));
					index = Math.max(Math.min(n-1, index), 0);

					value = ((y - a0.y - area_margin) / (a0.h-area_margin*2)) * -2 + 1;
					value = Math.max(Math.min(1, value), -1);
				} else if (lock_type === "real" || in_area(x, y, a1)) {
					type = "real";
				} else if (lock_type === "imag" || in_area(x, y, a2)) {
					type = "imag";
				} else {
					type = null;
				}
			}

			var set_value = function () {
				if (lock_type == "signal") {
					ftx.set_signal(lock_index, value);
				}
			};

			if (act === "move" && !drag) {
				selected_type = type;
				selected_index = index;
			} else if (act === "down") {
				selected_type = lock_type = type;
				selected_index = lock_index = index;
				drag = true;
				set_value();
			} else if (act == "move" && drag) {
				set_value();
			} else if (act == "up") {
				drag = false;
				lock_type = null;
				lock_index = null;
			}
			draw();
		};
	})();

	var mousedown = function (x, y) {
		act(x, y, "down");
	};

	var mouseup = function () {
		act(null, null, "up");
	};

	var mousemove = function (x, y) {
		act(x, y, "move");
	};

	window.onresize = function (e) {
		draw();
	};

	screen.onmousedown = function (e) {
		mousedown(e.layerX, e.layerY);
	};

	screen.onmouseup = function (e) {
		mouseup();
	};

	screen.onmouseleave = function (e) {
		mouseup();
	};

	screen.onmousemove = function (e) {
		mousemove(e.layerX, e.layerY);
	};
};
