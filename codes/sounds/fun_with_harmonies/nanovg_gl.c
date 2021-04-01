#include "gl.h"
#include "nanovg.h"

#define NANOVG_GL2_IMPLEMENTATION
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

NVGcontext* nanovg_create_context()
{
	const int flags = NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG;
	return nvgCreateGL2(flags);
}
