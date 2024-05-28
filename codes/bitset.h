#ifndef BITSET_H

#include <stdint.h>
#include <assert.h>

#define BSDEF_WRITE(N,L) \
static inline void bs##N##_write(uint##N##_t* set, int index, int v) \
{ \
	const uint##N##_t m = 1ULL<<(index&(N-1)); \
	set[index>>L] = (set[index>>L] & ~m) | (v ? m : 0); \
}

#define BSDEF_READ(N,L) \
static inline int bs##N##_read(uint##N##_t* set, int index) \
{ \
	return 0ULL != (set[index>>L] & 1ULL<<(index&(N-1))); \
}

#define BSDEF(N,L) \
	BSDEF_READ(N,L)  \
	BSDEF_WRITE(N,L)

BSDEF(8,3)
BSDEF(16,4)
BSDEF(32,5)
BSDEF(64,6)

#undef BSDEF
#undef BSDEF_WRITE
#undef BSDEF_READ

#define BASN(set,index) assert((0 <= (index) && (index) < (8*sizeof(set))) && "bitset access out-of-bounds")

#define BAS8_WRITE(set,index,v)   (BASN(set,index),bs8_write(set,index,v))
#define BAS8_READ(set,index)      (BASN(set,index),bs8_read(set,index))

#define BAS16_WRITE(set,index,v)  (BASN(set,index),bs16_write(set,index,v))
#define BAS16_READ(set,index)     (BASN(set,index),bs16_read(set,index))

#define BAS32_WRITE(set,index,v)  (BASN(set,index),bs32_write(set,index,v))
#define BAS32_READ(set,index)     (BASN(set,index),bs32_read(set,index))

#define BAS64_WRITE(set,index,v)  (BASN(set,index),bs64_write(set,index,v))
#define BAS64_READ(set,index)     (BASN(set,index),bs64_read(set,index))

#define BITSET_H
#endif
