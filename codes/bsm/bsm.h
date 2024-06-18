#ifndef BSM_H // Bitmap Stack Machine


// Example that renders a ring:
//    struct bsm* bsm = bsm_new();
//    bsm_begin_bitmap(bsm, 128, 256, 256); // 256x256 image, 128 pixels per unit
//      bsm_o01_circle(bsm, 1.0f); // Pushes a circle on the bitmap stack with radius=1
//      bsm_o01_circle(bsm, 0.89f); // Pushes a circle on the bitmap stack with radius=0.9
//      bsm_o21_difference(bsm);   // Subtracts the smaller circle from the bigger, or: in Forth notation: a b -- difference(a,b)
//    bsm_end_bitmap(bsm, output_bitmap, output_bitmap_width); // Store result in output
// Using the short-hand API, the draw calls could've been written as:
//      Circle(1.0f);
//      Circle(0.89f);
//      Difference();


// There's a useful BSM_DOODLE feature for visualizing the BSM stack:
//    $  cc -DBSM_DOODLE=my_first_drawing bsm.c drawings.c -o drawing_doodle -lm && ./drawing_doodle
// (requires stb_image_write.h).
// If drawings.c contains the function:
//    void my_first_drawing(struct bsm*)
// Then the program (`drawing_doodle`) renders the BSM stack to:
//    _bsm_doodle_my_first_drawing.png
// When combined with e.g. watch scripts and a reloading image viewer (like
// feh) you can make it so that you get immediate visual feedback when you
// write drawings.c.
// See BSM_DOODLE in bsm.c for details (it's not a lot of code if you need to
// hack it); when BSM_DOODLE is defined, then bsm.c defines main() (so you
// can't use your "production bsm.o").
// You can also define BSM_DOODLE_WIDTH, BSM_DOODLE_HEIGHT and
// BSM_DOODLE_PIXELS_PER_UNIT which are passed as-is to bsm_begin()
// Compilation might fail if drawings.c contains public symbols that refer to
// other public symbols that aren't compiled into your doodle executable. One
// solution is to add the required compilation units; another is to wrap the
// offending code in `#ifndef BSM_DOODLE`
// See also the bsm_doodle.py.

#ifdef BSM_DOODLE
#define BSM_STATIC        // make function public when building a BSM_DOODLE executable (so bsm.c can find it)
#else
#define BSM_STATIC static // make function static under normal circumstances
#endif


#include <stdint.h>

#define BSM_DEFAULT_SUPERSAMPLING (8)

struct bsm;

struct bsm* bsm_new(void);
void bsm_free(struct bsm*);

void bsm_begin_bitmap(struct bsm*, float pixels_per_unit, int width, int height);
void bsm_end_bitmap(struct bsm*, uint8_t* out, int stride);
// Renders a width x height bitmap for the shape inside the begin...end scope.
// Output is monochrome / 1-channel. `stride` is the width of the destination
// image (=number of bytes between consequtive rows).


void bsm_begin_bitmapMxN(struct bsm*, float pixels_per_unit, const int* widths, const int* heights);
void bsm_end_bitmapMxN(struct bsm*, uint8_t* out, int stride);
// Renders a M x N grid. `widths` and `heights` are arrays defining a "size
// matrix". Both arrays are 0-terminated, and negative values means "stretch".
// Stretch columns/rows are rendered 1 pixel and 0 units wide/high.
// Example: widths  = [10, -1, 20, 0] (or: [10, "stretch", 20, "end"])
//          heights = [15, -1, 25, 0] (or: [15, "stretch", 25, "end"])
//          pixels_per_units = 10
//    +---+-+---+
//    |a  |b|  c|
//    |   | |   |
//    |   | |   |
//    +---+-+---+
//    |d  |e|  f|
//    +---+-+---+
//    |   | |   |
//    |   | |   |
//    |g  |h|  i|
//    +---+-+---+
//     pixel pos    pixel dim     units pos    units dim
// a     0 , 0       10 x 15    -1.5 , -2.0    1.0 x 1.5
// b    10 , 0        1 x 15     0.0 , -2.0      0 x 1.5
// c    11 , 0       20 x 15     0.0 , -2.0    2.0 x 1.5
// d     0 , 15      10 x 1     -1.5 ,  0.0    1.0 x 0
// e    10 , 15       1 x 1      0.0 ,  0.0      0 x 0
// f    11 , 15      20 x 1      0.0 ,  0.0    2.0 x 0
// g     0 , 16      10 x 25    -1.5 ,  0.0    1.0 x 2.5
// h    10 , 16       1 x 25     0.0 ,  0.0      0 x 2.5
// i    11 , 16      20 x 25     0.0 ,  0.0    2.0 x 2.5
// Total pixel width   = 10+1+20 = 31     uni.0+2.0 = 3.0t width  = 1
// Total pixel heights = 15+1+25 = 41     unit height = 1.5+2.5 = 4.0


void bsm_begin_queries(struct bsm*, int n, float* coord_pairs);
void bsm_end_queries(struct bsm*, int n, int* inside);
// Evaluate point queries for the shape inside the begin...end scope.
// `coord_pairs` must contain 2*n elements (or: n [x,y] pairs). `inside` is
// filled with `n` "booleans"; 1 if inside; 0 if not. The `n` passed to "end"
// can be lower than the one passed to "begin", but not higher. The
// bsm_begin_query()/bsm_end_query() helper functions are convenient if you
// only need to evaluate a single point.

static inline void bsm_begin_query(struct bsm* bsm, float x, float y)
{
	float pairs[] = {x,y};
	bsm_begin_queries(bsm, 1, pairs);
}

static inline int bsm_end_query(struct bsm* bsm)
{
	int inside = 0;
	bsm_end_queries(bsm, 1, &inside);
	return inside;
}


void bsm_pick(struct bsm*, int stack_index);
// Pushes the element referenced `stack_index` on stack. Negative indices are
// indexed from top.
static inline void bsm_dup(struct bsm* bsm) { bsm_pick(bsm, -1); } // a -- a a
void bsm_swap(struct bsm*); // a b -- b a
void bsm_drop(struct bsm*); // a b -- a

int bsm_get_stack_height(struct bsm*);

int bsm_get_width(struct bsm*); // XXX only bitmap... doesn't work for bitmap3x3... do I need another interface?
int bsm_get_height(struct bsm*);


void bsm_set_supersampling(struct bsm*, int n);
// Sets supersampling factor; subsequent bsm_o01_*() calls will render n x n
// subpixels per pixel. If n<=0 then BSM_DEFAULT_SUPERSAMPLING is used instead.

// Coordinate system transforms:
void bsm_tx_save(struct bsm*);
void bsm_tx_restore(struct bsm*);
void bsm_tx_translate(struct bsm*, float x, float y);
void bsm_tx_rotate(struct bsm*, float degrees);
void bsm_tx_scale(struct bsm*, float scalar);

// Naming conventions for Stack operations: bsm_oXY_OP() performs OP where X is
// the number of values popped from the stack, and Y is the number pushed.

// Draw shapes: -- shape
void bsm_o01_circle(struct bsm*, float radius);
void bsm_o01_arc(struct bsm* bsm, float aperture_degrees, float radius, float width);
void bsm_o01_star(struct bsm*, int n, float outer_radius, float inner_radius);
void bsm_o01_box(struct bsm*, float width, float height);
void bsm_o01_rounded_box(struct bsm*, float width, float height, float radius);
void bsm_o01_line(struct bsm*, float width);
void bsm_o01_segment(struct bsm*, float x0, float y0, float x1, float y1, float r);
void bsm_o01_isosceles_triangle(struct bsm*, float w, float h);
void bsm_o01_isosceles_trapezoid(struct bsm*, float r1, float r2, float h);
void bsm_o01_split(struct bsm*);
void bsm_o01_pattern(struct bsm*, float* ws);

// Unary operations: a -- OP(a)
void bsm_o11_scale(struct bsm*, float scalar); // a -- a*scalar
void bsm_o11_invert(struct bsm*);// a -- 1-a

// Binary operations: a b -- OP(a,b)
void bsm_o21_union(struct bsm*);        // a b -- max(a,b)
void bsm_o21_add(struct bsm*);          // a b -- a+b (saturated)
void bsm_o21_intersection(struct bsm*); // a b -- min(a,b)
void bsm_o21_difference(struct bsm*);   // a b -- min(a,1-b)
//void bsm_o21_multiply(struct bsm*);   // a b -- a*b

#ifndef BSM_NO_SHORT_NAMES

  #ifndef BSM_INSTANCE
  #define BSM_INSTANCE (bsm)
  #endif

#define Dup()                        bsm_dup(BSM_INSTANCE)
#define Pick(i)                      bsm_pick(BSM_INSTANCE,i)
#define Swap()                       bsm_swap(BSM_INSTANCE)
#define Drop()                       bsm_drop(BSM_INSTANCE)
#define Save()                       bsm_tx_save(BSM_INSTANCE)
#define Restore()                    bsm_tx_restore(BSM_INSTANCE)
#define Translate(x,y)               bsm_tx_translate(BSM_INSTANCE,x,y)
#define Rotate(degrees)              bsm_tx_rotate(BSM_INSTANCE,degrees)
#define Scale(scalar)                bsm_tx_scale(BSM_INSTANCE,scalar)
#define Circle(r)                    bsm_o01_circle(BSM_INSTANCE,r)
#define Arc(a,r,w)                   bsm_o01_arc(BSM_INSTANCE,a,r,w)
#define Star(n,o,i)                  bsm_o01_star(BSM_INSTANCE,n,o,i)
#define Box(w,h)                     bsm_o01_box(BSM_INSTANCE,w,h)
#define RoundedBox(w,h,r)            bsm_o01_rounded_box(BSM_INSTANCE,w,h,r)
#define Line(w)                      bsm_o01_line(BSM_INSTANCE,w)
#define Segment(x0,y0,x1,y1,r)       bsm_o01_segment(BSM_INSTANCE,x0,y0,x1,y1,r)
#define IsoscelesTriangle(w,h)       bsm_o01_isosceles_triangle(BSM_INSTANCE,w,h)
#define IsoscelesTrapezoid(r1,r2,h)  bsm_o01_isosceles_trapezoid(BSM_INSTANCE,r1,r2,h)
#define Split(w)                     bsm_o01_split(BSM_INSTANCE)
#define Pattern(...)                 bsm_o01_pattern(BSM_INSTANCE,(float[]) { __VA_ARGS__, 0 })
#define Union()                      bsm_o21_union(BSM_INSTANCE)
#define Add()                        bsm_o21_add(BSM_INSTANCE)
#define Intersection()               bsm_o21_intersection(BSM_INSTANCE)
#define Difference()                 bsm_o21_difference(BSM_INSTANCE)

#endif

#define BSM_H
#endif
