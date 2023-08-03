
#include "tjsCommHead.h"
#include "tvpgl.h"
#include "LayerBitmapIntf.h"
#include "LayerBitmapUtility2.h"
#include "LayerBitmapFill.h"

struct PartialFillParam {
  tTVPBaseBitmap *self;
  tjs_uint8 *dest;
  tjs_int x;
  tjs_int y;
  tjs_int w;
  tjs_int h;
  tjs_int pitch;
  tjs_uint32 value;
  bool is32bpp;
};

void PartialFill(const PartialFillParam *param)
{
	if(param->is32bpp)
	{
		// 32bpp
		tjs_int pitch = param->pitch;
		tjs_uint8 *sc = param->dest + param->y * pitch;
        tjs_int x = param->x;
		tjs_int height = param->h;
		tjs_int width = param->w;
        tjs_uint32 value = param->value;

        // don't use no cache version. (for test reason)
#if 0
		if(height * width >= 64*1024/4)
		{
            while (height--) 
			{
				tjs_uint32 * p = (tjs_uint32*)sc + x;
				TVPFillARGB_NC(p, width, value);
				sc += pitch;
			}
		}
		else
#endif
		{
            while (height--)
			{
				tjs_uint32 * p = (tjs_uint32*)sc + x;
				TVPFillARGB(p, width, value);
				sc += pitch;
			}
		}
	}
	else
	{
		// 8bpp
		tjs_int pitch = param->pitch;
		tjs_uint8 *sc = param->dest + param->y * pitch;
        tjs_int x = param->x;
        tjs_int height = param->h;
        tjs_int width = param->w;
        tjs_uint32 value = param->value;

		while (height--)
		{
			tjs_uint8 * p = (tjs_uint8*)sc + x;
			memset(p, value, width);
			sc += pitch;
		}
	}
}
//---------------------------------------------------------------------------


void TJS_USERENTRY PartialFillEntry(void *v)
{
	const PartialFillParam *param = (const PartialFillParam *)v;
	PartialFill(param);
}

bool Fill(tTVPBaseBitmap *destbmp, tTVPRect rect, tjs_uint32 value)
{
	// fill target rectangle represented as "rect", with color ( and opacity )
	// passed by "value".
	// value must be : 0xAARRGGBB (for 32bpp) or 0xII ( for 8bpp )
	BOUND_CHECK(false);

	// Probably don't need to independ at this point, it was done before
#if 0
	if(!IsIndependent())
	{
		if(rect.left == 0 && rect.top == 0 &&
			rect.right == (tjs_int)GetWidth() && rect.bottom == (tjs_int)GetHeight())
		{
			// cover overall
			IndependNoCopy(); // indepent with no-copy
		}
	}
#endif

    tjs_int pitch = destbmp->GetPitchBytes();
    tjs_uint8 *dest = (tjs_uint8*)destbmp->GetScanLineForWrite(0);
    tjs_int h = rect.bottom - rect.top;
    tjs_int w = rect.right - rect.left;
    bool is32bpp = destbmp->Is32BPP();

    tjs_int taskNum = GetAdaptiveThreadNum(w * h, 150);
    TVPBeginThreadTask(taskNum);
    PartialFillParam params[TVPMaxThreadNum];
    for (tjs_int i = 0; i < taskNum; i++) {
		tjs_int y0, y1;
		y0 = h * i / taskNum;
		y1 = h * (i + 1) / taskNum;
		PartialFillParam *param = params + i;
		param->self = destbmp;
		param->dest = dest;
		param->pitch = pitch;
		param->x = rect.left;
		param->y = rect.top + y0;
		param->w = w;
		param->h = y1 - y0;
		param->value = value;
		param->is32bpp = is32bpp;
		TVPExecThreadTask(&PartialFillEntry, TVP_THREAD_PARAM(param));
    }
    TVPEndThreadTask();

    return true;
}
