
#include "tjsCommHead.h"
#include "tvpgl.h"
#include "LayerBitmapIntf.h"
#include "LayerBitmapUtility2.h"
#include "LayerBitmapBlit.h"
#include "LayerBitmapCopyRect.h"

static float sBmFactor[] =
{
  59, // bmCopy,
  59, // bmCopyOnAlpha,
  52, // bmAlpha,
  52, // bmAlphaOnAlpha,
  61, // bmAdd,
  59, // bmSub,
  45, // bmMul,
  10, // bmDodge,
  58, // bmDarken,
  56, // bmLighten,
  42, // bmScreen,
  52, // bmAddAlpha,
  52, // bmAddAlphaOnAddAlpha,
  52, // bmAddAlphaOnAlpha,
  52, // bmAlphaOnAddAlpha,
  52, // bmCopyOnAddAlpha,
  32, // bmPsNormal,
  30, // bmPsAdditive,
  29, // bmPsSubtractive,
  27, // bmPsMultiplicative,
  27, // bmPsScreen,
  15, // bmPsOverlay,
  15, // bmPsHardLight,
  10, // bmPsSoftLight,
  10, // bmPsColorDodge,
  10, // bmPsColorDodge5,
  10, // bmPsColorBurn,
  29, // bmPsLighten,
  29, // bmPsDarken,
  29, // bmPsDifference,
  26, // bmPsDifference5,
  66, // bmPsExclusion
};

struct PartialBltParam {
  tTVPBaseBitmap *self;
  tjs_uint8 *dest;
  tjs_int dpitch;
  tjs_int dx;
  tjs_int dy;
  tjs_int w;
  tjs_int h;
  const tjs_int8 *src;
  tjs_int spitch;
  tjs_int sx;
  tjs_int sy;
  tTVPBBBltMethod method;
  tjs_int opa;
  bool hda;
};

void PartialBlt(const PartialBltParam *param)
{
	tjs_uint8 *dest;
	const tjs_uint8 *src;
	tjs_int dpitch, dx, dy, w, h, spitch, sx, sy;
	tTVPBBBltMethod method;
	tjs_int opa;
	bool hda;

	dest = param->dest;
	dpitch = param->dpitch;
	dx = param->dx;
	dy = param->dy;
	w = param->w;
	h = param->h;
	src = reinterpret_cast<const tjs_uint8*>(param->src);
	spitch = param->spitch;
	sx = param->sx;
	sy = param->sy;
	method = param->method;
	opa = param->opa;
	hda = param->hda;

	dest += dy * dpitch + dx * sizeof(tjs_uint32);
	src  += sy * spitch + sx * sizeof(tjs_uint32);

#define TVP_BLEND_4(basename) /* blend for 4 types (normal, opacity, HDA, HDA opacity) */ \
	if(opa == 255)                                                            \
	{                                                                         \
		if(!hda)                                                              \
		{                                                                     \
			while(h--)                                                        \
				basename((tjs_uint32*)dest, (tjs_uint32*)src, w),             \
				dest+=dpitch, src+=spitch;                                    \
                                                                              \
		}                                                                     \
		else                                                                  \
		{                                                                     \
			while(h--)                                                        \
				basename##_HDA((tjs_uint32*)dest, (tjs_uint32*)src, w),       \
				dest+=dpitch, src+=spitch;                                    \
		}                                                                     \
	}                                                                         \
	else                                                                      \
	{                                                                         \
		if(!hda)                                                              \
		{                                                                     \
			while(h--)                                                        \
				basename##_o((tjs_uint32*)dest, (tjs_uint32*)src, w, opa),    \
				dest+=dpitch, src+=spitch;                                    \
		}                                                                     \
		else                                                                  \
		{                                                                     \
			while(h--)                                                        \
				basename##_HDA_o((tjs_uint32*)dest, (tjs_uint32*)src, w, opa),\
				dest+=dpitch, src+=spitch;                                    \
		}                                                                     \
	}


	switch(method)
	{
	case bmCopy:
		// constant ratio alpha blendng
		if(opa == 255 && hda)
		{
			while(h--)
				TVPCopyColor((tjs_uint32*)dest, (tjs_uint32*)src, w),
					dest+=dpitch, src+=spitch;
		}
		else if(!hda)
		{
			while(h--)
				TVPConstAlphaBlend((tjs_uint32*)dest, (tjs_uint32*)src, w, opa),
					dest+=dpitch, src+=spitch;
		}
		else
		{
			while(h--)
				TVPConstAlphaBlend_HDA((tjs_uint32*)dest, (tjs_uint32*)src, w, opa),
					dest+=dpitch, src+=spitch;
		}
		break;

	case bmCopyOnAlpha:
		// constant ratio alpha blending (assuming source is opaque)
		// with consideration of destination alpha
		if(opa == 255)
			while(h--)
				TVPCopyOpaqueImage((tjs_uint32*)dest, (tjs_uint32*)src, w),
				dest+=dpitch, src+=spitch;
		else
			while(h--)
				TVPConstAlphaBlend_d((tjs_uint32*)dest, (tjs_uint32*)src, w, opa),
				dest+=dpitch, src+=spitch;
		break;


	case bmAlpha:
		// alpha blending, ignoring destination alpha
		TVP_BLEND_4(TVPAlphaBlend);
		break;

	case bmAlphaOnAlpha:
		// alpha blending, with consideration of destination alpha
		if(opa == 255)
			while(h--)
				TVPAlphaBlend_d((tjs_uint32*)dest, (tjs_uint32*)src, w),
				dest+=dpitch, src+=spitch;
		else
			while(h--)
				TVPAlphaBlend_do((tjs_uint32*)dest, (tjs_uint32*)src, w, opa),
				dest+=dpitch, src+=spitch;
		break;

	case bmAdd:
		// additive blending ( this does not consider distination alpha )
		TVP_BLEND_4(TVPAddBlend);
		break;

	case bmSub:
		// subtractive blending ( this does not consider distination alpha )
		TVP_BLEND_4(TVPSubBlend);
		break;

	case bmMul:
		// multiplicative blending ( this does not consider distination alpha )
		TVP_BLEND_4(TVPMulBlend);
		break;


	case bmDodge:
		// color dodge mode ( this does not consider distination alpha )
		TVP_BLEND_4(TVPColorDodgeBlend);
		break;


	case bmDarken:
		// darken mode ( this does not consider distination alpha )
		TVP_BLEND_4(TVPDarkenBlend);
		break;


	case bmLighten:
		// lighten mode ( this does not consider distination alpha )
		TVP_BLEND_4(TVPLightenBlend);
		break;


	case bmScreen:
		// screen multiplicative mode ( this does not consider distination alpha )
		TVP_BLEND_4(TVPScreenBlend);
		break;


	case bmAddAlpha:
		// Additive Alpha
		TVP_BLEND_4(TVPAdditiveAlphaBlend);
		break;


	case bmAddAlphaOnAddAlpha:
		// Additive Alpha on Additive Alpha
		if(opa == 255)
		{
			while(h--)
				TVPAdditiveAlphaBlend_a((tjs_uint32*)dest, (tjs_uint32*)src, w),
				dest+=dpitch, src+=spitch;
		}
		else
		{
			while(h--)
				TVPAdditiveAlphaBlend_ao((tjs_uint32*)dest, (tjs_uint32*)src, w, opa),
				dest+=dpitch, src+=spitch;
		}
		break;


	case bmAddAlphaOnAlpha:
		// additive alpha on simple alpha
		// Not yet implemented
		break;

	case bmAlphaOnAddAlpha:
		// simple alpha on additive alpha
		if(opa == 255)
		{
			while(h--)
				TVPAlphaBlend_a((tjs_uint32*)dest, (tjs_uint32*)src, w),
				dest+=dpitch, src+=spitch;
		}
		else
		{
			while(h--)
				TVPAlphaBlend_ao((tjs_uint32*)dest, (tjs_uint32*)src, w, opa),
				dest+=dpitch, src+=spitch;
		}
		break;

	case bmCopyOnAddAlpha:
		// constant ratio alpha blending (assuming source is opaque)
		// with consideration of destination additive alpha
		if(opa == 255)
			while(h--)
				TVPCopyOpaqueImage((tjs_uint32*)dest, (tjs_uint32*)src, w),
				dest+=dpitch, src+=spitch;
		else
			while(h--)
				TVPConstAlphaBlend_a((tjs_uint32*)dest, (tjs_uint32*)src, w, opa),
				dest+=dpitch, src+=spitch;
		break;


	case bmPsNormal:
		// Photoshop compatible normal blend
		// (may take the same effect as bmAlpha)
		TVP_BLEND_4(TVPPsAlphaBlend);
		break;

	case bmPsAdditive:
		// Photoshop compatible additive blend
		TVP_BLEND_4(TVPPsAddBlend);
		break;

	case bmPsSubtractive:
		// Photoshop compatible subtractive blend
		TVP_BLEND_4(TVPPsSubBlend);
		break;

	case bmPsMultiplicative:
		// Photoshop compatible multiplicative blend
		TVP_BLEND_4(TVPPsMulBlend);
		break;

	case bmPsScreen:
		// Photoshop compatible screen blend
		TVP_BLEND_4(TVPPsScreenBlend);
		break;

	case bmPsOverlay:
		// Photoshop compatible overlay blend
		TVP_BLEND_4(TVPPsOverlayBlend);
		break;

	case bmPsHardLight:
		// Photoshop compatible hard light blend
		TVP_BLEND_4(TVPPsHardLightBlend);
		break;

	case bmPsSoftLight:
		// Photoshop compatible soft light blend
		TVP_BLEND_4(TVPPsSoftLightBlend);
		break;

	case bmPsColorDodge:
		// Photoshop compatible color dodge blend
		TVP_BLEND_4(TVPPsColorDodgeBlend);
		break;

	case bmPsColorDodge5:
		// Photoshop 5.x compatible color dodge blend
		TVP_BLEND_4(TVPPsColorDodge5Blend);
		break;

	case bmPsColorBurn:
		// Photoshop compatible color burn blend
		TVP_BLEND_4(TVPPsColorBurnBlend);
		break;

	case bmPsLighten:
		// Photoshop compatible compare (lighten) blend
		TVP_BLEND_4(TVPPsLightenBlend);
		break;

	case bmPsDarken:
		// Photoshop compatible compare (darken) blend
		TVP_BLEND_4(TVPPsDarkenBlend);
		break;

	case bmPsDifference:
		// Photoshop compatible difference blend
		TVP_BLEND_4(TVPPsDiffBlend);
		break;

	case bmPsDifference5:
		// Photoshop 5.x compatible difference blend
		TVP_BLEND_4(TVPPsDiff5Blend);
		break;

	case bmPsExclusion:
		// Photoshop compatible exclusion blend
		TVP_BLEND_4(TVPPsExclusionBlend);
		break;


	default:
				 ;
	}
}

void TJS_USERENTRY PartialBltEntry(void *v)
{
	const PartialBltParam *param = (const PartialBltParam *)v;
	PartialBlt(param);
}

bool Blt(tTVPBaseBitmap *destbmp, tjs_int x, tjs_int y, const tTVPBaseBitmap *ref,
		tTVPRect refrect, tTVPBBBltMethod method, tjs_int opa, bool hda)
{
	// blt src bitmap with various methods.

	// hda option ( hold destination alpha ) holds distination alpha,
	// but will select more complex function ( and takes more time ) for it ( if
	// can do )

	// this function does not matter whether source and destination bitmap is
	// overlapped.

	if(opa == 255 && method == bmCopy && !hda)
	{
		return LayerCopyRect(destbmp, x, y, ref, refrect, TVP_BB_COPY_MAIN|TVP_BB_COPY_MASK);
	}

	if(!destbmp->Is32BPP()) TVPThrowExceptionMessage(TJS_W("Invalid operation for 8bpp"));

	if(opa == 0) return false; // opacity==0 has no action

	// bound check
	tjs_int bmpw, bmph;

	bmpw = ref->GetWidth();
	bmph = ref->GetHeight();

	if(refrect.left < 0)
		x -= refrect.left, refrect.left = 0;
	if(refrect.right > bmpw)
		refrect.right = bmpw;

	if(refrect.left >= refrect.right) return false;

	if(refrect.top < 0)
		y -= refrect.top, refrect.top = 0;
	if(refrect.bottom > bmph)
		refrect.bottom = bmph;

	if(refrect.top >= refrect.bottom) return false;

	bmpw = destbmp->GetWidth();
	bmph = destbmp->GetHeight();


	tTVPRect rect;
	rect.left = x;
	rect.top = y;
	rect.right = rect.left + refrect.get_width();
	rect.bottom = rect.top + refrect.get_height();

	if(rect.left < 0)
	{
		refrect.left += -rect.left;
		rect.left = 0;
	}

	if(rect.right > bmpw)
	{
		refrect.right -= (rect.right - bmpw);
		rect.right = bmpw;
	}

	if(refrect.left >= refrect.right) return false; // not drawable

	if(rect.top < 0)
	{
		refrect.top += -rect.top;
		rect.top = 0;
	}

	if(rect.bottom > bmph)
	{
		refrect.bottom -= (rect.bottom - bmph);
		rect.bottom = bmph;
	}

	if(refrect.top >= refrect.bottom) return false; // not drawable

    tjs_uint8 *dest = (tjs_uint8*)destbmp->GetScanLineForWrite(0);
    const tjs_uint8 *src = (const tjs_uint8*)ref->GetScanLine(0);
    tjs_int dpitch = destbmp->GetPitchBytes();
    tjs_int spitch = ref->GetPitchBytes();
	tjs_int w = refrect.get_width();
	tjs_int h = refrect.get_height();

    tjs_int taskNum = GetAdaptiveThreadNum(w * h, sBmFactor[method]);
    TVPBeginThreadTask(taskNum);
    PartialBltParam params[TVPMaxThreadNum];
    for (tjs_int i = 0; i < taskNum; i++)
    {
		tjs_int y0, y1;
		y0 = h * i / taskNum;
		y1=  h * (i + 1) / taskNum;
		PartialBltParam *param = params + i;
		param->self = destbmp;
		param->dest = dest;
		param->dpitch = dpitch;
		param->dx = rect.left;
		param->dy = rect.top + y0;
		param->w = w;
		param->h = y1 - y0;
		param->src = reinterpret_cast<const tjs_int8*>(src);
		param->spitch = spitch;
		param->sx = refrect.left;
		param->sy = refrect.top + y0;
		param->method = method;
		param->opa = opa;
		param->hda = hda;
		TVPExecThreadTask(&PartialBltEntry, TVP_THREAD_PARAM(param));
    }
    TVPEndThreadTask();

    return true;
}
