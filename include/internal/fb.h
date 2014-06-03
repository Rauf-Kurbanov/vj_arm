#ifndef TRIK_V4L2_DSP_FB_INTERNAL_FB_H_
#define TRIK_V4L2_DSP_FB_INTERNAL_FB_H_

#include <stdbool.h>
#include <inttypes.h>

#include <linux/fb.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


typedef struct FBConfig // what user wants to set
{
  const char* m_path;
} FBConfig;

typedef struct FBOutput
{
  int                      m_fd;
  struct fb_fix_screeninfo m_fbFixInfo;
  struct fb_var_screeninfo m_fbVarInfo;
  void*                    m_fbPtr;
  size_t                   m_fbSize;
} FBOutput;


int fbOutputInit(bool _verbose);
int fbOutputFini();

int fbOutputOpen(FBOutput* _fb, const FBConfig* _config);
int fbOutputClose(FBOutput* _fb);
int fbOutputStart(FBOutput* _fb);
int fbOutputStop(FBOutput* _fb);
int fbOutputGetFrame(FBOutput* _fb, void** _framePtr, size_t* _frameSize);
int fbOutputPutFrame(FBOutput* _fb);

int fbOutputGetFormat(FBOutput* _fb,
                      size_t* _width, size_t* _height,
                      size_t* _lineLength, size_t* _imageSize, uint32_t* _format);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // !TRIK_V4L2_DSP_FB_INTERNAL_FB_H_
