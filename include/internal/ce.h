#ifndef TRIK_V4L2_DSP_FB_INTERNAL_CE_H_
#define TRIK_V4L2_DSP_FB_INTERNAL_CE_H_

#include <stdbool.h>
#include <inttypes.h>

#include <xdc/std.h>
#include <ti/xdais/xdas.h>
#include <ti/sdo/ce/Engine.h>
#include <ti/sdo/ce/osal/Memory.h>
#include <ti/sdo/ce/vidtranscode/vidtranscode.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern bool autoDetectHsv;
extern int autoDetectHue;
extern int autoDetectHueTolerance; 
extern int autoDetectSat;
extern int autoDetectSatTolerance;
extern int autoDetectVal;
extern int autoDetectValTolerance;

typedef struct CodecEngineConfig // what user wants to set
{
  const char* m_serverPath;
  const char* m_codecName;
} CodecEngineConfig;

typedef struct CodecEngine
{
  Engine_Handle m_handle;

  Memory_AllocParams m_allocParams;
  size_t     m_srcBufferSize;
  void*      m_srcBuffer;

  size_t     m_dstBufferSize;
  void*      m_dstBuffer;

  size_t     m_dstInfoBufferSize;
  void*      m_dstInfoBuffer;

  VIDTRANSCODE_Handle m_vidtranscodeHandle;

} CodecEngine;




int codecEngineInit(bool _verbose);
int codecEngineFini();

int codecEngineOpen(CodecEngine* _ce, const CodecEngineConfig* _config);
int codecEngineClose(CodecEngine* _ce);
int codecEngineStart(CodecEngine* _ce, const CodecEngineConfig* _config,
                     size_t _srcWidth, size_t _srcHeight,
                     size_t _srcLineLength, size_t _srcImageSize, uint32_t _srcFormat,
                     size_t _dstWidth, size_t _dstHeight,
                     size_t _dstLineLength, size_t _dstImageSize, uint32_t _dstFormat);
int codecEngineStop(CodecEngine* _ce);

int codecEngineTranscodeFrame(CodecEngine* _ce,
                              const void* _srcFramePtr, size_t _srcFrameSize,
                              void* _dstFramePtr, size_t _dstFrameSize, size_t* _dstFrameUsed,
                              float _detectHueFrom, float _detectHueTo,
                              float _detectSatFrom, float _detectSatTo,
                              float _detectValFrom, float _detectValTo,
                              int* _targetX, int* _targetMass);


int codecEngineReportLoad(CodecEngine* _ce);


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // !TRIK_V4L2_DSP_FB_INTERNAL_CE_H_
