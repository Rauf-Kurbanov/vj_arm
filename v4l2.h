#ifndef TRIK_V4L2_DSP_FB_INTERNAL_V4L2_H_
#define TRIK_V4L2_DSP_FB_INTERNAL_V4L2_H_

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <linux/videodev2.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


typedef struct V4L2Config // what user wants to set
{
  const char* m_path;
  size_t      m_width;
  size_t      m_height;
  uint32_t    m_format;
} V4L2Config;

typedef struct V4L2Input
{
  int                    m_fd;
  unsigned               m_frameCounter;
  struct v4l2_format     m_imageFormat;

  void*                  m_buffers[2];
  size_t                 m_bufferSize[2];
} V4L2Input;



int v4l2InputInit(bool _verbose);
int v4l2InputFini();

int v4l2InputOpen(V4L2Input* _v4l2, const V4L2Config* _config);
int v4l2InputClose(V4L2Input* _v4l2);
int v4l2InputStart(V4L2Input* _v4l2);
int v4l2InputStop(V4L2Input* _v4l2);
int v4l2InputGetFrame(V4L2Input* _v4l2, const void** _framePtr, size_t* _frameSize, size_t* _frameIndex);
int v4l2InputPutFrame(V4L2Input* _v4l2, size_t _frameIndex);

int v4l2InputGetFormat(V4L2Input* _v4l2,
                       size_t* _width, size_t* _height,
                       size_t* _lineLength, size_t* _imageSize, uint32_t* _format);

int v4l2InputReportFPS(V4L2Input* _v4l2, unsigned long long _ms);


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // !TRIK_V4L2_DSP_FB_INTERNAL_V4L2_H_
