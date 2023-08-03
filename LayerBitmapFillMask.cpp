
#include "tjsCommHead.h"
#include "tvpgl.h"
#include "LayerBitmapIntf.h"
#include "LayerBitmapUtility2.h"
#include "LayerBitmapFillMask.h"

struct PartialFillMaskParam {
  tTVPBaseBitmap *self;
  tjs_uint8 *dest;
  tjs_int x;
  tjs_int y;
  tjs_int w;
  tjs_int h;
  tjs_int pitch;
  tjs_int value;
};

void PartialFillMask(const PartialFillMaskParam *param)
{
    tjs_int pitch = param->pitch;
    tjs_uint8 *sc = param->dest + pitch * param->y;
    tjs_int left = param->x;
    tjs_int width = param->w;
    tjs_int height = param->h;
    tjs_int value = param->value;

    while(height--)
    {
        tjs_uint32 * p = (tjs_uint32*)sc + left;
        TVPFillMask(p, width, value);
        sc += pitch;
    }
}
//---------------------------------------------------------------------------

void TJS_USERENTRY PartialFillMaskEntry(void *v)
{
    const PartialFillMaskParam *param = (const PartialFillMaskParam *)v;
    PartialFillMask(param);
}

bool FillMask(tTVPBaseBitmap *destbmp, tTVPRect rect, tjs_int value)
{
	// fill mask with specified value.
	BOUND_CHECK(false);

	if(!destbmp->Is32BPP()) TVPThrowExceptionMessage(TJS_W("Invalid operation for 8bpp"));

	tjs_int pitch = destbmp->GetPitchBytes();
	tjs_uint8 *dest = (tjs_uint8*)destbmp->GetScanLineForWrite(0);
    tjs_int h = rect.bottom - rect.top;
    tjs_int w = rect.right - rect.left;

    tjs_int taskNum = GetAdaptiveThreadNum(w * h, 84);
    TVPBeginThreadTask(taskNum);
    PartialFillMaskParam params[TVPMaxThreadNum];
    for (tjs_int i = 0; i < taskNum; i++) {
        tjs_int y0, y1;
        y0 = h * i / taskNum;
        y1 = h * (i + 1) / taskNum;
        PartialFillMaskParam *param = params + i;
        param->self = destbmp;
        param->dest = dest;
        param->pitch = pitch;
        param->x = rect.left;
        param->y = rect.top + y0;
        param->w = w;
        param->h = y1 - y0;
        param->value = value;
        TVPExecThreadTask(&PartialFillMaskEntry, TVP_THREAD_PARAM(param));
    }
    TVPEndThreadTask();

    return true;
}
