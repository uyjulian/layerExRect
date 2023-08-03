
#include "tjsCommHead.h"
#include "tvpgl.h"
#include "LayerBitmapIntf.h"
#include "LayerBitmapUtility2.h"
#include "LayerBitmapCopyRect.h"

struct PartialCopyRectParam {
  tTVPBaseBitmap *self;
  tjs_int pixelsize;
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
  tjs_int plane;
  bool backwardCopy;
};

void PartialCopyRect(const PartialCopyRectParam *param)
{
	// transfer
	tjs_int pixelsize = param->pixelsize;
	tjs_int dpitch = param->dpitch;
	tjs_int spitch = param->spitch;
	tjs_int w = param->w;
	tjs_int wbytes = param->w * pixelsize;
	tjs_int h = param->h;
        tjs_int plane = param->plane;
        bool backwardCopy = param->backwardCopy;

	if(backwardCopy)
	{
		// backward copy
#if 0
		tjs_uint8 * dest = (tjs_uint8*)GetScanLineForWrite(rect.bottom-1) +
			rect.left*pixelsize;
		const tjs_uint8 * src = (const tjs_uint8*)ref->GetScanLine(refrect.bottom-1) +
			refrect.left*pixelsize;
#endif
		tjs_uint8 * dest = param->dest + dpitch * (param->dy + param->h - 1) + param->dx * pixelsize;
		const tjs_uint8 * src = reinterpret_cast<const tjs_uint8*>(param->src + spitch * (param->sy + param->h - 1) + param->sx * pixelsize);

		switch(plane)
		{
		case TVP_BB_COPY_MAIN:
			while(h--)
			{
				TVPCopyColor((tjs_uint32*)dest, (const tjs_uint32*)src, w);
				dest -= dpitch;
				src -= spitch;
			}
			break;
		case TVP_BB_COPY_MASK:
			while(h--)
			{
				TVPCopyMask((tjs_uint32*)dest, (const tjs_uint32*)src, w);
				dest -= dpitch;
				src -= spitch;
			}
			break;
		case TVP_BB_COPY_MAIN|TVP_BB_COPY_MASK:
			while(h--)
			{
				memmove(dest, src, wbytes);
				dest -= dpitch;
				src -= spitch;
			}
			break;
		}
	}
	else
	{
		// forward copy
#if 0
		tjs_uint8 * dest = (tjs_uint8*)GetScanLineForWrite(rect.top) +
			rect.left*pixelsize;
		const tjs_uint8 * src = (const tjs_uint8*)ref->GetScanLine(refrect.top) +
			refrect.left*pixelsize;
#endif
		tjs_uint8 * dest = param->dest + dpitch * (param->dy) + param->dx * pixelsize;
		const tjs_uint8 * src = reinterpret_cast<const tjs_uint8*>(param->src + spitch * (param->sy) + param->sx * pixelsize);

		switch(plane)
		{
		case TVP_BB_COPY_MAIN:
			while(h--)
			{
				TVPCopyColor((tjs_uint32*)dest, (const tjs_uint32*)src, w);
				dest += dpitch;
				src += spitch;
			}
			break;
		case TVP_BB_COPY_MASK:
			while(h--)
			{
				TVPCopyMask((tjs_uint32*)dest, (const tjs_uint32*)src, w);
				dest += dpitch;
				src += spitch;
			}
			break;
		case TVP_BB_COPY_MAIN|TVP_BB_COPY_MASK:
			while(h--)
			{
				memmove(dest, src, wbytes);
				dest += dpitch;
				src += spitch;
			}
			break;
		}
	}
}
//---------------------------------------------------------------------------

void TJS_USERENTRY PartialCopyRectEntry(void *v)
{
	const PartialCopyRectParam *param = (const PartialCopyRectParam *)v;
	PartialCopyRect(param);
}

bool LayerCopyRect(tTVPBaseBitmap *destbmp, tjs_int x, tjs_int y, const tTVPBaseBitmap *ref,
		tTVPRect refrect, tjs_int plane)
{
	// copy bitmap rectangle.
	// TVP_BB_COPY_MAIN in "plane" : main image is copied
	// TVP_BB_COPY_MASK in "plane" : mask image is copied
	// "plane" is ignored if the bitmap is 8bpp
	// the source rectangle is ( "refrect" ) and the destination upper-left corner
	// is (x, y).
	if(!destbmp->Is32BPP()) plane = (TVP_BB_COPY_MASK|TVP_BB_COPY_MAIN);
	// Probably need to do this in parent function
#if 0
	if(x == 0 && y == 0 && refrect.left == 0 && refrect.top == 0 &&
		refrect.right == (tjs_int)ref->GetWidth() &&
		refrect.bottom == (tjs_int)ref->GetHeight() &&
		(tjs_int)destbmp->GetWidth() == refrect.right &&
		(tjs_int)destbmp->GetHeight() == refrect.bottom &&
		plane == (TVP_BB_COPY_MASK|TVP_BB_COPY_MAIN) &&
		(bool)!destbmp->Is32BPP() == (bool)!ref->Is32BPP())
	{
		// entire area of both bitmaps
		AssignBitmap(*ref);
		return true;
	}
#endif

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


    // transfer
    tjs_uint8 *dest = (tjs_uint8*)destbmp->GetScanLineForWrite(0);
    const tjs_uint8 *src = (const tjs_uint8*)ref->GetScanLine(0);
    tjs_int dpitch = destbmp->GetPitchBytes();
    tjs_int spitch = ref->GetPitchBytes();
	tjs_int w = refrect.get_width();
	tjs_int h = refrect.get_height();
	tjs_int pixelsize = (destbmp->Is32BPP()?sizeof(tjs_uint32):sizeof(tjs_uint8));
    bool backwardCopy = (ref == destbmp && rect.top > refrect.top);

    tjs_int taskNum = (ref == destbmp) ? 1 : GetAdaptiveThreadNum(w * h, 66);
    TVPBeginThreadTask(taskNum);
    PartialCopyRectParam params[TVPMaxThreadNum];
    for (tjs_int i = 0; i < taskNum; i++) {
		tjs_int y0, y1;
		y0 = h * i / taskNum;
		y1=  h * (i + 1) / taskNum;
		PartialCopyRectParam *param = params + i;
		param->self = destbmp;
		param->pixelsize = pixelsize;
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
		param->plane = plane;
		param->backwardCopy = backwardCopy;
		TVPExecThreadTask(&PartialCopyRectEntry, TVP_THREAD_PARAM(param));
    }
    TVPEndThreadTask();

    return true;
}
