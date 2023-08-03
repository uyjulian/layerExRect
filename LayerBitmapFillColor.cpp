
#include "tjsCommHead.h"
#include "tvpgl.h"
#include "LayerBitmapIntf.h"
#include "LayerBitmapUtility2.h"
#include "LayerBitmapFillColor.h"

struct PartialFillColorParam {
  tTVPBaseBitmap *self;
  tjs_uint8 *dest;
  tjs_int x;
  tjs_int y;
  tjs_int w;
  tjs_int h;
  tjs_int pitch;
  tjs_uint32 color;
  tjs_int opa;
};

void PartialFillColor(const PartialFillColorParam *param)
{
    tjs_uint8 *sc = param->dest + param->y * param->pitch;
    tjs_int opa = param->opa;
    tjs_uint32 color = param->color;
    tjs_int left = param->x;
    tjs_int width = param->w;
    tjs_int height = param->h;
    tjs_int pitch = param->pitch;
        
    if(opa == 255)
    {
        // complete opaque fill
        while(height--)
        {
            tjs_uint32 * p = (tjs_uint32*)sc + left;
            TVPFillColor(p, width, color);
            sc += pitch;
        }
    }
    else
    {
        // alpha fill
        while(height--)
        {
            tjs_uint32 * p = (tjs_uint32*)sc + left;
            TVPConstColorAlphaBlend(p, width, color, opa);
            sc += pitch;
        }
    }
}
//---------------------------------------------------------------------------

void TJS_USERENTRY PartialFillColorEntry(void *v)
{
    const PartialFillColorParam *param = (const PartialFillColorParam *)v;
    PartialFillColor(param);
}

bool FillColor(tTVPBaseBitmap *destbmp, tTVPRect rect, tjs_uint32 color, tjs_int opa)
{
	// fill rectangle with specified color.
	// this ignores destination alpha (destination alpha will not change)
	// opa is fill opacity if opa is positive value.
	// negative value of opa is not allowed.
	BOUND_CHECK(false);

	if(!destbmp->Is32BPP()) TVPThrowExceptionMessage(TJS_W("Invalid operation for 8bpp"));

	if(opa == 0) return false; // no action

	if(opa < 0) opa = 0;
	if(opa > 255) opa = 255;

	tjs_int pitch = destbmp->GetPitchBytes();
	tjs_uint8 *dest = (tjs_uint8*)destbmp->GetScanLineForWrite(0);
    tjs_int h = rect.bottom - rect.top;
    tjs_int w = rect.right - rect.left;

    tjs_int taskNum = GetAdaptiveThreadNum(w * h, opa == 255 ? 115.f : 55.f);
    TVPBeginThreadTask(taskNum);
    PartialFillColorParam params[TVPMaxThreadNum];
    for (tjs_int i = 0; i < taskNum; i++) {
        tjs_int y0, y1;
        y0 = h * i / taskNum;
        y1 = h * (i + 1) / taskNum;
        PartialFillColorParam *param = params + i;
        param->self = destbmp;
        param->dest = dest;
        param->pitch = pitch;
        param->x = rect.left;
        param->y = rect.top + y0;
        param->w = w;
        param->h = y1 - y0;
        param->color = color;
        param->opa = opa;
        TVPExecThreadTask(&PartialFillColorEntry, TVP_THREAD_PARAM(param));
    }
    TVPEndThreadTask();

    return true;
}
