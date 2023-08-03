
#include "tjsCommHead.h"
#include "tvpgl.h"
#include "LayerBitmapIntf.h"
#include "LayerBitmapUtility2.h"
#include "LayerBitmapBlendColor.h"

struct PartialBlendColorParam {
  tTVPBaseBitmap *self;
  tjs_uint8 *dest;
  tjs_int x;
  tjs_int y;
  tjs_int w;
  tjs_int h;
  tjs_int pitch;
  tjs_uint32 color;
  tjs_int opa;
  bool additive;
};

void PartialBlendColor(const PartialBlendColorParam *param)
{
    tjs_uint32 color = param->color;
    tjs_int opa = param->opa;
    bool additive = param->additive;

    if(opa == 255)
    {
        // complete opaque fill
        color |= 0xff000000;

        tjs_int pitch = param->pitch;
        tjs_uint8 *sc = param->dest + pitch * param->y;
        tjs_int left = param->x;
        tjs_int width = param->w;
        tjs_int height = param->h;

        while (height--)
        {
            tjs_uint32 * p = (tjs_uint32*)sc + left;
            TVPFillARGB(p, width, color);
            sc += pitch;
        }
    }
    else
    {
        // alpha fill
        tjs_int pitch = param->pitch;
        tjs_uint8 *sc = param->dest + pitch * param->y;
        tjs_int left = param->x;
        tjs_int width = param->w;
        tjs_int height = param->h;

        if(!additive)
        {
            while(height--)
            {
                tjs_uint32 * p = (tjs_uint32*)sc + left;
                TVPConstColorAlphaBlend_d(p, width, color, opa);
                sc += pitch;
            }
        }
        else
        {
            while(height--)
            {
                tjs_uint32 * p = (tjs_uint32*)sc + left;
                TVPConstColorAlphaBlend_a(p, width, color, opa);
                sc += pitch;
            }
        }
    }
}
//---------------------------------------------------------------------------

void TJS_USERENTRY PartialBlendColorEntry(void *v)
{
    const PartialBlendColorParam *param = (const PartialBlendColorParam *)v;
    PartialBlendColor(param);
}

bool BlendColor(tTVPBaseBitmap *destbmp, tTVPRect rect, tjs_uint32 color, tjs_int opa,
	bool additive)
{
	// fill rectangle with specified color.
	// this considers destination alpha (additive or simple)

	BOUND_CHECK(false);

	if(!destbmp->Is32BPP()) TVPThrowExceptionMessage(TJS_W("Invalid operation for 8bpp"));

	if(opa == 0) return false; // no action
	if(opa < 0) opa = 0;
	if(opa > 255) opa = 255;

#if 0
    if(opa == 255 && !destbmp->IsIndependent())
    {
        if(rect.left == 0 && rect.top == 0 &&
            rect.right == (tjs_int)destbmp->GetWidth() && rect.bottom == (tjs_int)destbmp->GetHeight())
        {
            // cover overall
            destbmp->IndependNoCopy(); // indepent with no-copy
        }
    }
#endif

	tjs_int pitch = destbmp->GetPitchBytes();
	tjs_uint8 *dest = (tjs_uint8*)destbmp->GetScanLineForWrite(0);
    tjs_int h = rect.bottom - rect.top;
    tjs_int w = rect.right - rect.left;

    tjs_int factor;
    if (opa == 255)
        factor = 148;
    else if (! additive)
        factor = 25;
    else
        factor = 147;
    tjs_int taskNum = GetAdaptiveThreadNum(w * h, static_cast<float>(factor) );
    TVPBeginThreadTask(taskNum);
    PartialBlendColorParam params[TVPMaxThreadNum];
    for (tjs_int i = 0; i < taskNum; i++) {
        tjs_int y0, y1;
        y0 = h * i / taskNum;
        y1 = h * (i + 1) / taskNum;
        PartialBlendColorParam *param = params + i;
        param->self = destbmp;
        param->dest = dest;
        param->pitch = pitch;
        param->x = rect.left;
        param->y = rect.top + y0;
        param->w = w;
        param->h = y1 - y0;
        param->color = color;
        param->opa = opa;
        param->additive = additive;
        TVPExecThreadTask(&PartialBlendColorEntry, TVP_THREAD_PARAM(param));
    }
    TVPEndThreadTask();

    return true;
}

bool FillColorOnAlpha(tTVPBaseBitmap *destbmp, tTVPRect rect, tjs_uint32 color, tjs_int opa)
{
    return BlendColor(destbmp, rect, color, opa, false);
}

bool FillColorOnAddAlpha(tTVPBaseBitmap *destbmp, tTVPRect rect, tjs_uint32 color, tjs_int opa)
{
    return BlendColor(destbmp, rect, color, opa, true);
}

