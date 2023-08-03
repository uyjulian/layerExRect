/////////////////////////////////////////////
//                                         //
//    Copyright (C) 2023-2023 Julian Uy    //
//  https://sites.google.com/site/awertyb  //
//                                         //
//   See details of license at "LICENSE"   //
//                                         //
/////////////////////////////////////////////

#include "ncbind/ncbind.hpp"
#include "LayerBitmapIntf.h"
#include "LayerBitmapUtility.h"
#include "ComplexRect.h"
#include "LayerBitmapBlendColor.h"
#include "LayerBitmapBlit.h"
#include "LayerBitmapCopyRect.h"
#include "LayerBitmapFill.h"
#include "LayerBitmapFillColor.h"
#include "LayerBitmapFillMask.h"
#include "LayerBitmapRemoveConstOpacity.h"
#include <string.h>
#include <stdio.h>

static bool ClipDestPointAndSrcRect(const tTVPRect &ClipRect, tjs_int &dx, tjs_int &dy,
		tTVPRect &srcrectout, const tTVPRect &srcrect)
{
	// clip (dx, dy) <- srcrect	with current clipping rectangle
	srcrectout = srcrect;
	tjs_int dr = dx + srcrect.right - srcrect.left;
	tjs_int db = dy + srcrect.bottom - srcrect.top;

	if(dx < ClipRect.left)
	{
		srcrectout.left += (ClipRect.left - dx);
		dx = ClipRect.left;
	}

	if(dr > ClipRect.right)
	{
		srcrectout.right -= (dr - ClipRect.right);
	}

	if(srcrectout.right <= srcrectout.left) return false; // out of the clipping rect

	if(dy < ClipRect.top)
	{
		srcrectout.top += (ClipRect.top - dy);
		dy = ClipRect.top;
	}

	if(db > ClipRect.bottom)
	{
		srcrectout.bottom -= (db - ClipRect.bottom);
	}

	if(srcrectout.bottom <= srcrectout.top) return false; // out of the clipping rect

	return true;
}

static void PreRegistCallback()
{
	iTJSDispatch2 *global = TVPGetScriptDispatch();
	if (global)
	{
		tTJSVariant layer_val;
		static ttstr Layer_name(TJS_W("Layer"));
		global->PropGet(0, Layer_name.c_str(), Layer_name.GetHint(), &layer_val, global);
		tTJSVariantClosure layer_valclosure = layer_val.AsObjectClosureNoAddRef();
		if (layer_valclosure.Object)
		{
			layer_valclosure.DeleteMember(TJS_IGNOREPROP, TJS_W("fillRect"), 0, NULL);
			layer_valclosure.DeleteMember(TJS_IGNOREPROP, TJS_W("colorRect"), 0, NULL);
			layer_valclosure.DeleteMember(TJS_IGNOREPROP, TJS_W("copyRect"), 0, NULL);
			layer_valclosure.DeleteMember(TJS_IGNOREPROP, TJS_W("operateRect"), 0, NULL);
		}
	}
}

NCB_PRE_REGIST_CALLBACK(PreRegistCallback);

class LayerLayerExRect
{
public:
	static tjs_error TJS_INTF_METHOD fillRect(
		tTJSVariant	*result,
		tjs_int numparams,
		tTJSVariant **param,
		iTJSDispatch2 *objthis)
	{
		if(numparams < 5) return TJS_E_BADPARAMCOUNT;
	
		tjs_int x, y;
		x = *param[0];
		y = *param[1];
		tTVPRect rect(x, y, x+(tjs_int)*param[2], y+(tjs_int)*param[3]);
		tjs_uint32 color = static_cast<tjs_uint32>((tjs_int64)*param[4]);

		if(objthis == NULL) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTJSVariant bmpobject = tTJSVariant(objthis, objthis);
		tTJSVariantClosure bmpobject_clo = bmpobject.AsObjectClosureNoAddRef();

		tTVPRect ClipRect;

		// Check bounds first
		GetLayerInformationFromLayerObject(bmpobject_clo, NULL, NULL, &ClipRect, NULL, NULL, NULL, NULL);

		tTVPRect destrect;
		if(!TVPIntersectRect(&destrect, rect, ClipRect)) return TJS_S_OK; // out of the clipping rectangle

		tjs_int bmpwidth = 0;
		tjs_int bmpheight = 0;
		tjs_int bmppitch = 0;
		tjs_uint8* bmpdata = NULL;

		tTVPDrawFace Face = dfAuto; // (outward) current drawing layer face
		tTVPLayerType DisplayType = ltOpaque; // actual Type
		bool HoldAlpha = false;
		tjs_int ImageLeft = 0;
		tjs_int ImageTop = 0;

		GetLayerInformationFromLayerObject(bmpobject_clo, &Face, &DisplayType, NULL, &HoldAlpha, &ImageLeft, &ImageTop, NULL);

		tTVPDrawFace DrawFace = GetDrawFace(Face, DisplayType); // (actual) current drawing layer face

		bool ImageModified = false;
		if (DrawFace == dfProvince)
		{
			tjs_int provincebmpwidth = 0;
			tjs_int provincebmpheight = 0;
			tjs_int provincebmppitch = 0;
			tjs_uint8* provincebmpdata = NULL;

			// province
			color = color & 0xff;
			if (color)
			{
				GetProvinceBitmapInformationFromObject(bmpobject_clo, true, &provincebmpwidth, &provincebmpheight, &provincebmppitch, &provincebmpdata);
				if (provincebmpdata)
				{
					tTVPBaseBitmap provincebmpinfo(provincebmpwidth, provincebmpheight, provincebmppitch, provincebmpdata, false);
					ImageModified = Fill(&provincebmpinfo, destrect, color&0xff) || ImageModified;
				}
			}
			else
			{
				// Check read only for clear
				GetProvinceBitmapInformationFromObject(bmpobject_clo, false, NULL, NULL, NULL, &provincebmpdata);
				if (provincebmpdata)
				{
					GetProvinceBitmapInformationFromObject(bmpobject_clo, true, &provincebmpwidth, &provincebmpheight, &provincebmppitch, &provincebmpdata);
					tTVPBaseBitmap provincebmpinfo(provincebmpwidth, provincebmpheight, provincebmppitch, provincebmpdata, false);
#if 0
					// It doesn't look like we can call DeallocateProvinceImage from script...
					if(destrect.left == 0 && destrect.top == 0 &&
						destrect.right == (tjs_int)ProvinceImage->GetWidth() &&
						destrect.bottom == (tjs_int)ProvinceImage->GetHeight())
					{
						// entire area of the province image will be filled with 0
						DeallocateProvinceImage();
						ImageModified = true;
					}
					else
#endif
					{
						ImageModified = Fill(&provincebmpinfo, destrect, color&0xff) || ImageModified;
					}
				}
			}
		}
		else
		{
			if (DrawFace == dfAlpha || DrawFace == dfAddAlpha || (DrawFace == dfOpaque && !HoldAlpha))
			{
				GetBitmapInformationFromObject(bmpobject_clo, true, &bmpwidth, &bmpheight, &bmppitch, &bmpdata);
				// main and mask
				if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Not drawable layer type"));
				tTVPBaseBitmap bmpinfo(bmpwidth, bmpheight, bmppitch, bmpdata);
				color = (color & 0xff000000) + (TVPToActualColor(color&0xffffff)&0xffffff);
				ImageModified = Fill(&bmpinfo, destrect, color) || ImageModified;
			}
			else if (DrawFace == dfOpaque)
			{
				GetBitmapInformationFromObject(bmpobject_clo, true, &bmpwidth, &bmpheight, &bmppitch, &bmpdata);
				// main only
				if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Not drawable layer type"));
				tTVPBaseBitmap bmpinfo(bmpwidth, bmpheight, bmppitch, bmpdata);
				color = TVPToActualColor(color);
				ImageModified = FillColor(&bmpinfo, destrect, color, 255) || ImageModified;
			}
			else if (DrawFace == dfMask)
			{
				GetBitmapInformationFromObject(bmpobject_clo, true, &bmpwidth, &bmpheight, &bmppitch, &bmpdata);
				// mask only
				if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Not drawable layer type"));
				tTVPBaseBitmap bmpinfo(bmpwidth, bmpheight, bmppitch, bmpdata);
				ImageModified = FillMask(&bmpinfo, destrect, color&0xff) || ImageModified;
			}
		}

		if (ImageModified)
		{
			tTVPRect ur = destrect;
			UpdateLayerWithLayerObject(bmpobject_clo, &ur, &ImageLeft, &ImageTop);
		}
		return TJS_S_OK;
	}

	static tjs_error TJS_INTF_METHOD colorRect(
		tTJSVariant	*result,
		tjs_int numparams,
		tTJSVariant **param,
		iTJSDispatch2 *objthis)
	{
		if(numparams < 5) return TJS_E_BADPARAMCOUNT;
	
		tjs_int x, y;
		x = *param[0];
		y = *param[1];
		tTVPRect rect(x, y, x+(tjs_int)*param[2], y+(tjs_int)*param[3]);
		tjs_uint32 color = static_cast<tjs_uint32>((tjs_int64)*param[4]);
		tjs_int opa = (numparams>=6 && param[5]->Type() != tvtVoid)? (tjs_int) *param[5] : 255;

		if(objthis == NULL) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTJSVariant bmpobject = tTJSVariant(objthis, objthis);
		tTJSVariantClosure bmpobject_clo = bmpobject.AsObjectClosureNoAddRef();

		tTVPRect ClipRect;

		// Check bounds first
		GetLayerInformationFromLayerObject(bmpobject_clo, NULL, NULL, &ClipRect, NULL, NULL, NULL, NULL);

		tTVPRect destrect;
		if(!TVPIntersectRect(&destrect, rect, ClipRect)) return TJS_S_OK; // out of the clipping rectangle

		tjs_int bmpwidth = 0;
		tjs_int bmpheight = 0;
		tjs_int bmppitch = 0;
		tjs_uint8* bmpdata = NULL;

		tjs_int provincebmpwidth = 0;
		tjs_int provincebmpheight = 0;
		tjs_int provincebmppitch = 0;
		tjs_uint8* provincebmpdata = NULL;

		tTVPDrawFace Face = dfAuto; // (outward) current drawing layer face
		tTVPLayerType DisplayType = ltOpaque; // actual Type
		bool HoldAlpha = false;
		tjs_int ImageLeft = 0;
		tjs_int ImageTop = 0;

		GetLayerInformationFromLayerObject(bmpobject_clo, &Face, &DisplayType, NULL, &HoldAlpha, &ImageLeft, &ImageTop, NULL);

		tTVPDrawFace DrawFace = GetDrawFace(Face, DisplayType); // (actual) current drawing layer face

		bool ImageModified = false;

		switch(DrawFace)
		{
		case dfAlpha: // main and mask
		{
			GetBitmapInformationFromObject(bmpobject_clo, true, &bmpwidth, &bmpheight, &bmppitch, &bmpdata);
			if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Not drawable layer type"));
			tTVPBaseBitmap bmpinfo(bmpwidth, bmpheight, bmppitch, bmpdata);
			if(opa > 0)
			{
				color = TVPToActualColor(color);
				ImageModified = FillColorOnAlpha(&bmpinfo, destrect, color, opa) || ImageModified;
			}
			else
			{
				ImageModified = RemoveConstOpacity(&bmpinfo, destrect, -opa) || ImageModified;
			}
			break;
		}

		case dfAddAlpha: // additive alpha; main and mask
		{
			GetBitmapInformationFromObject(bmpobject_clo, true, &bmpwidth, &bmpheight, &bmppitch, &bmpdata);
			if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Not drawable layer type"));
			tTVPBaseBitmap bmpinfo(bmpwidth, bmpheight, bmppitch, bmpdata);
			if(opa >= 0)
			{
				color = TVPToActualColor(color);
				ImageModified = FillColorOnAddAlpha(&bmpinfo, destrect, color, opa) || ImageModified;
			}
			else
			{
				TVPThrowExceptionMessage(TJS_W("Negative opacity not supported on this face"));
			}
			break;
		}

		case dfOpaque: // main only
		{
			GetBitmapInformationFromObject(bmpobject_clo, true, &bmpwidth, &bmpheight, &bmppitch, &bmpdata);
			if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Not drawable layer type"));
			tTVPBaseBitmap bmpinfo(bmpwidth, bmpheight, bmppitch, bmpdata);
			color = TVPToActualColor(color);
			ImageModified = FillColor(&bmpinfo, destrect, color, opa) || ImageModified;
				// note that tTVPBaseBitmap::FillColor always holds destination alpha
			break;
		}


		case dfMask: // mask ( opacity will be ignored )
		{
			GetBitmapInformationFromObject(bmpobject_clo, true, &bmpwidth, &bmpheight, &bmppitch, &bmpdata);
			if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Not drawable layer type"));
			tTVPBaseBitmap bmpinfo(bmpwidth, bmpheight, bmppitch, bmpdata);
			ImageModified = FillMask(&bmpinfo, destrect, color&0xff ) || ImageModified;
			break;
		}

		case dfProvince: // province ( opacity will be ignored )
			{
				// province
				color = color & 0xff;
				if (color)
				{
					// Independ the bitmap
					GetProvinceBitmapInformationFromObject(bmpobject_clo, true, &provincebmpwidth, &provincebmpheight, &provincebmppitch, &provincebmpdata);
					if (provincebmpdata)
					{
						tTVPBaseBitmap provincebmpinfo(provincebmpwidth, provincebmpheight, provincebmppitch, provincebmpdata, false);
						ImageModified = Fill(&provincebmpinfo, destrect, color&0xff) || ImageModified;
					}
				}
				else
				{
					// Check read only for clear
					GetProvinceBitmapInformationFromObject(bmpobject_clo, false, NULL, NULL, NULL, &provincebmpdata);
					if (provincebmpdata)
					{
						// Independ the bitmap
						GetProvinceBitmapInformationFromObject(bmpobject_clo, true, &provincebmpwidth, &provincebmpheight, &provincebmppitch, &provincebmpdata);
						tTVPBaseBitmap provincebmpinfo(provincebmpwidth, provincebmpheight, provincebmppitch, provincebmpdata, false);
#if 0
						// It doesn't look like we can call DeallocateProvinceImage from script...
						if(destrect.left == 0 && destrect.top == 0 &&
							destrect.right == (tjs_int)ProvinceImage->GetWidth() &&
							destrect.bottom == (tjs_int)ProvinceImage->GetHeight())
						{
							// entire area of the province image will be filled with 0
							DeallocateProvinceImage();
							ImageModified = true;
						}
						else
#endif
						{
							ImageModified = Fill(&provincebmpinfo, destrect, color&0xff) || ImageModified;
						}
					}
				}
				break;
			}
		}

		if (ImageModified)
		{
			tTVPRect ur = destrect;
			UpdateLayerWithLayerObject(bmpobject_clo, &ur, &ImageLeft, &ImageTop);
		}
		return TJS_S_OK;
	}

	static tjs_error TJS_INTF_METHOD copyRect(
		tTJSVariant	*result,
		tjs_int numparams,
		tTJSVariant **param,
		iTJSDispatch2 *objthis)
	{
		if(numparams < 7) return TJS_E_BADPARAMCOUNT;
		
		tTVPRect inrect(*param[3], *param[4], *param[5], *param[6]);
		inrect.right += inrect.left;
		inrect.bottom += inrect.top;

		tTVPRect srcrect = inrect;

		tjs_int dx = *param[0];
		tjs_int dy = *param[1];

		if(objthis == NULL) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTJSVariant bmpobject = tTJSVariant(objthis, objthis);
		tTJSVariantClosure bmpobject_clo = bmpobject.AsObjectClosureNoAddRef();

		tTJSVariantClosure srcbmpobject_clo = param[2]->AsObjectClosureNoAddRef();

		tTVPRect ClipRect;

		// Check bounds first
		GetLayerInformationFromLayerObject(bmpobject_clo, NULL, NULL, &ClipRect, NULL, NULL, NULL, NULL);

		tTVPRect rect;
		if(!ClipDestPointAndSrcRect(ClipRect, dx, dy, rect, srcrect)) return TJS_S_OK; // out of the clipping rect
		// Now get information (this will independ the bitmap)
		tjs_int bmpwidth = 0;
		tjs_int bmpheight = 0;
		tjs_int bmppitch = 0;
		tjs_uint8* bmpdata = NULL;
		GetBitmapInformationFromObject(bmpobject_clo, true, &bmpwidth, &bmpheight, &bmppitch, &bmpdata);
		if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTVPBaseBitmap bmpinfo(bmpwidth, bmpheight, bmppitch, bmpdata);

		tjs_int srcbmpwidth = 0;
		tjs_int srcbmpheight = 0;
		tjs_int srcbmppitch = 0;
		tjs_uint8* srcbmpdata = NULL;
		GetBitmapInformationFromObject(srcbmpobject_clo, false, &srcbmpwidth, &srcbmpheight, &srcbmppitch, &srcbmpdata);
		if(!srcbmpdata) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTVPBaseBitmap srcbmpinfo(srcbmpwidth, srcbmpheight, srcbmppitch, srcbmpdata);

		tTVPDrawFace Face = dfAuto; // (outward) current drawing layer face
		tTVPLayerType DisplayType = ltOpaque; // actual Type
		bool HoldAlpha = false;
		tjs_int ImageLeft = 0;
		tjs_int ImageTop = 0;

		GetLayerInformationFromLayerObject(bmpobject_clo, &Face, &DisplayType, NULL, &HoldAlpha, &ImageLeft, &ImageTop, NULL);

		tTVPDrawFace DrawFace = GetDrawFace(Face, DisplayType); // (actual) current drawing layer face

		bool ImageModified = false;

		switch(DrawFace)
		{
		case dfAlpha:
		case dfAddAlpha:
		{
			if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Not drawable layer type"));
			if(!srcbmpdata) TVPThrowExceptionMessage(TJS_W("Source layer has no image"));
			ImageModified = LayerCopyRect(&bmpinfo, dx, dy, &srcbmpinfo, rect,
				TVP_BB_COPY_MAIN|TVP_BB_COPY_MASK) || ImageModified;
			break;
		}

		case dfOpaque:
		{
			if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Not drawable layer type"));
			if(!srcbmpdata) TVPThrowExceptionMessage(TJS_W("Source layer has no image"));
			ImageModified = LayerCopyRect(&bmpinfo, dx, dy, &srcbmpinfo, rect,
				HoldAlpha?TVP_BB_COPY_MAIN:(TVP_BB_COPY_MAIN|TVP_BB_COPY_MASK)) || ImageModified;
			break;
		}

		case dfMask:
		{
			if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Not drawable layer type"));
			if(!srcbmpdata) TVPThrowExceptionMessage(TJS_W("Source layer has no image"));
			ImageModified = LayerCopyRect(&bmpinfo, dx, dy, &srcbmpinfo, rect,
				TVP_BB_COPY_MASK) || ImageModified;
			break;
		}

		case dfProvince:
		{
			// TODO: province support
#if 0
			if(!provincesrc)
			{
				// source province image is null;
				// fill destination with zero
				if(ProvinceImage) Fill(ProvinceImage, rect, 0);
				ImageModified = true;
			}
			else
			{
				// province image is not created if the image is not needed
				// allocate province image
				if(!ProvinceImage) AllocateProvinceImage();
				// then copy
				ImageModified = LayerCopyRect(ProvinceImage, dx, dy, provincesrc, rect) || ImageModified;
			}
#endif
			break;
		}
		}

		if (ImageModified)
		{
			tTVPRect ur = rect;
			ur.set_offsets(dx, dy);
			UpdateLayerWithLayerObject(bmpobject_clo, &ur, &ImageLeft, &ImageTop);
		}
		return TJS_S_OK;
	}


	static tjs_error TJS_INTF_METHOD operateRect(
		tTJSVariant	*result,
		tjs_int numparams,
		tTJSVariant **param,
		iTJSDispatch2 *objthis)
	{
		if(numparams < 7) return TJS_E_BADPARAMCOUNT;

		tTJSVariantClosure srcbmpobject_clo = param[2]->AsObjectClosureNoAddRef();
		tTVPBlendOperationMode automode = omAlpha;
		tTVPLayerType srcdisplaytype = ltAlpha;

		GetLayerInformationFromLayerObject(srcbmpobject_clo, NULL, &srcdisplaytype, NULL, NULL, NULL, NULL, NULL);
		automode = GetOperationModeFromType(srcdisplaytype);

		tTVPRect inrect(*param[3], *param[4], *param[5], *param[6]);
		inrect.right += inrect.left;
		inrect.bottom += inrect.top;

		tTVPBlendOperationMode mode;
		if(numparams >= 8 && param[7]->Type() != tvtVoid)
			mode = (tTVPBlendOperationMode)(tjs_int)(*param[7]);
		else
			mode = omAuto;

		if(numparams >= 10 && param[9]->Type() != tvtVoid)
		{
			TVPAddLog(TVPFormatMessage(TJS_W("Warring : Method %1 %2th parameter had is ignore. Hold destination alpha parameter is now deprecated."),
				TJS_W("Layer.operateRect"), TJS_W("10")));
		}

		// get correct blend mode if the mode is omAuto
		if(mode == omAuto) mode = automode;

		tjs_int dx = *param[0];
		tjs_int dy = *param[1];

		tTVPRect srcrect = inrect;

		tjs_int opacity = (numparams>=9 && param[8]->Type() != tvtVoid)?(tjs_int)*param[8]:255;

		if(objthis == NULL) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTJSVariant bmpobject = tTJSVariant(objthis, objthis);
		tTJSVariantClosure bmpobject_clo = bmpobject.AsObjectClosureNoAddRef();

		tTVPRect ClipRect;

		// Check bounds first
		GetLayerInformationFromLayerObject(bmpobject_clo, NULL, NULL, &ClipRect, NULL, NULL, NULL, NULL);

		tTVPRect rect;
		if(!ClipDestPointAndSrcRect(ClipRect, dx, dy, rect, srcrect)) return TJS_S_OK; // out of the clipping rect

		// It does not throw an exception in this case perhaps
		if(mode == omAuto) TVPThrowExceptionMessage( TJS_W("Cannot accept omAuto mode") );

		// Now get information (this will independ the bitmap)
		tjs_int bmpwidth = 0;
		tjs_int bmpheight = 0;
		tjs_int bmppitch = 0;
		tjs_uint8* bmpdata = NULL;
		GetBitmapInformationFromObject(bmpobject_clo, true, &bmpwidth, &bmpheight, &bmppitch, &bmpdata);
		if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTVPBaseBitmap bmpinfo(bmpwidth, bmpheight, bmppitch, bmpdata);

		tjs_int srcbmpwidth = 0;
		tjs_int srcbmpheight = 0;
		tjs_int srcbmppitch = 0;
		tjs_uint8* srcbmpdata = NULL;
		GetBitmapInformationFromObject(srcbmpobject_clo, false, &srcbmpwidth, &srcbmpheight, &srcbmppitch, &srcbmpdata);
		if(!srcbmpdata) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTVPBaseBitmap srcbmpinfo(srcbmpwidth, srcbmpheight, srcbmppitch, srcbmpdata);

		tTVPDrawFace Face = dfAuto; // (outward) current drawing layer face
		tTVPLayerType DisplayType = ltOpaque; // actual Type
		bool HoldAlpha = false;
		tjs_int ImageLeft = 0;
		tjs_int ImageTop = 0;

		GetLayerInformationFromLayerObject(bmpobject_clo, &Face, &DisplayType, NULL, &HoldAlpha, &ImageLeft, &ImageTop, NULL);

		tTVPDrawFace DrawFace = GetDrawFace(Face, DisplayType); // (actual) current drawing layer face

		// convert tTVPBlendOperationMode to tTVPBBBltMethod
		tTVPBBBltMethod met;
		if(!GetBltMethodFromOperationModeAndDrawFace(DrawFace, met, mode))
		{
			// unknown blt mode
			TVPThrowExceptionMessage(TJS_W("Not drawble face type %1"), TJS_W("operateRect"));
		}

		bool ImageModified = false;

		if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Not drawable layer type"));
		if(!srcbmpdata) TVPThrowExceptionMessage(TJS_W("Source layer has no image"));

		ImageModified =
			Blt(&bmpinfo, dx, dy, &srcbmpinfo, rect, met, opacity, HoldAlpha) || ImageModified;

		if (ImageModified)
		{
			tTVPRect ur = rect;
			ur.set_offsets(dx, dy);
			UpdateLayerWithLayerObject(bmpobject_clo, &ur, &ImageLeft, &ImageTop);
		}
		return TJS_S_OK;
	}
};

NCB_ATTACH_CLASS(LayerLayerExRect, Layer)
{
	RawCallback("fillRect", &Class::fillRect, 0);
	RawCallback("colorRect", &Class::colorRect, 0);
	RawCallback("copyRect", &Class::copyRect, 0);
	RawCallback("operateRect", &Class::operateRect, 0);
};
