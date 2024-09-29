#include <windows.h>
#include <objidl.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_convert.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_image.h"

#include "iio.h"

#ifdef A5O_CFG_IIO_HAVE_GDIPLUS_LOWERCASE_H
   #include <gdiplus.h>
#else
   #include <GdiPlus.h>
#endif

A5O_DEBUG_CHANNEL("image")

/* Needed with the MinGW w32api-3.15 headers. */
using namespace Gdiplus;

#if !defined(_MSC_VER) && !defined(__uuidof)
#define __uuidof(x) (IID_ ## x)
#endif

static bool gdiplus_inited = false;
static ULONG_PTR gdiplusToken = 0;

/* Source:
 *   http://msdn.microsoft.com/en-us/library/ms533843%28VS.85%29.aspx
 */
static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
   UINT num = 0;          // number of image encoders
   UINT size = 0;         // size of the image encoder array in bytes

   Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

   Gdiplus::GetImageEncodersSize(&num, &size);
   if (size == 0) {
      return -1;  
   }

   pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(al_malloc(size));
   if (pImageCodecInfo == NULL) {
      return -1;  
   }

   GetImageEncoders(num, size, pImageCodecInfo);

   for (UINT j = 0; j < num; ++j) {
      if(wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
         *pClsid = pImageCodecInfo[j].Clsid;
         al_free(pImageCodecInfo);
         return j;
      }    
   }

   al_free(pImageCodecInfo);
   return -1;  
}

/* A wrapper around an already opened A5O_FILE* pointer
 */
class AllegroWindowsStream : public IStream 
{
   long refCount;
   A5O_FILE *fp;

public:
   /* Create a stream from an open file handle */
   AllegroWindowsStream(A5O_FILE *fp) :
      refCount(1),
      fp(fp)
   {
      this->fp = fp;
   }

   virtual ~AllegroWindowsStream()
   {
   }

   /* IUnknown */
   virtual ULONG STDMETHODCALLTYPE AddRef(void)
   {
      return (ULONG) InterlockedIncrement(&refCount);  
   }

   virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject)
   {
      if (iid == __uuidof(IUnknown) || iid == __uuidof(ISequentialStream)
         || iid == __uuidof(IStream)) {
         *ppvObject = static_cast<IStream*>(this);
         AddRef();
         return S_OK;
      }
      else {
         return E_NOINTERFACE;
      }
   }

   virtual ULONG STDMETHODCALLTYPE Release(void)
   {
      ULONG ret = InterlockedDecrement(&refCount);
      if (ret == 0) {
         delete this; 
         return 0; 
      }
      return ret; 
   }

   /* ISequentialStream */
   virtual HRESULT STDMETHODCALLTYPE Read(void *pv, ULONG cb, ULONG *pcbRead)
   {
      size_t read = al_fread(fp, pv, cb);
      if (pcbRead) {
         *pcbRead = read;
      }
      return read == cb ? S_OK : S_FALSE;
   }

   virtual HRESULT STDMETHODCALLTYPE Write(const void *pv, ULONG cb,
      ULONG *pcbWritten)
   {
      size_t written = al_fwrite(fp, pv, cb);
      if (pcbWritten) {
         *pcbWritten = written;
      }
      return written == cb ? S_OK : STG_E_CANTSAVE; 
   }
    
   /* IStream */
   virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin,
      ULARGE_INTEGER *plibNewPosition)
   {
      A5O_SEEK o;
      if (dwOrigin == STREAM_SEEK_SET) {
         o = A5O_SEEK_SET;
      }
      else if (dwOrigin == STREAM_SEEK_CUR) {
         o = A5O_SEEK_CUR; 
      }
      else {
         o = A5O_SEEK_END;
      }
      
      bool ret = al_fseek(fp, dlibMove.QuadPart, o);

      if (plibNewPosition) {
         int64_t pos = al_ftell(fp);
         if (pos == -1) {
            return STG_E_INVALIDFUNCTION;
         }

         plibNewPosition->QuadPart = pos;
      }

      return ret ? S_OK : STG_E_INVALIDFUNCTION;
   }

   /* The GDI+ image I/O methods need to know the file size */
   virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG *pstatstg, DWORD grfStatFlag)
   {		
      (void) grfStatFlag;
      memset(pstatstg, 0, sizeof(*pstatstg));
      pstatstg->type = STGTY_STREAM;
      pstatstg->cbSize.QuadPart = al_fsize(fp); 
      return S_OK;
   }

   /* The following IStream methods aren't needed */
   virtual HRESULT STDMETHODCALLTYPE Clone(IStream **ppstm)
   {
      (void) ppstm;
      return E_NOTIMPL;
   }

   virtual HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags)
   {
      (void) grfCommitFlags;
      return E_NOTIMPL;
   }

   virtual HRESULT STDMETHODCALLTYPE CopyTo (IStream *pstm, ULARGE_INTEGER cb,
      ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
   {
      (void) pstm;
      (void) cb;
      (void) pcbRead;
      (void) pcbWritten;
      return E_NOTIMPL;
   }

   virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER libOffset,
      ULARGE_INTEGER cb, DWORD dwLockType)
   {
      (void) libOffset;
      (void) cb;
      (void) dwLockType;
      return E_NOTIMPL;
   }
    
   virtual HRESULT STDMETHODCALLTYPE Revert()
   {
      return E_NOTIMPL;
   }

   virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize)
   { 
      (void) libNewSize;
      return E_NOTIMPL;
   }
    
   virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset,
      ULARGE_INTEGER cb, DWORD dwLockType)
   {
      (void) libOffset;
      (void) cb;
      (void) dwLockType;
      return E_NOTIMPL;
   }
};

static void load_indexed_data(Gdiplus::Bitmap *gdi_bmp,
   A5O_BITMAP *a_bmp, uint32_t w, uint32_t h)
{
   Gdiplus::BitmapData *gdi_lock = new Gdiplus::BitmapData();
   Gdiplus::Rect rect(0, 0, w, h);

   if (!gdi_bmp->LockBits(&rect, Gdiplus::ImageLockModeRead,
         PixelFormat8bppIndexed, gdi_lock))
   {
      A5O_LOCKED_REGION *a_lock = al_lock_bitmap(a_bmp,
         A5O_PIXEL_FORMAT_SINGLE_CHANNEL_8, A5O_LOCK_WRITEONLY);

      if (a_lock) {
         unsigned char *in = (unsigned char *)gdi_lock->Scan0;
         unsigned char *out = (unsigned char *)a_lock->data;

         if (gdi_lock->Stride == a_lock->pitch) {
            memcpy(out, in, h * gdi_lock->Stride);
         }
         else {
            uint32_t rows = h;
            while (rows--) {
               memcpy(out, in, w);
               in += gdi_lock->Stride;
               out += a_lock->pitch;
            }
         }
         al_unlock_bitmap(a_bmp);
      }

      gdi_bmp->UnlockBits(gdi_lock);
   }

   delete gdi_lock;
}

static void load_non_indexed_data(Gdiplus::Bitmap *gdi_bmp,
   A5O_BITMAP *a_bmp, uint32_t w, uint32_t h, bool premul)
{
   Gdiplus::BitmapData *gdi_lock = new Gdiplus::BitmapData();
   Gdiplus::Rect rect(0, 0, w, h);

   if (!gdi_bmp->LockBits(&rect, Gdiplus::ImageLockModeRead,
         PixelFormat32bppARGB, gdi_lock))
   {
      A5O_LOCKED_REGION *a_lock = al_lock_bitmap(a_bmp,
         A5O_PIXEL_FORMAT_ARGB_8888, A5O_LOCK_WRITEONLY);

      if (a_lock) {
         unsigned char *in = (unsigned char *)gdi_lock->Scan0;
         unsigned char *out = (unsigned char *)a_lock->data;

         if (premul) {
            int in_inc = gdi_lock->Stride - (w*4);
            int out_inc = a_lock->pitch - (w*4);
            for (unsigned int y = 0; y < h; y++) {
               for (unsigned int x = 0; x < w; x++) {
                  unsigned char r, g, b, a;
                  b = *in++;
                  g = *in++;
                  r = *in++;
                  a = *in++;
                  b = b * a / 255;
                  g = g * a / 255;
                  r = r * a / 255;
                  *out++ = b;
                  *out++ = g;
                  *out++ = r;
                  *out++ = a;
               }
               in += in_inc;
               out += out_inc;
            }
         }
         else {
            if (gdi_lock->Stride == a_lock->pitch) {
               memcpy(out, in, h * gdi_lock->Stride);
            }
            else {
               uint32_t rows = h;
               while (rows--) {
                  memcpy(out, in, w * 4);
                  in += gdi_lock->Stride;
                  out += a_lock->pitch;
               }
            }
         }
         al_unlock_bitmap(a_bmp);
      }

      gdi_bmp->UnlockBits(gdi_lock);
   }

   delete gdi_lock;
}

A5O_BITMAP *_al_load_gdiplus_bitmap_f(A5O_FILE *fp, int flags)
{
   AllegroWindowsStream *s = new AllegroWindowsStream(fp);
   if (!s) {
      A5O_ERROR("Unable to create AllegroWindowsStream.\n");
      return NULL;
   }

   A5O_BITMAP *a_bmp = NULL;
   Gdiplus::Bitmap *gdi_bmp = Gdiplus::Bitmap::FromStream(s, false);

   if (gdi_bmp) {
      const uint32_t w = gdi_bmp->GetWidth();
      const uint32_t h = gdi_bmp->GetHeight();
      const PixelFormat pf = gdi_bmp->GetPixelFormat();
      bool premul = !(flags & A5O_NO_PREMULTIPLIED_ALPHA);
      bool keep_index = (flags & A5O_KEEP_INDEX);

      a_bmp = al_create_bitmap(w, h);
      if (a_bmp) {
         if (pf == PixelFormat8bppIndexed && keep_index) {
            load_indexed_data(gdi_bmp, a_bmp, w, h);
         }
         else {
            load_non_indexed_data(gdi_bmp, a_bmp, w, h, premul);
         }
      }
      delete gdi_bmp;
   }

   s->Release();

   return a_bmp;
}

A5O_BITMAP *_al_load_gdiplus_bitmap(const char *filename, int flags)
{
   A5O_BITMAP *bmp = NULL;
   A5O_FILE *fp;
	
   fp = al_fopen(filename, "rb");		
   if (fp) {
      bmp = _al_load_gdiplus_bitmap_f(fp, flags);
      al_fclose(fp);
   }

   return bmp;
}

bool _al_save_gdiplus_bitmap_f(A5O_FILE *fp, const char *ident,
   A5O_BITMAP *a_bmp)
{
   CLSID encoder;
   int encoder_status = -1;

   if (!_al_stricmp(ident, ".bmp")) {
      encoder_status = GetEncoderClsid(L"image/bmp", &encoder);
   }
   else if (!_al_stricmp(ident, ".jpg") || !_al_stricmp(ident, ".jpeg")) {
      encoder_status = GetEncoderClsid(L"image/jpeg", &encoder);
   }
   else if (!_al_stricmp(ident, ".gif")) {
      encoder_status = GetEncoderClsid(L"image/gif", &encoder);
   }
   else if (!_al_stricmp(ident, ".tif") || !_al_stricmp(ident, ".tiff")) {
      encoder_status = GetEncoderClsid(L"image/tiff", &encoder);
   }
   else if (!_al_stricmp(ident, ".png")) {
      encoder_status = GetEncoderClsid(L"image/png", &encoder);
   }

   if (encoder_status == -1) {
      A5O_ERROR("Invalid encoder status.\n");
      return false;    
   }

   AllegroWindowsStream *s = new AllegroWindowsStream(fp);
   if (!s) {
      A5O_ERROR("Couldn't create AllegroWindowsStream.\n");
      return false;
   }

   const int w = al_get_bitmap_width(a_bmp), h = al_get_bitmap_height(a_bmp);
   bool ret = false;

   Gdiplus::Bitmap *gdi_bmp = new Gdiplus::Bitmap(w, h, PixelFormat32bppARGB);

   if (gdi_bmp) {
      Gdiplus::Rect rect(0, 0, w, h);
      Gdiplus::BitmapData *gdi_lock = new Gdiplus::BitmapData();

      if (!gdi_bmp->LockBits(&rect, Gdiplus::ImageLockModeWrite,
            PixelFormat32bppARGB, gdi_lock)) {

         A5O_LOCKED_REGION *a_lock = al_lock_bitmap(
            a_bmp, A5O_PIXEL_FORMAT_ARGB_8888, A5O_LOCK_READONLY);

         if (a_lock) {
            unsigned char *in = (unsigned char *)a_lock->data;
            unsigned char *out = (unsigned char *)gdi_lock->Scan0;
           
            if (gdi_lock->Stride == a_lock->pitch) {
               memcpy(out, in, h * gdi_lock->Stride);
            }
            else {
               uint32_t rows = h;
               while (rows--) {
                  memcpy(out, in, w * 4);
                  in += a_lock->pitch;
                  out += gdi_lock->Stride;
               }
            }

            al_unlock_bitmap(a_bmp);
         }
         gdi_bmp->UnlockBits(gdi_lock);
      }

      ret = (gdi_bmp->Save(s, &encoder, NULL) == 0);

      delete gdi_lock;
      delete gdi_bmp;
   }

   s->Release();

   return ret;
}

bool _al_save_gdiplus_bitmap(const char *filename, A5O_BITMAP *bmp)
{
   A5O_FILE *fp;
   bool ret = false;

   fp = al_fopen(filename, "wb");
   if (fp) {
      A5O_PATH *path = al_create_path(filename);
      if (path) {
         ret = _al_save_gdiplus_bitmap_f(fp, al_get_path_extension(path), bmp);
         al_destroy_path(path);
      }
      al_fclose(fp);
   }

   return ret;
}	

bool _al_save_gdiplus_png_f(A5O_FILE *f, A5O_BITMAP *bmp)
{
   return _al_save_gdiplus_bitmap_f(f, ".png", bmp);
}

bool _al_save_gdiplus_jpg_f(A5O_FILE *f, A5O_BITMAP *bmp)
{
   return _al_save_gdiplus_bitmap_f(f, ".jpg", bmp);
}

bool _al_save_gdiplus_tif_f(A5O_FILE *f, A5O_BITMAP *bmp)
{
   return _al_save_gdiplus_bitmap_f(f, ".tif", bmp);
}

bool _al_save_gdiplus_gif_f(A5O_FILE *f, A5O_BITMAP *bmp)
{
   return _al_save_gdiplus_bitmap_f(f, ".gif", bmp);
}

bool _al_init_gdiplus()
{
   if (!gdiplus_inited) {
      Gdiplus::GdiplusStartupInput gdiplusStartupInput;
      if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL)
            == Gdiplus::Ok) {
         gdiplus_inited = TRUE;
         _al_add_exit_func(_al_shutdown_gdiplus, "_al_shutdown_gdiplus");
      }
   }

   return gdiplus_inited;
}

void _al_shutdown_gdiplus()
{
   if (gdiplus_inited) {
      Gdiplus::GdiplusShutdown(gdiplusToken);
      gdiplus_inited = false;
   }
}

/* vim: set sts=3 sw=3 et: */
