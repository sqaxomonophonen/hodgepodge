#ifndef DA_H // dynamic array, macro powered

// XXX be careful with macro gotchas; it's tempting to use `i++` in macros that
// accept an INDEX, but they evaluate your expression multiple times.

#include <assert.h>
#include <string.h>

void _daMaybeGrow(size_t item_size, void** pitems, int* pcap, int required_cap);

#define DA_STRUCT_BODY(TYPE)    { TYPE* items; int len,cap; }
#define DA(TYPE,NAME)           struct DA_STRUCT_BODY(TYPE) NAME

#define daLen(ARR)              ((ARR).len)
#define daItemSize(ARR)         (sizeof(*(ARR).items))
#define daSetMinCap(ARR,CAP)    (assert((CAP)>=0),\
                                _daMaybeGrow(daItemSize(ARR),(void**)&((ARR).items),&((ARR).cap),CAP))
#define daSetLen(ARR,LEN)       (daSetMinCap(ARR,LEN),assert((ARR).cap>=(LEN)),(ARR).len=(LEN))
#define daReset(ARR)            daSetLen(ARR,0)
#define daValidN(ARR,INDEX,N)   ((0<=(INDEX))&&((INDEX)<=(daLen(ARR)-(N))))
#define daValid(ARR,INDEX)      daValid(ARR,1)
#define daGuardN(ARR,INDEX,N)   assert(daValidN(ARR,INDEX,N)&&"index out of bounds")
#define daGuard(ARR,INDEX)      daGuardN(ARR,INDEX,1)
#define daUP(ARR,INDEX)         (&(ARR).items[INDEX])
#define daPtr(ARR,INDEX)        (daGuard(ARR,INDEX),daUP(ARR,INDEX))
#define daPtr0(ARR)             daPtr(ARR,0)
#define daGet(ARR,INDEX)        (*daPtr(ARR,INDEX))
#define daAddNPtr(ARR,N)        (daSetLen(ARR,(N)+daLen(ARR)),daPtr(ARR,daLen(ARR)-(N)))
#define daPut(ARR,ITEM)         (*daAddNPtr(ARR,1)=(ITEM))
#define daInsNPtr(ARR,INDEX,N)  ((void)daAddNPtr(ARR,N),\
                                memmove(daUP(ARR,(INDEX)+(N)),daPtr(ARR,INDEX),daItemSize(ARR)*(daLen(ARR)-(N)-(INDEX))),\
                                daPtr(ARR,INDEX))
#define daIns(ARR,INDEX,ITEM)   (*(daInsNPtr(ARR,INDEX,1))=(ITEM))
#define daSet(ARR,INDEX,ITEM)   (*daPtr(ARR,INDEX)=(ITEM))
#define daDelN(ARR,INDEX,N)     (daGuardN(ARR,INDEX,N),\
                                memmove(daPtr(ARR,INDEX),1+daPtr(ARR,INDEX),daItemSize(ARR)*(daLen(ARR)-(INDEX)-(N))),\
                                daSetLen(ARR,daLen(ARR)-1))
#define daDel(ARR,INDEX)        daDelN(ARR,INDEX,1)
#define daPop(ARR)              (daSetLen(ARR,daLen(ARR)-1),(ARR).items[daLen(ARR)])

#define daCopy(DST,SRC)         ((void)(DST.items==SRC.items), /* try to emit warning if not same type */ \
                                daSetLen(DST,daLen(SRC)),\
                                memcpy(DST.items, SRC.items, daItemSize(DST)*daLen(DST)))

// BACKGROUND: I previously used the arr*() macros in stb_ds.h, but:
//  - I declared arrays with an `_arr`-suffix to distinguish them from other
//    pointers/arrays. It bothered me a bit because I'm not a fan of Hungarian
//    notation and the like, but it starts making sense when even the
//    declaration is misleading? Also, as a convention it should be documented.
//  - I tend to check that indices are within array bounds, but the arr*()
//    macros leave that to you. So, using daGet() (which does bounds checks)
//    instead of direct access isn't really inconvenient for me.
// (inspired by tsoding, reimplemented from memory)
// XXX I ended up going back to arr*() again :) Reasons:
//  - I ended up doing a few daGet(xs,i++) blunders; arr*() "cleverly" avoids
//    this by not having an arrget()? :)
//  - Bounds checks are great, but I did it manually everywhere anyway? Also,
//    maybe I can add a bounds checking macro?
//  - I started to need custom allocators (and "allocation contexts"), which
//    was half-way there in stb_ds.h


#ifdef DA_IMPLEMENTATION

#include <stdlib.h>

void _daMaybeGrow(size_t item_size, void** pitems, int* pcap, int required_cap)
{
	assert(required_cap >= 0);
	if (*pcap >= required_cap) return;
	const int cap0 = *pcap;
	int cap = *pcap;
	if (cap == 0) cap = 8;
	while (cap < required_cap) { assert(cap>0); cap <<= 1; }
	assert(cap > cap0);
	void* p = realloc(*pitems, item_size * cap);
	assert(p != NULL);
	*pitems = p;
	*pcap = cap;
}

#endif

#define DA_H
#endif
