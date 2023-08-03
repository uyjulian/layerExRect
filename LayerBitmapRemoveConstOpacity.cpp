
#include "tjsCommHead.h"
#include "tvpgl.h"
#include "LayerBitmapIntf.h"
#include "LayerBitmapUtility2.h"
#include "LayerBitmapRemoveConstOpacity.h"

struct PartialRemoveConstOpacityParam {
  tTVPBaseBitmap *self;
  tjs_uint8 *dest;
  tjs_int x;
  tjs_int y;
  tjs_int w;
  tjs_int h;
  tjs_int pitch;
  tjs_int level;
};

void PartialRemoveConstOpacity(const PartialRemoveConstOpacityParam *param)
{
  tjs_int pitch = param->pitch;
  tjs_uint8 *sc = param->dest + pitch * param->y;
  tjs_int left = param->x;
  tjs_int width = param->w;
  tjs_int height = param->h;
  tjs_int level = param->level;

  if(level == 255)
  {
    // completely remove opacity
                while(height--)
    {
      tjs_uint32 * p = (tjs_uint32*)sc + left;
      TVPFillMask(p, width, 0);
      sc += pitch;
    }
  }
  else
  {
                while(height--)
    {
      tjs_uint32 * p = (tjs_uint32*)sc + left;
      TVPRemoveConstOpacity(p, width, level);
      sc += pitch;
    }

  }
}

void TJS_USERENTRY PartialRemoveConstOpacityEntry(void *v)
{
  const PartialRemoveConstOpacityParam *param = (const PartialRemoveConstOpacityParam *)v;
  PartialRemoveConstOpacity(param);
}

bool RemoveConstOpacity(tTVPBaseBitmap *destbmp, tTVPRect rect, tjs_int level)
{
	// remove constant opacity from bitmap. ( similar to PhotoShop's eraser tool )
	// level is a strength of removing ( 0 thru 255 )
	// this cannot work with additive alpha mode.

	BOUND_CHECK(false);

	if(!destbmp->Is32BPP()) TVPThrowExceptionMessage(TJS_W("Invalid operation for 8bpp"));

	if(level == 0) return false; // no action
	if(level < 0) level = 0;
	if(level > 255) level = 255;

	tjs_int pitch = destbmp->GetPitchBytes();
	tjs_uint8 *dest = (tjs_uint8*)destbmp->GetScanLineForWrite(0);
        tjs_int h = rect.bottom - rect.top;
        tjs_int w = rect.right - rect.left;

        tjs_int taskNum = GetAdaptiveThreadNum(w * h, level == 255 ? 83.f : 50.f);
        TVPBeginThreadTask(taskNum);
        PartialRemoveConstOpacityParam params[TVPMaxThreadNum];
        for (tjs_int i = 0; i < taskNum; i++) {
          tjs_int y0, y1;
          y0 = h * i / taskNum;
          y1 = h * (i + 1) / taskNum;
          PartialRemoveConstOpacityParam *param = params + i;
          param->self = destbmp;
          param->dest = dest;
          param->pitch = pitch;
          param->x = rect.left;
          param->y = rect.top + y0;
          param->w = w;
          param->h = y1 - y0;
          param->level = level;
          TVPExecThreadTask(&PartialRemoveConstOpacityEntry, TVP_THREAD_PARAM(param));
        }
        TVPEndThreadTask();

        return true;
}
