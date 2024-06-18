
// TODO
//  - gradients? maybe circular and/or gaussian too? in which case: I should
//    probably allow squishing it because tx doesn't allow this (
//  - bsm_o21_multiply()
//  - SIMD support?
//  - It might be faster to render one block at a time in the final image.
//    It would probably involve queueing draw calls and executing them in
//    bsm_end_bitmap(), instead of executing them immediately. It might also be
//    faster to treeat memset'd blocks as special cases.

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bsm.h"

#define BLOCK_SIZE_LOG2 (3)
#define BLOCK_SIZE      (1 << BLOCK_SIZE_LOG2)
#define BLOCK_N         (1 << (2*BLOCK_SIZE_LOG2))

static const float fpi = 3.141592653589793f;

struct tx {
	// transform that supports translation, rotation and scaling
	// corresponds to a 3x2 2D affine matrix but one basis defined from the
	// other (which disallows skewing and scaling each axis independently)
	float basis_x,  basis_y; // also defines scale (see tx_get_scale())
	float origin_x, origin_y;
};

static inline float tx_get_scale(struct tx* tx)
{
	return sqrtf(tx->basis_x*tx->basis_x + tx->basis_y*tx->basis_y);
}

static inline void tx_invert(struct tx* tx)
{
	const float I = 1.0f / (tx->basis_x*tx->basis_x + tx->basis_y*tx->basis_y);
	const float bx =  I * tx->basis_x;
	const float by = -I * tx->basis_y;
	const float ox = I * ((-tx->basis_y * tx->origin_y) - (tx->basis_x * tx->origin_x));
	const float oy = I * (( tx->basis_y * tx->origin_x) - (tx->basis_x * tx->origin_y));
	tx->basis_x = bx;
	tx->basis_y = by;
	tx->origin_x = ox;
	tx->origin_y = oy;
}

#define INDEX_STACK_SIZE  (256)
#define TX_STACK_SIZE     (64)
#define FREE_STACK_SIZE   (256)
#define MAX_QUERIES       (256)
#define MxN_MAX_ELEMENTS  (32) // MxN_MAX_ELEMENTS^2 elements required by `struct bsm`

enum mode {
	NONE = 0,
	BITMAP,
	BITMAPMxN,
	QUERIES
};

struct bsm {
	enum mode mode;

	int tx_stack_height;
	struct tx tx_stack[TX_STACK_SIZE];

	size_t value_size;
	uint8_t* scratch;
	size_t scratch_cap_bytes;

	int index_stack_height;
	int index_stack[INDEX_STACK_SIZE];

	int free_stack_height;
	int free_stack[FREE_STACK_SIZE];

	int supersampling;

	union {
		struct {
			float pixels_per_unit;
			int width, height;
			int n_columns, n_rows;
			float pp0x, pp0y;
		} bitmap;

		struct {
			float pixels_per_unit;
			int n_widths;
			int n_heights;
			int n_cells; // n_widths * n_heights
			int widths[MxN_MAX_ELEMENTS];
			int heights[MxN_MAX_ELEMENTS];
			float p0x[MxN_MAX_ELEMENTS];
			float p0y[MxN_MAX_ELEMENTS];
			int s0x[MxN_MAX_ELEMENTS];
			int s0y[MxN_MAX_ELEMENTS];
			int offsets[MxN_MAX_ELEMENTS*MxN_MAX_ELEMENTS];
			int plan[2*MxN_MAX_ELEMENTS*MxN_MAX_ELEMENTS];
			int n_renders[MxN_MAX_ELEMENTS*MxN_MAX_ELEMENTS];
		} bitmapMxN;

		struct {
			int n;
			float coord_pairs[2*MAX_QUERIES];
		} queries;
	};
};

struct bsm* bsm_new(void)
{
	struct bsm* bsm = calloc(1, sizeof *bsm);
	return bsm;
}

void bsm_free(struct bsm* bsm)
{
	if (bsm->scratch) free(bsm->scratch);
	free(bsm);
}

static inline int resolve_stack_index(struct bsm* bsm, int i)
{
	if (i < 0) i = bsm->index_stack_height + i;
	assert(0 <= i && i < bsm->index_stack_height);
	return i;
}

// be careful not to call this before doing any stack manipulation, as this may
// change the actual pointer
static uint8_t* deref_value(struct bsm* bsm, int ref)
{
	const size_t sz = bsm->value_size;
	assert(0 <= ref && (ref+1)*sz <= bsm->scratch_cap_bytes);
	return &bsm->scratch[sz * ref];
}

static uint8_t* deref_top(struct bsm* bsm)
{
	return deref_value(bsm, bsm->index_stack[resolve_stack_index(bsm, -1)]);
}

static void reset_stacks(struct bsm* bsm)
{
	bsm->index_stack_height = 0;
	bsm->free_stack_height = 0;
	bsm->tx_stack_height = 1;
	bsm->tx_stack[0] = (struct tx) {
		.basis_x = 1,
		.basis_y = 0,
		.origin_x = 0,
		.origin_y = 0,
	};
}


static inline int calc_n_blocks(int size)
{
	return (size + (1<<BLOCK_SIZE_LOG2) - 1) >> BLOCK_SIZE_LOG2;
}

void bsm_begin_bitmap(struct bsm* bsm, float pixels_per_unit, int width, int height)
{
	assert(bsm->mode == NONE);
	bsm->bitmap.pixels_per_unit = pixels_per_unit;
	bsm->bitmap.width = width;
	bsm->bitmap.height = height;
	bsm->bitmap.n_columns = calc_n_blocks(width);
	bsm->bitmap.n_rows    = calc_n_blocks(height);
	bsm->bitmap.pp0x = (float)width * -0.5f;
	bsm->bitmap.pp0y = (float)height * -0.5f;
	bsm->value_size = (bsm->bitmap.n_columns << BLOCK_SIZE_LOG2) * (bsm->bitmap.n_rows << BLOCK_SIZE_LOG2);
	reset_stacks(bsm);
	bsm->mode = BITMAP;
}

static void bsm_blit_bitmap(struct bsm* bsm, int i, uint8_t* out, int stride)
{
	assert(bsm->mode == BITMAP);
	const int n_rows = bsm->bitmap.n_rows;
	const int n_columns = bsm->bitmap.n_columns;
	uint8_t* readp = deref_value(bsm, bsm->index_stack[resolve_stack_index(bsm, i)]);
	for (int row = 0; row < n_rows; row++) {
		for (int column = 0; column < n_columns; column++) {
			uint8_t* p = out + (column << BLOCK_SIZE_LOG2) + (row << BLOCK_SIZE_LOG2) * stride;
			const int mx = bsm->bitmap.width  - column * BLOCK_SIZE;
			const int my = bsm->bitmap.height -    row * BLOCK_SIZE;
			const int nx = mx < BLOCK_SIZE ? mx : BLOCK_SIZE;
			const int ny = my < BLOCK_SIZE ? my : BLOCK_SIZE;
			for (int y = 0; y < ny; y++) {
				memcpy(p, readp, nx);
				p     += stride;
				readp += BLOCK_SIZE;
			}
			readp+=(BLOCK_SIZE-ny)*BLOCK_SIZE;
		}
	}
}


void bsm_end_bitmap(struct bsm* bsm, uint8_t* out, int stride)
{
	assert(bsm->mode == BITMAP);
	bsm_blit_bitmap(bsm, -1, out, stride);
	bsm->mode = NONE;
}

static void axisMxN(float pixels_per_unit, const int* sizes, int* out_sizes, int* out_n, float* out_o0s, int* out_s0s)
{
	int n = 0;
	float o0 = 0.0f;
	int s0 = 0;
	for (const int* p = sizes; *p != 0; n++, p++) {
		assert(n < MxN_MAX_ELEMENTS);
		const int s = *p;
		if (out_sizes) out_sizes[n] = s;
		if (out_o0s) out_o0s[n] = o0;
		o0 += (float)(s>0?s:0);
		if (out_s0s) out_s0s[n] = s0;
		s0 += s>0?s:1;
	}
	if (out_n) *out_n = n;
	if (out_o0s) {
		// center ordinates
		const float d = 0.5f*o0;
		for (int i = 0; i < n; i++) out_o0s[i] -= d;
	}
}

void bsm_begin_bitmapMxN(struct bsm* bsm, float pixels_per_unit, const int* widths, const int* heights)
{
	assert(bsm->mode == NONE);

	bsm->bitmapMxN.pixels_per_unit = pixels_per_unit;

	axisMxN(pixels_per_unit , widths  , bsm->bitmapMxN.widths  , &bsm->bitmapMxN.n_widths  , bsm->bitmapMxN.p0x , bsm->bitmapMxN.s0x);
	axisMxN(pixels_per_unit , heights , bsm->bitmapMxN.heights , &bsm->bitmapMxN.n_heights , bsm->bitmapMxN.p0y , bsm->bitmapMxN.s0y);
	const int nc = bsm->bitmapMxN.n_cells = bsm->bitmapMxN.n_widths * bsm->bitmapMxN.n_heights;

	int offset = 0;
	int* offsets = bsm->bitmapMxN.offsets;
	int* plan = bsm->bitmapMxN.plan;
	int* n_renders = bsm->bitmapMxN.n_renders;
	enum { BLOCK=0, NONBLOCK, N_KIND };
	for (int kind = 0; kind < N_KIND; kind++) {
		for (int y = 0; y < bsm->bitmapMxN.n_heights; y++) {
			for (int x = 0; x < bsm->bitmapMxN.n_widths; x++) {
				const int w = bsm->bitmapMxN.widths[x];
				const int h = bsm->bitmapMxN.heights[y];
				assert(w != 0 && h != 0);
				int doff = 0;
				int nr = 0;
				if (w > 0 && h > 0) {
					if (kind == BLOCK) {
						nr = calc_n_blocks(w) * calc_n_blocks(h);
						doff = nr << (2*BLOCK_SIZE_LOG2);
					}
				} else if (w > 0 && h < 0) {
					if (kind == NONBLOCK) {
						nr = 1;
						doff = w;
					}
				} else if (w < 0 && h > 0) {
					if (kind == NONBLOCK) {
						nr = 1;
						doff = h;
					}
				} else if (w < 0 && h < 0) {
					if (kind == NONBLOCK) {
						nr = 1;
						doff = 1;
					}
				} else {
					assert(!"unreachable!");
				}
				if (doff) {
					assert(doff > 0);
					*(offsets++) = offset;
					*(plan++) = x;
					*(plan++) = y;
					*(n_renders++) = nr;
					offset += doff;
				}
			}
		}
	}
	assert((offsets - bsm->bitmapMxN.offsets) == nc);
	assert((plan    - bsm->bitmapMxN.plan)    == 2*nc);
	bsm->value_size = offset;
	reset_stacks(bsm);
	bsm->mode = BITMAPMxN;
}

void bsm_end_bitmapMxN(struct bsm* bsm, uint8_t* out, int stride)
{
	assert(bsm->mode == BITMAPMxN);
	const int n = bsm->bitmapMxN.n_cells;
	uint8_t* top = deref_top(bsm);
	for (int i = 0; i < n; i++) {
		uint8_t* readp = top + bsm->bitmapMxN.offsets[i];
		const int cell_x = bsm->bitmapMxN.plan[   i<<1 ];
		const int cell_y = bsm->bitmapMxN.plan[1+(i<<1)];
		uint8_t* p0 = out + bsm->bitmapMxN.s0x[cell_x] + stride*bsm->bitmapMxN.s0x[cell_y];
		const int w = bsm->bitmapMxN.widths[cell_x];
		const int h = bsm->bitmapMxN.heights[cell_y];
		const int pu = w > 0;
		const int pv = h > 0;
		if (pu && pv) {
			assert(w>0 && h>0);
			const int n_columns = calc_n_blocks(w);
			const int n_rows    = calc_n_blocks(h);
			for (int row = 0; row < n_rows; row++) {
				for (int column = 0; column < n_columns; column++) {
					uint8_t* p1 = p0 + (column << BLOCK_SIZE_LOG2) + (row << BLOCK_SIZE_LOG2) * stride;
					const int mx = w - column * BLOCK_SIZE;
					const int my = h -    row * BLOCK_SIZE;
					const int nx = mx < BLOCK_SIZE ? mx : BLOCK_SIZE;
					const int ny = my < BLOCK_SIZE ? my : BLOCK_SIZE;
					for (int y = 0; y < ny; y++) {
						memcpy(p1, readp, nx);
						p1    += stride;
						readp += BLOCK_SIZE;
					}
					readp+=(BLOCK_SIZE-ny)*BLOCK_SIZE;
				}
			}
		} else if (pu && !pv) {
			assert(w > 0);
			memcpy(p0, readp, w);
		} else if (!pu && pv) {
			assert(h > 0);
			uint8_t* p1 = p0;
			for (int i = 0; i < h; i++, readp++, p1 += stride) *p1 = *readp;
		} else if (!pu && !pv) {
			*p0 = *readp;
		} else {
			assert(!"unreachable");
		}
	}

	bsm->mode = NONE;
}

void bsm_begin_queries(struct bsm* bsm, int n, float* coord_pairs)
{
	assert(bsm->mode == NONE);
	assert(n <= MAX_QUERIES);
	bsm->queries.n = n;
	bsm->value_size = n;
	memcpy(bsm->queries.coord_pairs, coord_pairs, 2*n*sizeof(bsm->queries.coord_pairs[0]));
	reset_stacks(bsm);
	bsm->mode = QUERIES;
}

void bsm_end_queries(struct bsm* bsm, int n, int* inside)
{
	assert(bsm->mode == QUERIES);
	assert(n <= MAX_QUERIES);
	assert(n <= bsm->queries.n);
	uint8_t* v = deref_top(bsm);
	for (int i = 0; i < n; i++) {
		inside[i] = !!v[i];
	}
	bsm->mode = NONE;
}

struct tx* current_tx(struct bsm* bsm)
{
	assert(bsm->tx_stack_height > 0);
	return &bsm->tx_stack[bsm->tx_stack_height-1];
}

void bsm_tx_save(struct bsm* bsm)
{
	assert(bsm->tx_stack_height < TX_STACK_SIZE);
	bsm->tx_stack[bsm->tx_stack_height++] = *current_tx(bsm);
}

void bsm_tx_restore(struct bsm* bsm)
{
	assert(bsm->tx_stack_height > 1);
	bsm->tx_stack_height--;
}

void bsm_tx_translate(struct bsm* bsm, float x, float y)
{
	struct tx* tx = current_tx(bsm);
	tx->origin_x += (x*tx->basis_x - y*tx->basis_y);
	tx->origin_y += (x*tx->basis_y + y*tx->basis_x);
}

static inline float bsm__deg2rad(float deg) { return deg * 0.017453292519943295f; }

void bsm_tx_rotate(struct bsm* bsm, float degrees)
{
	struct tx* tx = current_tx(bsm);
	const float radians = bsm__deg2rad(degrees);
	const float u = cosf(radians);
	const float v = sinf(radians);
	const float bx = u*tx->basis_x - v*tx->basis_y;
	const float by = u*tx->basis_y + v*tx->basis_x;
	tx->basis_x = bx;
	tx->basis_y = by;
}

void bsm_tx_scale(struct bsm* bsm, float scalar)
{
	struct tx* tx = current_tx(bsm);
	tx->basis_x *= scalar;
	tx->basis_y *= scalar;
}

static void kernel_memset(uint8_t* p, uint8_t v8)
{
	memset(p, v8, sizeof(p[0])*BLOCK_N);
}

#if 0
#define TEST_OFFSET 44 // try this if you want to visualize test block memset optimizations
#else
#define TEST_OFFSET 0
#endif

static void kernel_inside(uint8_t* p)
{
	kernel_memset(p, 255-TEST_OFFSET);
}

static void kernel_outside(uint8_t* p)
{
	kernel_memset(p, TEST_OFFSET);
}

static int int_compar(const void* va, const void* vb)
{
	return *(const int*)va - *(const int*)vb;
}

static void push_ref(struct bsm* bsm, int ref)
{
	assert(bsm->index_stack_height < INDEX_STACK_SIZE);
	bsm->index_stack[bsm->index_stack_height++] = ref;
}

static int pop_ref(struct bsm* bsm)
{
	assert((bsm->index_stack_height > 0) && "index stack underflow");
	return bsm->index_stack[--bsm->index_stack_height];
}

static int get_avail(struct bsm* bsm)
{
	return bsm->scratch_cap_bytes / bsm->value_size;
}

// repopulates the "free stack" so that unused bitmaps may be re-allocated.
// this is not done automatically because you must be able to pop bitmaps from
// the stack and use them before they "evaporate".
static void collect_garbage(struct bsm* bsm)
{
	int index_tmp[INDEX_STACK_SIZE];
	static_assert(sizeof(index_tmp) < (1<<18));
	const int ns = bsm->index_stack_height;
	memcpy(index_tmp, bsm->index_stack, ns * sizeof(index_tmp[0]));
	qsort(index_tmp, ns, sizeof(index_tmp[0]), int_compar);
	const int na = get_avail(bsm);
	int c = 0;
	bsm->free_stack_height = 0;
	for (int i = 0; i < na; i++) {
		while (c < ns && i > index_tmp[c]) c++;
		if (c >= ns || i !=index_tmp[c]) {
			if (bsm->free_stack_height >= FREE_STACK_SIZE) return;
			bsm->free_stack[bsm->free_stack_height++] = i;
		}
	}
}

static int new_ref(struct bsm* bsm)
{
	int ref = -1;
	const int n_avail = get_avail(bsm);
	if (bsm->free_stack_height > 0) {
		ref = bsm->free_stack[--bsm->free_stack_height];
	} else {
		ref = n_avail;
	}
	assert(ref >= 0);
	if (ref >= n_avail) {
		const size_t newcap = (ref+1) * bsm->value_size;
		bsm->scratch_cap_bytes = newcap;
		bsm->scratch = realloc(bsm->scratch, newcap);
		assert(bsm->scratch != NULL);
	}
	push_ref(bsm, ref);
	return ref;
}

void bsm_pick(struct bsm* bsm, int stack_index)
{
	push_ref(bsm, bsm->index_stack[resolve_stack_index(bsm, stack_index)]);
}

void bsm_swap(struct bsm* bsm)
{
	const int i0 = resolve_stack_index(bsm, -2);
	const int i1 = resolve_stack_index(bsm, -1);
	const int tmp = bsm->index_stack[i0];
	bsm->index_stack[i0] = bsm->index_stack[i1];
	bsm->index_stack[i1] = tmp;
}

void bsm_drop(struct bsm* bsm)
{
	assert((bsm->index_stack_height--) > 0);
	collect_garbage(bsm);
}

struct render {
	struct bsm* bsm;
	uint8_t* value;
	uint8_t* p;
	float x,y;
	float distdet;
	float distdetsqr;
	union {
		struct {
			int ib;
		} bitmap;
		struct {
			int im0,im1;
		} bitmapMxN;
		struct {
			struct tx tx;
			int qi;
		} queries;
	};

	float x0, y0, rx, ry;

	float nscale;
	float base_udx, base_udy;
	float udx0, udy0, vdx0, vdy0;
	float udx1, udy1, vdx1, vdy1;
	int nx, ny;
	int subnx, subny;
};

static void setup_render(struct render* r, int pu, int pv, int npx)
{
	const enum mode mode = r->bsm->mode;
	assert((mode == BITMAP) || (mode == BITMAPMxN));

	const int ss = r->bsm->supersampling <= 0 ? BSM_DEFAULT_SUPERSAMPLING : r->bsm->supersampling;

	const float udx = r->base_udx;
	const float udy = r->base_udy;

	r->udx0 = pu ?  udx : 0.0f;
	r->udy0 = pu ?  udy : 0.0f;
	r->vdx0 = pv ? -udy : 0.0f;
	r->vdy0 = pv ?  udx : 0.0f;

	const float ssi = 1.0f / (float)ss;
	r->udx1 = r->udx0 * ssi;
	r->udy1 = r->udy0 * ssi;
	r->vdx1 = r->vdx0 * ssi;
	r->vdy1 = r->vdy0 * ssi;

	r->nx = pu ? npx : 1;
	r->ny = pv ? npx : 1;
	r->subnx = pu ? ss : 1;
	r->subny = pv ? ss : 1;
	r->nscale = 256.0f / (float)(r->subnx * r->subny);
}

static inline struct render render_begin(struct bsm* bsm)
{
	assert(0 < bsm->tx_stack_height && bsm->tx_stack_height <= TX_STACK_SIZE);
	struct render r = {
		.bsm=bsm,
		.value=deref_value(bsm, new_ref(bsm)),
	};

	float ppu = 0.0f;
	switch (bsm->mode) {
	case BITMAP:     ppu = bsm->bitmap.pixels_per_unit;    break;
	case BITMAPMxN:  ppu = bsm->bitmapMxN.pixels_per_unit; break;
	default: break;
	}

	switch (bsm->mode) {
	case BITMAP:
	case BITMAPMxN: {
		assert(ppu > 0.0f);
		const float hbs = ((0.5f * (float)BLOCK_SIZE) / ppu);
		struct tx itx = *(current_tx(bsm));
		tx_invert(&itx);
		const float block_dmax = sqrtf(hbs*hbs*2.0f) * tx_get_scale(&itx);
		r.distdet = block_dmax;
		r.distdetsqr = block_dmax * block_dmax;
		r.base_udx = itx.basis_x / ppu;
		r.base_udy = itx.basis_y / ppu;
		r.x0 = itx.origin_x;
		r.y0 = itx.origin_y;
	}	break;

	case QUERIES: break;
	default: assert(!"unhandled case");
	}

	switch (bsm->mode) {

	case BITMAP: {
		r.bitmap.ib = -1;
		setup_render(&r, 1, 1, BLOCK_SIZE);
	}	break;

	case BITMAPMxN: {
		r.bitmapMxN.im0 = -1;
		r.bitmapMxN.im1 = -1;
	}	break;

	case QUERIES: {
		r.queries.qi = -1;
		r.queries.tx = *(current_tx(bsm));
		r.distdet = 0;
		r.distdetsqr = 0;
	}	break;

	default: assert(!"unhandled case");

	}

	return r;
}

static void setup_xy(struct render* r, float px, float py)
{
	r->x = r->x0 + px*r->udx0 + py*r->vdx0;
	r->y = r->y0 + px*r->udy0 + py*r->vdy0;
	r->rx = r->x + (-0.5f*(float)BLOCK_SIZE)*r->udx0 + (-0.5f*(float)BLOCK_SIZE)*r->vdx0;
	r->ry = r->y + (-0.5f*(float)BLOCK_SIZE)*r->udy0 + (-0.5f*(float)BLOCK_SIZE)*r->vdy0;
}

static inline int render_next(struct render* r)
{
	struct bsm* bsm = r->bsm;
	if (bsm == NULL) return 0;

	switch (bsm->mode) {

	case BITMAP: {
		const int bi = ++r->bitmap.ib;
		const int column = bi % bsm->bitmap.n_columns;
		const int row    = bi / bsm->bitmap.n_columns;
		if (row >= bsm->bitmap.n_rows) {
			bsm = NULL;
			return 0;
		}
		assert(0 <= bi && ((1+bi) * BLOCK_N) <= bsm->value_size);

		r->p = &r->value[bi * BLOCK_N];

		const float px = bsm->bitmap.pp0x + (float)BLOCK_SIZE * ((float)column + 0.5f);
		const float py = bsm->bitmap.pp0y + (float)BLOCK_SIZE * ((float)row    + 0.5f);
		setup_xy(r, px, py);

		return 1;
	}	break;

	case BITMAPMxN: {
		const int im1 = ++r->bitmapMxN.im1;
		if (im1 == 0) {
			const int im0 = ++r->bitmapMxN.im0;
			if (im0 >= bsm->bitmapMxN.n_cells) {
				r->bsm = NULL;
				return 0;
			}
		}
		const int im0 = r->bitmapMxN.im0;

		const int cell_x = bsm->bitmapMxN.plan[   im0<<1 ];
		const int cell_y = bsm->bitmapMxN.plan[1+(im0<<1)];
		const int w = bsm->bitmapMxN.widths[cell_x];
		const int h = bsm->bitmapMxN.heights[cell_y];
		const int pu = w>0;
		const int pv = h>0;
		float px = bsm->bitmapMxN.p0x[cell_x];
		float py = bsm->bitmapMxN.p0y[cell_y];
		int o2=0;
		int npx;
		int dx=0,dy=0;
		if (pu && pv) {
			assert(im1 >= 0);
			const int n = calc_n_blocks(w);
			dx = im1 % n;
			dy = im1 / n;
			o2 = im1*BLOCK_N;
			npx = BLOCK_SIZE;
		} else if (pu && !pv) {
			assert(w > 0);
			npx = w;
		} else if (!pu && pv) {
			assert(h > 0);
			npx = h;
		} else if (!pu && !pv) {
			npx = 1;
		} else {
			assert(!"unreachable");
		}
		px += (float)(BLOCK_SIZE) * (0.5f + (float)dx);
		py += (float)(BLOCK_SIZE) * (0.5f + (float)dy);

		const int offset = bsm->bitmapMxN.offsets[im0];
		r->p = &r->value[offset+o2];
		setup_render(r, pu, pv, npx);
		setup_xy(r, px, py);

		if ((im1+1) == bsm->bitmapMxN.n_renders[im0]) {
			r->bitmapMxN.im1 = -1; // next iteration: trigger im0++ and im1=0
		}

		return 1;
	}	break;

	case QUERIES: {
		r->p = &r->value[r->queries.qi++];
		if (r->queries.qi < bsm->queries.n) {
			r->x = bsm->queries.coord_pairs[r->queries.qi*2+0];
			r->y = bsm->queries.coord_pairs[r->queries.qi*2+1];
			return 1;
		} else {
			r->bsm = NULL;
			return 0;
		}
	}	break;

	default: assert(!"unhandled case");
	}
}


#define RENDER_PRE(IN_R,OUT_PX,OUT_PY) \
	const struct render* _r = &(IN_R); \
	assert(_r->bsm->mode == BITMAP || _r->bsm->mode == BITMAPMxN); \
	\
	const float _r_nscale = _r->nscale; \
	const float _r_udx0 = _r->udx0; \
	const float _r_udy0 = _r->udy0; \
	const float _r_vdx0 = _r->vdx0; \
	const float _r_vdy0 = _r->vdy0; \
	const float _r_udx1 = _r->udx1; \
	const float _r_udy1 = _r->udy1; \
	const float _r_vdx1 = _r->vdx1; \
	const float _r_vdy1 = _r->vdy1; \
	const int _r_nx = _r->nx; \
	const int _r_ny = _r->ny; \
	const int _r_subnx = _r->subnx; \
	const int _r_subny = _r->subny; \
	uint8_t* _r_p = _r->p; \
	float _rx0 = _r->rx; \
	float _ry0 = _r->ry; \
	\
	for (int _r_yi = 0; _r_yi < _r_ny; _r_yi++) { \
		float _rx1 = _rx0; \
		float _ry1 = _ry0; \
		for (int _r_xi = 0; _r_xi < _r_nx; _r_xi++) { \
			float ACC = 0.0f; \
			float _rx2 = _rx1 + 0.5f*(_r_udx1+_r_vdx1); \
			float _ry2 = _ry1 + 0.5f*(_r_udy1+_r_vdy1); \
			for (int _r_sy = 0; _r_sy < _r_subny; _r_sy++) { \
				float _rx3 = _rx2; \
				float _ry3 = _ry2; \
				for (int _r_sx = 0; _r_sx < _r_subnx; _r_sx++) { \
					float OUT_PX = _rx3; \
					float OUT_PY = _ry3;

//					>>> your render code here <<<

#define RENDER_POST() \
					_rx3 += _r_udx1; \
					_ry3 += _r_udy1; \
				} \
				_rx2 += _r_vdx1; \
				_ry2 += _r_vdy1; \
			} \
			*(_r_p++) = (uint8_t)floorf(fminf(255.0f, ACC * _r_nscale)); \
			_rx1 += _r_udx0; \
			_ry1 += _r_udy0; \
		} \
		_rx0 += _r_vdx0; \
		_ry0 += _r_vdy0; \
	}



#define R0_BEGIN_SD(OUT_PX,OUT_PY) \
	struct render _rr = render_begin(bsm); \
	while (render_next(&_rr)) { \
		float OUT_PX = _rr.x; \
		float OUT_PY = _rr.y; \

#define R0__YIELD_SD_DET(DET) \
		int _r_det = DET; \
		if (bsm->mode == QUERIES) { \
			_rr.value[_rr.queries.qi] = _r_det < 0 ? 255 : 0; \
		} else if (bsm->mode == BITMAP || bsm->mode == BITMAPMxN) { \
			if (bsm->mode == BITMAPMxN) _r_det = 0; \
			if (_r_det > 0) { \
				kernel_outside(_rr.p); \
			} else if (_r_det < 0) { \
				kernel_inside(_rr.p); \
			} else {

#define R0_YIELD_SD(SD) \
			const float _r_sd = SD; \
			R0__YIELD_SD_DET(_r_sd > _rr.distdet ? 1 : _r_sd < -_rr.distdet ? -1 : 0)

#define R0_YIELD_SDSQR(SDSQR) \
			const float _r_sdsqr = SDSQR; \
			R0__YIELD_SD_DET(_r_sdsqr > _rr.distdetsqr ? 1 : _r_sdsqr < -_rr.distdetsqr ? -1 : 0)

#define R1_BEGIN_ACC(OUT_PX,OUT_PY) \
				assert(bsm->mode == BITMAP || bsm->mode == BITMAPMxN); \
				RENDER_PRE(_rr,OUT_PX,OUT_PY)

#define R1_YIELD_ACC(ACCADD) \
				ACC += ACCADD; \
				RENDER_POST() \

#define R_END() \
			} \
		} else {  \
			assert(!"unhandled bsm mode"); \
		} \
	}



// MACRO MADNESS BEWARE! Yes, it's hairy but it allows reusing the signed
// distance functions for point queries without splitting everything into two
// functions. The general bsm_o01_*() structure should be:
//
//    bsm_o01_OPNAME(struct bsm* bsm, ...)
//    {
//      // function-wide definitions here
//      R0_BEGIN_SD(px,py)
//         // signed-distance function definitions here
//      R0_YIELD_SD(signed distance expr) // or: R0_YIELD_SDSQR(signed distance squared expr)
//
//      // subpixel coordinate-independent definitions here
//      R1_BEGIN_ACC(px,py)
//         // subpixel acc code definitions here
//      R1_YIELD_ACC(acc expr)
//	R_END()
//      // end-of-function code here
//    }
//
// If you want switching for R1_BEGIN_ACC()/R1_YIELD_ACC(), see
// bsm_o01_pattern() for inspiration (it has more than one
// R1_BEGIN_ACC/R1_YIELD_ACC scopes)


void bsm_o01_circle(struct bsm* bsm, float radius)
{
	R0_BEGIN_SD(px,py)
	R0_YIELD_SD(sqrtf(px*px + py*py) - radius)

	const float r2 = radius*radius;
	R1_BEGIN_ACC(px,py)
	R1_YIELD_ACC((float)((px*px + py*py) < r2))
	R_END();
}

void bsm_o01_arc(struct bsm* bsm, float aperture_degrees, float radius, float width)
{
	const float a = bsm__deg2rad(aperture_degrees);
	const float s = sinf(a);
	const float c = cosf(a);

	R0_BEGIN_SD(px,py)
		const float x = fabs(px);
		const float y = py;
		float sd = 0.0f;
		if (c*x > s*y) {
			const float sx = x-s*radius;
			const float sy = y-c*radius;
			sd = sqrtf(sx*sx + sy*sy);
		} else {
			sd = fabsf(sqrtf(x*x + y*y) - radius);
		}
		sd -= width;
	R0_YIELD_SD(sd)

	const float ww = width*width;
	R1_BEGIN_ACC(px,py)
		const float x = fabs(px);
		const float y = py;
		float acc;
		if (c*x > s*y) {
			const float sx = x-s*radius;
			const float sy = y-c*radius;
			const float sdsd = sx*sx + sy*sy;
			acc = (float)(ww >= sdsd);
		} else {
			const float sd = fabsf(sqrtf(x*x + y*y) - radius) - width;
			acc = (float)(sd < 0.0f);
		}
	R1_YIELD_ACC(acc)
	R_END();
}

static inline float star_sdf(float px, float py, int n, float outer_radius, float inner_radius)
{
	// SDF by Inigo Quilez, see: https://www.shadertoy.com/view/3tSGDy
	const float an = fpi / (float)n;
	const float m = (float)n + (inner_radius/outer_radius)*(2.0f-(float)n);
	const float en = fpi/m;
	const float racs_x = outer_radius * cosf(an);
	const float racs_y = outer_radius * sinf(an);
	const float ecs_x = cosf(en);
	const float ecs_y = sinf(en);
	px = fabsf(px);
	const float bn = fmodf(atan2f(px, py), 2.0f*an) - an;
	const float plen0 = sqrtf(px*px + py*py);
	px = plen0 * cosf(bn);
	py = plen0 * fabsf(sinf(bn));
	px -= racs_x;
	py -= racs_y;
	const float mul = fmaxf(0.0f, fminf(racs_y / ecs_y, -(px*ecs_x + py*ecs_y)));
	px += ecs_x * mul;
	py += ecs_y * mul;
	const float plen1 = sqrtf(px*px + py*py);
	return plen1 * (px>=0.0f ? 1.0f : -1.0f);
}

struct stargs {
	float an, racs_x, racs_y, ecs_x, ecs_y;
};

static void stargs_init(struct stargs* s, int n, float outer_radius, float inner_radius)
{
	s->an = fpi / (float)n;
	const float m = (float)n + (inner_radius/outer_radius)*(2.0f-(float)n);
	const float en = fpi / m;
	s->racs_x = outer_radius * cosf(s->an);
	s->racs_y = outer_radius * sinf(s->an);
	s->ecs_x = cosf(en);
	s->ecs_y = sinf(en);
}

static inline int star_inside(float px, float py, struct stargs s)
{
	px = fabsf(px);
	const float bn = fmodf(atan2f(px, py), 2.0f*s.an) - s.an;
	const float plen0 = sqrtf(px*px + py*py);
	px = plen0 * cosf(bn)        - s.racs_x;
	py = plen0 * fabsf(sinf(bn)) - s.racs_y;
	px += s.ecs_x * fmaxf(0.0f, fminf(s.racs_y / s.ecs_y, -(px*s.ecs_x + py*s.ecs_y)));
	return px <= 0.0f;
}

void bsm_o01_star(struct bsm* bsm, int n, float outer_radius, float inner_radius)
{
	R0_BEGIN_SD(px,py)
	const float sd = star_sdf(px,py,n,outer_radius,inner_radius);
	R0_YIELD_SD(sd)
	struct stargs stargs;
	stargs_init(&stargs, n, outer_radius,inner_radius);
	R1_BEGIN_ACC(px,py)
	const float acc = (float)star_inside(px,py,stargs);
	R1_YIELD_ACC(acc)
	R_END();
}

void bsm_o01_box(struct bsm* bsm, float w, float h)
{
	R0_BEGIN_SD(px,py)
		const float dx = fabsf(px) - w;
		const float dy = fabsf(py) - h;
		const float ddx = fmaxf(0.0f, dx);
		const float ddy = fmaxf(0.0f, dy);
		const float dc2 = ddx*ddx + ddy*ddy;
		const float de = fminf(0.0f, fmaxf(dx, dy));
		const float sdsqr = dc2 - de*de;
	R0_YIELD_SDSQR(sdsqr)

	R1_BEGIN_ACC(px,py)
	R1_YIELD_ACC((float)(fabsf(px) < w && fabsf(py) < h))
	R_END();
}

void bsm_o01_rounded_box(struct bsm* bsm, float w, float h, float radius)
{
	const float r2 = radius*radius;
	const float wr = w - radius;
	const float hr = h - radius;
	R0_BEGIN_SD(px,py)
		const float dx = fabsf(px) - wr;
		const float dy = fabsf(py) - hr;
		const float ddx = fmaxf(0.0f, dx);
		const float ddy = fmaxf(0.0f, dy);
		const float dc2 = ddx*ddx + ddy*ddy;
		const float de = fminf(0.0f, fmaxf(dx, dy));
		const float sd = sqrtf(dc2) + de - radius;
	R0_YIELD_SD(sd)

	R1_BEGIN_ACC(px,py)
		const float x = fabsf(px);
		const float y = fabsf(py);
		const float acc = (float)
			(
			   (x < wr && y < hr)
			|| (x < w  && y < hr)
			|| (x < wr && y < h )
			|| (x >= wr && y >= hr && ((x-wr)*(x-wr)+(y-hr)*(y-hr)) < r2)
			);
	R1_YIELD_ACC(acc)
	R_END();
}

void bsm_o01_line(struct bsm* bsm, float w)
{
	R0_BEGIN_SD(px,py)
		(void)py;
	R0_YIELD_SD(fabsf(px) - w)
	R1_BEGIN_ACC(px,py)
		(void)py;
	R1_YIELD_ACC((float)(fabsf(px) < w))
	R_END();
}

void bsm_o01_segment(struct bsm* bsm, float x0, float y0, float x1, float y1, float r)
{
	const float bax = x1 - x0;
	const float bay = y1 - y0;
	R0_BEGIN_SD(px,py)
		const float pax = px - x0;
		const float pay = py - y0;
		const float h = fmaxf(0.0f, fminf(1.0f, (pax*bax + pay*bay) / (bax*bax + bay*bay)));
		const float dx = (px-x0) - h*(x1-x0);
		const float dy = (py-y0) - h*(y1-y0);
		const float sd = sqrtf(dx*dx + dy*dy) - r;
	R0_YIELD_SD(sd)
	const float rr = r*r;
	const float bdot = (bax*bax + bay*bay);
	R1_BEGIN_ACC(px,py)
		const float pax = px - x0;
		const float pay = py - y0;
		const float h = fmaxf(0.0f, fminf(1.0f, (pax*bax + pay*bay) / bdot));
		const float dx = (px-x0) - h*bax;
		const float dy = (py-y0) - h*bay;
		const float dd = dx*dx + dy*dy;
		const float acc = dd < rr ? 1.0f : 0.0f;
	R1_YIELD_ACC(acc)
	R_END();
}

void bsm_o01_isosceles_triangle(struct bsm* bsm, float w, float h)
{
	// SDF by Inigo Quilez, see: https://www.shadertoy.com/view/MldcD7
	const float dd = 1.0f / (w*w + h*h);
	const float wi = 1.0f / w;
	R0_BEGIN_SD(px,py)
		px = fabsf(px);
		const float c0 = fmaxf(0.0f, fminf(1.0f, (px*w+py*h)*dd));
		const float ax = px - w*c0;
		const float ay = py - h*c0;
		const float bx = px - w * fmaxf(0.0f, fminf(1.0f, px*wi));
		const float by = py - h;
		const float d = fminf(ax*ax+ay*ay, bx*bx+by*by);
		const float s = fmaxf(px*h - py*w, py-h);
		const float sd = sqrtf(d) * (s>0.0f?1.0f:-1.0f);
	R0_YIELD_SD(sd)
	R1_BEGIN_ACC(px,py)
		const float acc = fmaxf(fabsf(px)*h - py*w, py-h) <= 0.0f;
	R1_YIELD_ACC(acc)
	R_END();
}

void bsm_o01_isosceles_trapezoid(struct bsm* bsm, float r1, float r2, float h)
{
	// SDF by Inigo Quilez, see: https://www.shadertoy.com/view/MlycD3
	const float k1x = r2;
	const float k1y = h;
	const float r2r1 = r2-r1;
	const float k2x = r2r1;
	const float k2y = 2.0f*h;
	const float id2k2 = 1.0f / (k2x*k2x + k2y*k2y);
	R0_BEGIN_SD(px,py)
		px = fabsf(px);
		const float cax = fmaxf(0.0f, px-((py<0.0f)?r1:r2));
		const float cay = fabsf(py)-h;
		const float dd = ((k1x-px)*k2x + (k1y-py)*k2y) * id2k2;

		const float cbs = fmaxf(0.0f, fminf(1.0f, dd));
		const float cbx = px - k1x + (r2r1)*cbs;
		const float cby = py - k1y + (2.0f*h)*cbs;
		const float s = (cbx < 0.0f && cay < 0.0f) ? -1.0f : 1.0f;
		const float sd = s * sqrtf(fminf(cax*cax + cay*cay, cbx*cbx + cby*cby));
	R0_YIELD_SD(sd)
	R1_BEGIN_ACC(px,py)
		px = fabsf(px);
		const float cay = fabsf(py)-h;
		const float dd = ((k1x-px)*k2x + (k1y-py)*k2y) * id2k2;

		const float cbs = fmaxf(0.0f, fminf(1.0f, dd));
		const float cbx = px - k1x + (r2r1)*cbs;
		const float acc = (float)(cbx < 0.0f && cay < 0.0f);
	R1_YIELD_ACC(acc)
	R_END();
}

void bsm_o01_split(struct bsm* bsm)
{
	R0_BEGIN_SD(px,py)
	(void)py;
	R0_YIELD_SD(px)
	R1_BEGIN_ACC(px,py)
		(void)py;
	R1_YIELD_ACC((float)(px <= 0.0f))
	R_END();
}

static inline float pat1(float x)
{
	#if 1
	return x - floorf(x); // 4x faster?
	#else
	x = fmodf(x, 1.0f);
	if (x < 0) x = 1.0f + x;
	return x;
	#endif
}

static inline float patw(float x, float w, float wi)
{
	return pat1(x*wi)*w;
}

void bsm_o01_pattern(struct bsm* bsm, float* ws)
{
	float* p = ws;
	int n = 0;
	float all = 0.0f;
	while (*p > 0) { all+=*p ; p++; n++; }
	assert(((n&1) == 0) && "ws count must be even");
	assert((n >= 2) && "empty pattern");
	assert(all > 0.0f);
	const float allinv = 1.0f / all;

	R0_BEGIN_SD(px,py)
		(void)py;
		float x = patw(px, all, allinv);
		float sd = 0;
		for (int i = 0; i < n; i++) {
			const float w = ws[i];
			if (x < w) {
				const float w2 = w*0.5f;
				sd = (w2-fabsf(x-w2)) * ((i&1)==0 ? -1.0f : 1.0f);
				break;
			}
			x -= w;
		}
	R0_YIELD_SD(sd)

	if (n == 2) {
		const float split = ws[0];
		R1_BEGIN_ACC(px,py)
		(void)py;
		R1_YIELD_ACC(patw(px, all, allinv) < split)
	} else {
		R1_BEGIN_ACC(px,py)
		(void)py;
		float x = patw(px, all, allinv);
		float acc = 0.0f;
		for (int i = 0; i < n; i++) {
			const float w = ws[i];
			if (x < w) {
				if ((i&1) == 0) acc = 1.0f;
				break;
			}
			x -= w;
		}
		R1_YIELD_ACC(acc)
	}

	R_END();
}

#define O21_EXPR(BSM,EXPR) \
	struct bsm* _o_bsm = BSM; \
	const int _o_ref_b = pop_ref(_o_bsm); \
	const int _o_ref_a = pop_ref(_o_bsm); \
	const int _o_ref_c = new_ref(_o_bsm); \
	const uint8_t* _o_pb = deref_value(bsm, _o_ref_b); \
	const uint8_t* _o_pa = deref_value(bsm, _o_ref_a); \
	uint8_t*       _o_pc = deref_value(bsm, _o_ref_c); \
	const int _o_n = _o_bsm->value_size; \
	for (int i = 0; i < _o_n; i++) { \
		const uint8_t a = *(_o_pa++); \
		const uint8_t b = *(_o_pb++); \
		*(_o_pc++) = EXPR; \
	} \
	collect_garbage(_o_bsm);

void bsm_o21_union(struct bsm* bsm)
{
	O21_EXPR(bsm, a>b?a:b)
}

void bsm_o21_add(struct bsm* bsm)
{
	O21_EXPR(bsm, (a+b)>255?255:a+b);
}

void bsm_o21_intersection(struct bsm* bsm)
{
	O21_EXPR(bsm, a<b?a:b)
}

void bsm_o21_difference(struct bsm* bsm)
{
	O21_EXPR(bsm, a<(255-b)?a:(255-b))
}

int bsm_get_stack_height(struct bsm* bsm) { return bsm->index_stack_height; }

int bsm_get_width(struct bsm* bsm) { assert(bsm->mode == BITMAP); return bsm->bitmap.width; }
int bsm_get_height(struct bsm* bsm) { assert(bsm->mode == BITMAP); return bsm->bitmap.height; }
void bsm_set_supersampling(struct bsm* bsm, int n) { bsm->supersampling = n; }

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if defined(BSM_DOODLE)

// see BSM_DOODLE in bsm.h

#ifndef BSM_DOODLE_WIDTH
#define BSM_DOODLE_WIDTH (256)
#endif

#ifndef BSM_DOODLE_HEIGHT
#define BSM_DOODLE_HEIGHT (256)
#endif

#ifndef BSM_DOODLE_PIXELS_PER_UNIT
#define BSM_DOODLE_PIXELS_PER_UNIT ((BSM_DOODLE_WIDTH)>>1)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include  "stb_image_write.h"

static inline double bsm_doodle_get_time(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return 1e-9*(double)ts.tv_nsec + (double)ts.tv_sec;
}

void BSM_DOODLE(struct bsm*); // NOTE: BSM_DOODLE is a #define

#define BSM_DOODLE_STR0(x) #x
#define BSM_DOODLE_STR1(x) BSM_DOODLE_STR0(x)

int main(int argc, char** argv)
{
	const double t0 = bsm_doodle_get_time();
	struct bsm* bsm = bsm_new();
	bsm_begin_bitmap(bsm, BSM_DOODLE_PIXELS_PER_UNIT, BSM_DOODLE_WIDTH, BSM_DOODLE_HEIGHT);
	BSM_DOODLE(bsm);
	const int n = bsm_get_stack_height(bsm);
	const int width = bsm_get_width(bsm);
	const int height = bsm_get_height(bsm);
	const int height_total = height * n;
	uint8_t* bitmap = malloc(width*height_total);
	for (int i = 0; i < n; i++) {
		bsm_blit_bitmap(bsm, i, bitmap + width*height*i, width);
	}
	const double dt = bsm_doodle_get_time() - t0;
	const char* path = "_bsm_doodle_" BSM_DOODLE_STR1(BSM_DOODLE) ".png";
	assert(stbi_write_png(path, width, height_total, 1, bitmap, width));
	printf("Render took %.4f seconds; wrote %s\n", dt, path);
	return EXIT_SUCCESS;
}

#endif // BSM_DOODLE

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef UNIT_TEST
//   cc -DUNIT_TEST -Wall -o test_bsm bsm.c -lm && ./test_bsm
// or
//   cc -DUNIT_TEST=<testname> -Wall -o test_bsm bsm.c -lm && ./test_bsm

#include <stdio.h>
#include <time.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static int FAIL;
static struct bsm* bsm;

static inline double tim(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return 1e-9*(double)ts.tv_nsec + (double)ts.tv_sec;
}

#define _STR(x) #x
#define STR(x) _STR(x)
static const char* test_filter_string = STR(UNIT_TEST); // is "1" if UNIT_TEST is defined, but without a specific value
static inline int include_test(const char* name)
{
	if (strcmp(test_filter_string, "1") == 0) {
		return 1;
	} else {
		return strcmp(test_filter_string, name) == 0;
	}
}

static void test_bitmaps(void)
{
	int is_inside_test = 0;
	int TI = -1;
	#define TEST(NAME) \
	{ \
		const char* test_name = #NAME; \
		assert(!is_inside_test); \
		is_inside_test = 1; \
		if (include_test(#NAME)) { \
			const double t0 = tim();

	#define ENDTEST \
			const double dt = tim() - t0; \
			char path[1<<12]; \
			if (TI >= 0) { \
				snprintf(path, sizeof path, "_test_bsm_%s_%.2d.png", test_name, TI); \
			} else { \
				snprintf(path, sizeof path, "_test_bsm_%s.png", test_name); \
			} \
			const int width = bsm_get_width(bsm); \
			const int height = bsm_get_height(bsm); \
			printf("TEST(%s) took %.4f seconds; writing %s (%dx%d) ...\n", test_name, dt, path, width, height); \
			uint8_t* data = malloc(width * height); \
			assert((bsm_get_stack_height(bsm) == 1) && "expected one bitmap on stack"); \
			bsm_end_bitmap(bsm, data, width); \
			assert(stbi_write_png(path, width, height, 1, data, width)); \
			free(data); \
		} \
		assert(is_inside_test); \
		is_inside_test = 0; \
	}

	TEST(rings) {
		bsm_begin_bitmap(bsm, 256, 512, 512);
		const int n = 20;
		const float d = 1.0f / (float)(2*n);
		for (int i = 0; i < n; i++) {
			bsm_o01_circle(bsm, 1.0f - (float)(i*2)*d);
			bsm_o01_circle(bsm, 1.0f - (float)(i*2+1)*d);
			bsm_o21_difference(bsm);
			if (i > 0) bsm_o21_union(bsm);
		}
	} ENDTEST

	TEST(transforms) {
		bsm_begin_bitmap(bsm, 300, 512, 512);
		bsm_tx_save(bsm);
		bsm_tx_rotate(bsm, 7);
		bsm_o01_box(bsm, 0.4f, 0.8f);
		bsm_tx_restore(bsm);
		const int n = 32;
		for (int i = 0; i < n; i++) {
			bsm_tx_save(bsm);
			bsm_tx_rotate(bsm, (360.0f*i)/(float)n);
			bsm_tx_translate(bsm, 0.5f, 0.0f);
			//bsm_tx_translate(bsm, -0.5f, 0.0f);
			bsm_o01_circle(bsm, 0.03);
			bsm_o21_difference(bsm);
			bsm_tx_restore(bsm);
		}
	} ENDTEST

	TEST(tx2) {
		bsm_begin_bitmap(bsm, 512, 512, 256);

		bsm_o01_circle(bsm, 0.01); // (0,0)-dot for reference

		// expecting r=0.05 circle in (0.5, 0)
		bsm_tx_save(bsm);
		bsm_tx_translate(bsm, 0.5f, 0.0f);
		bsm_tx_scale(bsm, 0.5f);
		bsm_o01_circle(bsm, 0.1);
		bsm_tx_restore(bsm);
		bsm_o21_union(bsm);

		// expecting r=0.05 circle in (-0.25, -0.25)
		bsm_tx_save(bsm);
		bsm_tx_scale(bsm, 0.5f);
		bsm_tx_translate(bsm, -0.5f, -0.5f);
		bsm_o01_circle(bsm, 0.1);
		bsm_tx_restore(bsm);
		bsm_o21_union(bsm);
	} ENDTEST

	TEST(scaling1) {
		bsm_begin_bitmap(bsm, 128, 256, 256);
		const float F = 4.0f;
		bsm_tx_scale(bsm, F);
		bsm_o01_circle(bsm, 1.0f / F);
	} ENDTEST

	TEST(scaling2) {
		bsm_begin_bitmap(bsm, 128, 256, 256);
		const float F = 4.0f;
		bsm_tx_scale(bsm, 1.0f / F);
		bsm_o01_circle(bsm, F);
	} ENDTEST

	for (TI = 0; TI < 16; TI++) {
		TEST(edgecase) {
			const int W=48+TI;
			bsm_begin_bitmap(bsm, (float)W/2.0f, W, W);
			const float d = 1.0f / (float)W;
			Circle(1.0+d*2);
			Circle(1.0);
			Difference();
		} ENDTEST
	}
	TI=-1;

	TEST(shapes) {
		bsm_begin_bitmap(bsm, 256, 512, 512);
		int i = 0;
		const int N = 4;
		const float NI = 1.0f / (float)N;
		for (int row = 0; row < N; row++) {
			for (int col = 0; col < N; col++) {
				Save();
				const float A = 10.0;
				Scale(NI);
				Translate(
					((float)col - (float)N*0.5f + 0.5f) * 2.0f,
					((float)row - (float)N*0.5f + 0.5f) * 2.0f
				);
				Rotate(A);
				int ii = 0;
				if (0) {
				} else if ((ii++)==i) {
					Circle(0.8);
				} else if ((ii++)==i) {
					Arc(150.0, 0.8, 0.1);
				} else if ((ii++)==i) {
					Box(0.4, 0.2);
				} else if ((ii++)==i) {
					RoundedBox(0.4, 0.2, 0.1);
				} else if ((ii++)==i) {
					Line(0.1);
					Circle(0.8);
					Intersection();
				} else if ((ii++)==i) {
					Split();
					Circle(0.8);
					Intersection();
				} else if ((ii++)==i) {
					Pattern(0.02, 0.03, 0.07, 0.1);
					Circle(0.8);
					Intersection();
				} else if ((ii++)==i) {
					Pattern(0.02, 0.03); // NOTE: n=2 is an internal special case
					Circle(0.8);
					Intersection();
				} else if ((ii++)==i) {
					Star(16, 0.8, 0.7);
				} else if ((ii++)==i) {
					Star(12, 0.9, 0.1);
				} else if ((ii++)==i) {
					Star(40, 0.9, 0.2);
				} else if ((ii++)==i) {
					Star(7, 0.8, 0.5);
				} else if ((ii++)==i) {
					Segment(-0.8, -0.2, 0.8, 0.2, 0.1);
					Segment(-0.8, 0.4, 0.4, -0.5, 0.05);
					Union();
				} else if ((ii++)==i) {
					Circle(1.0);
					IsoscelesTriangle(0.2, 0.9);
					Difference();
				} else if ((ii++)==i) {
					Circle(1.0);
					IsoscelesTrapezoid(0.1, 0.5, 0.7);
					Difference();
				}
				i++;
				Restore();
				while (bsm->index_stack_height >= 2) Union();
			}
		}
	} ENDTEST

	// seams_visible vs seams_invisible: exactly the same test except for
	// the join-operation: Union() (max(a,b)) vs Add() (a+b, saturated).
	// Union() is probably better when shapes are overlapping, while Add()
	// is better when shapes are joined.

	TEST(seams_visible) {
		bsm_begin_bitmap(bsm, 128, 256, 256);
		int n = 0;
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				Save();
				Rotate(11.1f);
				Scale(0.2f);
				Translate(-3.0f + (float)x*2.0f, -3.0f + (float)y*2.0f);
				Box(1.0, 1.0);
				Restore();
				n++;
			}
		}
		for (int i = 0; i < (n-1); i++) Union();
	} ENDTEST

	TEST(seams_invisible) {
		bsm_begin_bitmap(bsm, 128, 256, 256);
		int n = 0;
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				Save();
				Rotate(11.1f);
				Scale(0.2f);
				Translate(-3.0f + (float)x*2.0f, -3.0f + (float)y*2.0f);
				Box(1.0, 1.0);
				Restore();
				n++;
			}
		}
		for (int i = 0; i < (n-1); i++) Add();
	} ENDTEST

	// regression test: non-block aligned dimensions caused corrupt images
	TEST(odddim) {
		const int X = 198;
		bsm_begin_bitmap(bsm, 100, X, X);

		Save();
		Translate(0.2, 0.3);
		Circle(0.8);
		Circle(0.7);
		Difference();
		Restore();

		Save();
		Translate(-0.4, -0.1);
		Circle(0.4);
		Circle(0.3);
		Difference();
		Restore();

		Union();

	} ENDTEST

	#undef ENDTEST
	#undef TEST
}

static void test_bitmaps3x3(void)
{
	int TI=-1;
	int is_inside_test = 0;
	#define TEST3(NAME,W,H,PPU) \
	{ \
		const char* test_name = #NAME; \
		assert(!is_inside_test); \
		is_inside_test = 1; \
		if (include_test(#NAME)) { \
			const double t0 = tim(); \
			const int base_width = W; \
			const int base_height = H; \
			const float pixels_per_unit = PPU; \
			const int widths[]  = {base_width,  -1, base_width,  0}; \
			const int heights[] = {base_height, -1, base_height, 0}; \
			bsm_begin_bitmapMxN(bsm, pixels_per_unit, widths, heights);

	#define END3TEST \
			const double dt = tim() - t0; \
			char path[1<<12]; \
			if (TI >= 0) { \
				snprintf(path, sizeof path, "_test_bsm_3x3_%s_%.2d.png", test_name, TI); \
			} else { \
				snprintf(path, sizeof path, "_test_bsm_3x3_%s.png", test_name); \
			} \
			const int width = 2*base_width+1; \
			const int height = 2*base_height+1; \
			printf("TEST3(%s) took %.4f seconds; writing %s (%dx%d) ...\n", test_name, dt, path, width, height); \
			uint8_t* data = malloc(width * height); \
			assert((bsm_get_stack_height(bsm) == 1) && "expected one bitmap on stack"); \
			bsm_end_bitmapMxN(bsm, data, width); \
			assert(stbi_write_png(path, width, height, 1, data, width)); \
			free(data); \
		} \
		assert(is_inside_test); \
		is_inside_test = 0; \
	}

	TEST3(frame31,32,32,32) {
		Circle(1.0f);
	} END3TEST

	TEST3(frame30,32,32,32) {
		Circle(1.0f);
		Circle(0.9f);
		Difference();
	} END3TEST

	for (TI = 0; TI < 16; TI++) {
		const int W = 24+TI;
		TEST3(edgecase,W,W,W) {
			const float d = 1.0f / (float)W;
			Circle(1.0+d*2);
			Circle(1.0);
			Difference();
		} END3TEST
	}
	TI=-1;
}

static void test_queries(void)
{
	int is_inside_test = 0;
	int n_tests_executed = 0;
	int n_tests_failed = 0;

	#define QTEST(NAME, ...) \
	{ \
		const char* test_name = #NAME; \
		assert(!is_inside_test); \
		is_inside_test = 1; \
		float args[] = {__VA_ARGS__}; \
		static_assert(((sizeof(args)/sizeof(args[0])) % 3) == 0, "bad test args"); \
		if (include_test(#NAME)) { \
			n_tests_executed++; \
			const int n = (sizeof(args)/sizeof(args[0])) / 3; \
			float coord_pairs[1<<10]; \
			for (int i = 0; i < n; i++) { \
				coord_pairs[i*2+0] = args[i*3+0]; \
				coord_pairs[i*2+1] = args[i*3+1]; \
			} \
			bsm_begin_queries(bsm, n, coord_pairs); \

	#define ENDQTEST() \
			int result[256]; \
			bsm_end_queries(bsm, n, result); \
			int test_failed = 0; \
			for (int pass = 0; pass < 2; pass++) { \
				int n_queries_failed = 0; \
				for (int i = 0; i < n; i++) { \
					const int expected = args[i*3+2] != 0.0f; \
					const int actual = result[i] != 0; \
					if (actual != expected) { \
						const char* ns[] = {"OUTSIDE","INSIDE"}; \
						if (pass == 1) { \
							printf("   (%.2f,%.2f) was expected to be %s, but was %s\n", \
								coord_pairs[i*2+0], \
								coord_pairs[i*2+1], \
								ns[expected], \
								ns[actual]); \
						} \
						n_queries_failed++; \
						test_failed = 1; \
					} \
				} \
				if (pass == 0) printf("QTEST(%s): %s\n", test_name, n_queries_failed ? "FAIL" : "OK"); \
			} \
			if (test_failed) n_tests_failed++; \
		} \
		assert(is_inside_test); \
		is_inside_test = 0; \
	}

	QTEST(circle,  0,0,1,  0.5,0,1,  2,0,0,  0,2,0,  -2,0,0,  0,-2,0,  ) {
		Circle(1.0f);
	} ENDQTEST()

	QTEST(boolean,  0,0,0,  0.1,0,0,  0,0.1,0,   0.5,0,1,  0,0.5,1,  2,0,0,  0,2,0,  -2,0,0,  0,-2,0,  ) {
		Circle(1.0f);
		Circle(0.3f);
		Difference();
	} ENDQTEST()

	QTEST(arc,  0,0,0,  1,0,1,   -1,0,1,  0,1,1,  0,-1,0,  ) {
		Arc(150.0, 1, 0.1);
	} ENDQTEST()

	QTEST(split,  -1,0,1,  1,0,0,  ) {
		Split();
	} ENDQTEST()

	#undef ENDQTEST
	#undef QTEST

	if (n_tests_executed) {
		if (n_tests_failed == 0) {
			printf("Queries tests: OK\n");
		} else {
			printf("Queries tests: FAIL (n_tests_failed=%d)\n", n_tests_failed);
			FAIL=1;
		}
	}
}

static inline void dumptx(struct tx* tx) { printf("b=[%.3f,%.3f] o=[%.3f,%.3f]\n", tx->basis_x, tx->basis_y, tx->origin_x, tx->origin_y); }

static void test_tx(void)
{
	{
		struct tx tx = {.basis_x = 1, .origin_x = 1, .origin_y = 2};
		tx_invert(&tx);
		assert(tx.basis_x == 1.0f);
		assert(tx.basis_y == 0.0f);
		assert(tx.origin_x == -1.0f);
		assert(tx.origin_y == -2.0f);
	}

	{
		struct tx tx = {.basis_x = 2.0f};
		tx_invert(&tx);
		assert(tx.basis_x == 0.5f);
		assert(tx.basis_y == 0.0f);
		assert(tx.origin_x == 0.0f);
		assert(tx.origin_y == 0.0f);
	}

	{
		const float ph = 0.2f;
		const float m = 1.5f;
		const float c = cosf(ph)*m;
		const float s = sinf(ph)*m;
		const float ox = 0.6f;
		const float oy = -0.33f;
		struct tx tx = {.basis_x = c, .basis_y = s, .origin_x = ox, .origin_y = oy };
		//dumptx(&tx);
		tx_invert(&tx);
		//dumptx(&tx);
		tx_invert(&tx);
		//dumptx(&tx);
		assert(fabsf(tx.basis_x - c) < 1e-5);
		assert(fabsf(tx.basis_y - s) < 1e-5);
		assert(fabsf(tx.origin_x - ox) < 1e-5);
		assert(fabsf(tx.origin_y - oy) < 1e-5);
	}

	printf("TX tests: OK\n");
}

int main(int argc, char** argv)
{
	bsm = bsm_new();
	test_bitmaps3x3();
	test_bitmaps();
	test_queries();
	test_tx();
	return FAIL ? EXIT_FAILURE : EXIT_SUCCESS;
}

#endif // UNIT_TEST
