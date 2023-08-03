
#define TVP_BB_COPY_MAIN 1
#define TVP_BB_COPY_MASK 2

extern bool LayerCopyRect(tTVPBaseBitmap *destbmp, tjs_int x, tjs_int y, const tTVPBaseBitmap *ref,
		tTVPRect refrect, tjs_int plane);
