
//---------------------------------------------------------------------------
static tjs_int GetAdaptiveThreadNum(tjs_int pixelNum, float factor)
{
  if (pixelNum >= factor * 500)
    return TVPGetThreadNum();
  else
    return 1;
}
//---------------------------------------------------------------------------
#define RET_VOID
#define BOUND_CHECK(x) \
{ \
	tjs_int i; \
	if(rect.left < 0) rect.left = 0; \
	if(rect.top < 0) rect.top = 0; \
	if(rect.right > (i=destbmp->GetWidth())) rect.right = i; \
	if(rect.bottom > (i=destbmp->GetHeight())) rect.bottom = i; \
	if(rect.right - rect.left <= 0 || rect.bottom - rect.top <= 0) \
		return x; \
}
