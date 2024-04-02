import java.lang.foreign.*;
import java.lang.invoke.*;
import java.util.Random;

class Perf {
	public final static void main(String[] argv) {
		if (argv.length != 1) {
			System.out.println("Usage: perf <n>");
			return;
		}
		int n = Integer.parseInt(argv[0]);
		System.out.println("N=" + n);
		try (Arena arena = Arena.ofConfined()) {
			SequenceLayout layout = MemoryLayout.sequenceLayout(n,
				MemoryLayout.structLayout(
					ValueLayout.JAVA_FLOAT.withName("x"),
					ValueLayout.JAVA_FLOAT.withName("y"),
					ValueLayout.JAVA_INT.withName("i")
				)
			).withName("mystruct");
			VarHandle xHandle = layout.varHandle(MemoryLayout.PathElement.sequenceElement(), MemoryLayout.PathElement.groupElement("x"));
			VarHandle yHandle = layout.varHandle(MemoryLayout.PathElement.sequenceElement(), MemoryLayout.PathElement.groupElement("y"));
			VarHandle iHandle = layout.varHandle(MemoryLayout.PathElement.sequenceElement(), MemoryLayout.PathElement.groupElement("i"));
			MemorySegment mem = arena.allocate(layout);
			Random rng = new Random();
			for (int i = 0; i < n; i++) {
				xHandle.set(mem, i, rng.nextFloat());
				yHandle.set(mem, i, rng.nextFloat());
				iHandle.set(mem, i, rng.nextInt());
			}

			for (int pass = 0; pass < 10; pass++) {
				long t0 = System.nanoTime();
				float sum = 0.0f;
				int sum2 = 0;
				for (int it = 0; it < n; it++) {
					Float x = (Float)xHandle.get(mem, it);
					Float y = (Float)yHandle.get(mem, it);
					Integer i = (Integer)iHandle.get(mem, it);
					sum2 += i.intValue();
					float fx = x.floatValue();
					float fy = x.floatValue();
					sum += fx*fy;
				}
				long dt = System.nanoTime() - t0;
				System.out.println("sum="+sum+" sum2="+sum2+" "+(float)n/(dt*1e-9)+" it/s");
			}
		}
	}
}
