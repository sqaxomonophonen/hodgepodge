import java.util.Random;

class MyStruct {
	float x;
	float y;
	int i;
}

class Perf2 {
	public final static void main(String[] argv) {
		if (argv.length != 1) {
			System.out.println("Usage: perf <n>");
			return;
		}
		int n = Integer.parseInt(argv[0]);
		MyStruct[] mem = new MyStruct[n];
		Random rng = new Random();
		for (int i = 0; i < n; i++) {
			mem[i] = new MyStruct();
			mem[i].x = rng.nextFloat();
			mem[i].y = rng.nextFloat();
			mem[i].i = rng.nextInt();
		}

		for (int pass = 0; pass < 10; pass++) {
			long t0 = System.nanoTime();
			float sum = 0.0f;
			int sum2 = 0;
			for (int i = 0; i < n; i++) {
				MyStruct e = mem[i];
				sum2 += e.i;
				sum += e.x*e.y;
			}
			long dt = System.nanoTime() - t0;
			System.out.println("sum="+sum+" sum2="+sum2+" "+String.format("%f", (float)n/(dt*1e-9))+" it/s");
		}
	}
}

