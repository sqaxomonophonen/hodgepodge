/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
import java.lang.foreign.Arena;
import java.lang.foreign.MemoryLayout;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.SequenceLayout;
import java.lang.foreign.ValueLayout;
import java.lang.invoke.VarHandle;
import java.util.Random;

/**
 *
 * @author user
 */
public class Perf4 {
    public final static long n = 10_000_000;        
    public final static SequenceLayout layout = MemoryLayout.sequenceLayout(n,
				MemoryLayout.structLayout(
					ValueLayout.JAVA_FLOAT.withName("x"),
					ValueLayout.JAVA_FLOAT.withName("y"),
					ValueLayout.JAVA_INT.withName("flags")
				)
			).withName("mystruct");
    public final static VarHandle xHandle = layout.varHandle(MemoryLayout.PathElement.sequenceElement(), MemoryLayout.PathElement.groupElement("x"));
    public final static VarHandle yHandle = layout.varHandle(MemoryLayout.PathElement.sequenceElement(), MemoryLayout.PathElement.groupElement("y"));
    public final static VarHandle flagsHandle = layout.varHandle(MemoryLayout.PathElement.sequenceElement(), MemoryLayout.PathElement.groupElement("flags"));
    public final static void main(String[] argv) {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment mem = arena.allocate(layout);
            Random rng = new Random();
            for (long i = 0; i < n; i++) {
                xHandle.set(mem, 0L, (long)i, rng.nextFloat());
                yHandle.set(mem, 0L, (long)i, rng.nextFloat());
                flagsHandle.set(mem, 0L, (long)i, rng.nextInt());
            }

            for (int pass = 0; pass < 10; pass++) {
                long t0 = System.nanoTime();
                float sum = 0.0f;
                int isum = 0;
                for (int i = 0; i < n; i++) {
                        float x = (float) xHandle.get(mem, 0L, (long)i);
                        float y = (float) yHandle.get(mem, 0L, (long)i);
                        int flags = (int) flagsHandle.get(mem, 0L, (long)i);
                        isum += flags;
                        float fx = x;
                        float fy = y;
                        sum += fx*fy;
                }
                long dt = System.nanoTime() - t0;
                System.out.println("sum="+sum+" isum="+isum+" "+String.format("%f", (float)n/(dt*1e-9))+" it/s");
            }
        }
    }
}
