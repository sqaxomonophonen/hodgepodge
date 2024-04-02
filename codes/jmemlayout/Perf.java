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
					ValueLayout.JAVA_INT.withName("flags")
				)
			).withName("mystruct");
			VarHandle xHandle = layout.varHandle(MemoryLayout.PathElement.sequenceElement(), MemoryLayout.PathElement.groupElement("x"));
			VarHandle yHandle = layout.varHandle(MemoryLayout.PathElement.sequenceElement(), MemoryLayout.PathElement.groupElement("y"));
			VarHandle flagsHandle = layout.varHandle(MemoryLayout.PathElement.sequenceElement(), MemoryLayout.PathElement.groupElement("flags"));
			MemorySegment mem = arena.allocate(layout);
			Random rng = new Random();
			for (int i = 0; i < n; i++) {
				xHandle.set(mem, i, rng.nextFloat());
				yHandle.set(mem, i, rng.nextFloat());
				flagsHandle.set(mem, i, rng.nextInt());
			}

			long t0 = System.nanoTime();
			float sum = 0.0f;
			for (int i = 0; i < n; i++) {
				Float x = (Float)xHandle.get(mem, i);
				Float y = (Float)yHandle.get(mem, i);
				Integer flags = (Integer)flagsHandle.get(mem, i);
				if ((flags.intValue() & 256) == 0) continue;
				float fx = x.floatValue();
				float fy = x.floatValue();
				sum += fx*fy;
			}
			long dt = System.nanoTime() - t0;
			System.out.println("sum="+sum+" "+(float)n/(dt*1e-9)+" it/s");
		}
	}
}
