#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <xdc/std.h>
#include <xdc/runtime/Diags.h>
#include <ti/sdo/ce/CERuntime.h>
#include <ti/sdo/ce/Server.h>

#include <linux/videodev2.h>

//#include <trik/vidtranscode_resample/trik_vidtranscode_resample.h>
#include <trik/vidtranscode_cv/trik_vidtranscode_cv.h>

#include "internal/ce.h"

#warning Check BUFALIGN usage!
#ifndef BUFALIGN
#define BUFALIGN 128
#endif

#define ALIGN_UP(v, a) ((((v)+(a)-1)/(a))*(a))

static int do_memoryAlloc(CodecEngine* _ce, size_t _srcBufferSize, size_t _dstBufferSize)
{
  memset(&_ce->m_allocParams, 0, sizeof(_ce->m_allocParams));
  _ce->m_allocParams.type = Memory_CONTIGPOOL;
  _ce->m_allocParams.flags = Memory_CACHED;
  _ce->m_allocParams.align = BUFALIGN;
  _ce->m_allocParams.seg = 0;

  _ce->m_srcBufferSize = ALIGN_UP(_srcBufferSize, BUFALIGN);
  if ((_ce->m_srcBuffer = Memory_alloc(_ce->m_srcBufferSize, &_ce->m_allocParams)) == NULL)
  {
    fprintf(stderr, "Memory_alloc(src, %zu) failed\n", _ce->m_srcBufferSize);
    _ce->m_srcBufferSize = 0;
    return ENOMEM;
  }

  _ce->m_dstBufferSize = ALIGN_UP(_dstBufferSize, BUFALIGN);
  if ((_ce->m_dstBuffer = Memory_alloc(_ce->m_dstBufferSize, &_ce->m_allocParams)) == NULL)
  {
    fprintf(stderr, "Memory_alloc(dst, %zu) failed\n", _ce->m_dstBufferSize);
    _ce->m_dstBufferSize = 0;

    Memory_free(_ce->m_srcBuffer, _ce->m_srcBufferSize, &_ce->m_allocParams);
    _ce->m_srcBuffer = NULL;
    _ce->m_srcBufferSize = 0;
    return ENOMEM;
  }

  _ce->m_dstInfoBufferSize = ALIGN_UP(1000, BUFALIGN);
  if ((_ce->m_dstInfoBuffer = Memory_alloc(_ce->m_dstInfoBufferSize, &_ce->m_allocParams)) == NULL)
  {
    fprintf(stderr, "Memory_alloc(dst, %zu) failed\n", _ce->m_dstInfoBufferSize);
    _ce->m_dstInfoBufferSize = 0;

    Memory_free(_ce->m_dstBuffer, _ce->m_dstBufferSize, &_ce->m_allocParams);
    _ce->m_dstBuffer = NULL;
    _ce->m_dstBufferSize = 0;
    Memory_free(_ce->m_srcBuffer, _ce->m_srcBufferSize, &_ce->m_allocParams);
    _ce->m_srcBuffer = NULL;
    _ce->m_srcBufferSize = 0;
    return ENOMEM;
  }

  return 0;
}

static int do_memoryFree(CodecEngine* _ce)
{
  if (_ce->m_dstInfoBuffer != NULL)
  {
    Memory_free(_ce->m_dstInfoBuffer, _ce->m_dstInfoBufferSize, &_ce->m_allocParams);
    _ce->m_dstInfoBuffer = NULL;
    _ce->m_dstInfoBufferSize = 0;
  }

  if (_ce->m_dstBuffer != NULL)
  {
    Memory_free(_ce->m_dstBuffer, _ce->m_dstBufferSize, &_ce->m_allocParams);
    _ce->m_dstBuffer = NULL;
    _ce->m_dstBufferSize = 0;
  }

  if (_ce->m_srcBuffer != NULL)
  {
    Memory_free(_ce->m_srcBuffer, _ce->m_srcBufferSize, &_ce->m_allocParams);
    _ce->m_srcBuffer = NULL;
    _ce->m_srcBufferSize = 0;
  }

  return 0;
}

static XDAS_Int32 do_convertPixelFormat(CodecEngine* _ce, uint32_t _format)
{
  switch (_format)
  {
    case V4L2_PIX_FMT_RGB24:	return TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_RGB888;
    case V4L2_PIX_FMT_RGB565:	return TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_RGB565;
    case V4L2_PIX_FMT_RGB565X:	return TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_RGB565X;
    case V4L2_PIX_FMT_YUV32:	return TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_YUV444;
    case V4L2_PIX_FMT_YUYV:	return TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_YUV422;
    default:
      fprintf(stderr, "Unknown pixel format %c%c%c%c\n",
              _format&0xff, (_format>>8)&0xff, (_format>>16)&0xff, (_format>>24)&0xff);
      return TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_UNKNOWN;
  }
}

static int do_setupCodec(CodecEngine* _ce, const char* _codecName,
                         size_t _srcWidth, size_t _srcHeight,
                         size_t _srcLineLength, size_t _srcImageSize, uint32_t _srcFormat,
                         size_t _dstWidth, size_t _dstHeight,
                         size_t _dstLineLength, size_t _dstImageSize, uint32_t _dstFormat)
{
  if (_codecName == NULL)
    return EINVAL;

#if 1
  fprintf(stderr, "VIDTRANSCODE_control(%c%c%c%c@%zux%zu[%zu] -> %c%c%c%c@%zux%zu[%zu])\n",
                _srcFormat&0xff, 
          (_srcFormat>> 8)&0xff, 
          (_srcFormat>>16)&0xff, 
          (_srcFormat>>24)&0xff,
          _srcWidth, _srcHeight, _srcLineLength,
                _dstFormat&0xff, 
          (_dstFormat>> 8)&0xff, 
          (_dstFormat>>16)&0xff, 
          (_dstFormat>>24)&0xff,
          _dstWidth, _dstHeight, _dstLineLength);
#endif

  TRIK_VIDTRANSCODE_CV_Params ceParams;
  memset(&ceParams, 0, sizeof(ceParams));
  ceParams.base.size = sizeof(ceParams);
  ceParams.base.numOutputStreams = 1;
  ceParams.base.formatInput = do_convertPixelFormat(_ce, _srcFormat);
  ceParams.base.formatOutput[0] = do_convertPixelFormat(_ce, _dstFormat);
  ceParams.base.maxHeightInput = _srcHeight;
  ceParams.base.maxWidthInput = _srcWidth;
  ceParams.base.maxHeightOutput[0] = _dstHeight;
  ceParams.base.maxWidthOutput[0] = _dstWidth;
  ceParams.base.dataEndianness = XDM_BYTE;

  char* codec = strdup(_codecName);
  if ((_ce->m_vidtranscodeHandle = VIDTRANSCODE_create(_ce->m_handle, codec, &ceParams.base)) == NULL)
  {
    free(codec);
    fprintf(stderr, "VIDTRANSCODE_create(%s) failed\n", _codecName);
    return EBADRQC;
  }
  free(codec);

  TRIK_VIDTRANSCODE_CV_DynamicParams ceDynamicParams;
  memset(&ceDynamicParams, 0, sizeof(ceDynamicParams));
  ceDynamicParams.base.size = sizeof(ceDynamicParams);
  ceDynamicParams.base.keepInputResolutionFlag[0] = XDAS_FALSE;
  ceDynamicParams.base.outputHeight[0] = _dstHeight;
  ceDynamicParams.base.outputWidth[0] = _dstWidth;
  ceDynamicParams.base.keepInputFrameRateFlag[0] = XDAS_TRUE;
  ceDynamicParams.inputHeight = _srcHeight;
  ceDynamicParams.inputWidth = _srcWidth;
  ceDynamicParams.inputLineLength = _srcLineLength;
  ceDynamicParams.outputLineLength[0] = _dstLineLength;

  IVIDTRANSCODE_Status ceStatus;
  memset(&ceStatus, 0, sizeof(ceStatus));
  ceStatus.size = sizeof(ceStatus);
  XDAS_Int32 controlResult = VIDTRANSCODE_control(_ce->m_vidtranscodeHandle, XDM_SETPARAMS, &ceDynamicParams.base, &ceStatus);
  if (controlResult != IVIDTRANSCODE_EOK)
  {
    fprintf(stderr, "VIDTRANSCODE_control() failed: %"PRIi32"/%"PRIi32"\n", controlResult, ceStatus.extendedError);
    return EBADRQC;
  }

  return 0;
}

static int do_releaseCodec(CodecEngine* _ce)
{
  if (_ce->m_vidtranscodeHandle != NULL)
    VIDTRANSCODE_delete(_ce->m_vidtranscodeHandle);
  _ce->m_vidtranscodeHandle = NULL;

  return 0;
}

static int do_transcodeFrame(CodecEngine* _ce,
                             const void* _srcFramePtr, size_t _srcFrameSize,
                             void* _dstFramePtr, size_t _dstFrameSize, size_t* _dstFrameUsed,
                             float _detectHueFrom, float _detectHueTo,
                             float _detectSatFrom, float _detectSatTo,
                             float _detectValFrom, float _detectValTo,
                             int* _targetX, int* _targetMass)
{
  if (_ce->m_srcBuffer == NULL || _ce->m_dstBuffer == NULL/* || _ce->m_dstInfoBuffer == NULL*/)
    return ENOTCONN;
  if (_srcFramePtr == NULL || _dstFramePtr == NULL)
    return EINVAL;
  if (_srcFrameSize > _ce->m_srcBufferSize || _dstFrameSize > _ce->m_dstBufferSize)
    return ENOSPC;

  TRIK_VIDTRANSCODE_CV_InArgs tcInArgs;
  memset(&tcInArgs, 0, sizeof(tcInArgs));
  tcInArgs.base.size = sizeof(tcInArgs);
  tcInArgs.base.numBytes = _srcFrameSize;
  tcInArgs.base.inputID = 1; // must be non-zero, otherwise caching issues appear

  tcInArgs.alg.detectHueFrom = _detectHueFrom;
  tcInArgs.alg.detectHueTo   = _detectHueTo;
  tcInArgs.alg.detectSatFrom = _detectSatFrom;
  tcInArgs.alg.detectSatTo   = _detectSatTo;
  tcInArgs.alg.detectValFrom = _detectValFrom;
  tcInArgs.alg.detectValTo   = _detectValTo;
  tcInArgs.alg.autoDetectHsv = autoDetectHsv;

  TRIK_VIDTRANSCODE_CV_OutArgs tcOutArgs;
  memset(&tcOutArgs,    0, sizeof(tcOutArgs));
  tcOutArgs.base.size = sizeof(tcOutArgs);

  XDM1_BufDesc tcInBufDesc;
  memset(&tcInBufDesc,  0, sizeof(tcInBufDesc));
  tcInBufDesc.numBufs = 1;
  tcInBufDesc.descs[0].buf = _ce->m_srcBuffer;
  tcInBufDesc.descs[0].bufSize = _srcFrameSize;

  XDM_BufDesc tcOutBufDesc;
  memset(&tcOutBufDesc, 0, sizeof(tcOutBufDesc));
  XDAS_Int8* tcOutBufDesc_bufs[1];
  XDAS_Int32 tcOutBufDesc_bufSizes[1];
  tcOutBufDesc.numBufs = 1;
  tcOutBufDesc.bufs = tcOutBufDesc_bufs;
  tcOutBufDesc.bufs[0] = _ce->m_dstBuffer;
  tcOutBufDesc.bufSizes = tcOutBufDesc_bufSizes;
  tcOutBufDesc.bufSizes[0] = _dstFrameSize;
//  tcOutBufDesc.bufSizes[1] = _ce->m_dstInfoBufferSize;
//  tcOutBufDesc.bufs[1] = _ce->m_dstInfoBuffer;
  memcpy(_ce->m_srcBuffer, _srcFramePtr, _srcFrameSize);

  Memory_cacheWbInv(_ce->m_srcBuffer, _ce->m_srcBufferSize); // invalidate and flush *whole* cache, not only written portion, just in case
  Memory_cacheInv(_ce->m_dstBuffer, _ce->m_dstBufferSize); // invalidate *whole* cache, not only expected portion, just in case
/*  Memory_cacheInv(_ce->m_dstInfoBuffer, _ce->m_dstInfoBufferSize);*/

  XDAS_Int32 processResult = VIDTRANSCODE_process(_ce->m_vidtranscodeHandle, &tcInBufDesc, &tcOutBufDesc, &tcInArgs.base, &tcOutArgs);
  if (processResult != IVIDTRANSCODE_EOK)
  {
    fprintf(stderr, "VIDTRANSCODE_process(%zu -> %zu) failed: %"PRIi32"/%"PRIi32"\n",
            _srcFrameSize, _dstFrameSize, processResult, tcOutArgs.base.extendedError);
    return EILSEQ;
  }

#if 0 // It seems so far that this call is not required at all
  if (XDM_ISACCESSMODE_WRITE(tcOutArgs.encodedBuf[0].accessMask))
    Memory_cacheWb(_ce->m_dstBuffer, _ce->m_dstBufferSize);
  if (XDM_ISACCESSMODE_WRITE(tcOutArgs.encodedBuf[1].accessMask))
    Memory_cacheWb(_ce->m_dstInfoBuffer, _ce->m_dstInfoBufferSize);
#endif

  if (tcOutArgs.base.encodedBuf[0].bufSize < 0)
  {
    *_dstFrameUsed = 0;
    fprintf(stderr, "VIDTRANSCODE_process(%zu -> %zu) returned negative buffer size\n",
            _srcFrameSize, _dstFrameSize);
  }
  else if ((size_t)(tcOutArgs.base.encodedBuf[0].bufSize) > _dstFrameSize)
  {
    *_dstFrameUsed = _dstFrameSize;
    fprintf(stderr, "VIDTRANSCODE_process(%zu -> %zu) returned too large buffer %zu, truncated\n",
            _srcFrameSize, _dstFrameSize, *_dstFrameUsed);
  }
  else
    *_dstFrameUsed = tcOutArgs.base.encodedBuf[0].bufSize;


  memcpy(_dstFramePtr, _ce->m_dstBuffer, *_dstFrameUsed);

  *_targetX    = tcOutArgs.alg.targetX;
//  _targetY    = tcOutArgs.alg.targetY;
  *_targetMass = (int)(tcOutArgs.alg.targetSize);
  autoDetectHue = tcOutArgs.alg.detectHue;
  autoDetectHueTolerance = tcOutArgs.alg.detectHueTolerance;
  autoDetectSat = tcOutArgs.alg.detectSat;
  autoDetectSatTolerance = tcOutArgs.alg.detectSatTolerance;
  autoDetectVal = tcOutArgs.alg.detectVal;
  autoDetectValTolerance = tcOutArgs.alg.detectValTolerance;

/*
  fprintf(stderr, "hsv: (%d, %d) (%d, %d) (%d, %d)\n", autoDetectHueFrom, autoDetectHueTo, 
                                                       autoDetectSatFrom, autoDetectSatTo,
                                                       autoDetectValFrom, autoDetectValTo);
*/

  return 0;
}

static int do_reportLoad(CodecEngine* _ce)
{
  Server_Handle ceServerHandle = Engine_getServer(_ce->m_handle);
  if (ceServerHandle == NULL)
  {
    fprintf(stderr, "Engine_getServer() failed\n");
    return ENOTCONN;
  }

  fprintf(stderr, "DSP load %d%%\n", (int)Server_getCpuLoad(ceServerHandle));

  Int sNumSegs;
  Server_Status sStatus = Server_getNumMemSegs(ceServerHandle, &sNumSegs);
  if (sStatus != Server_EOK)
    fprintf(stderr, "Server_getNumMemSegs() failed: %d\n", (int)sStatus);
  else
  {
    Int sSegIdx;
    for (sSegIdx = 0; sSegIdx < sNumSegs; ++sSegIdx)
    {
      Server_MemStat sMemStat;
      sStatus = Server_getMemStat(ceServerHandle, sSegIdx, &sMemStat);
      if (sStatus != Server_EOK)
      {
        fprintf(stderr, "Server_getMemStat() failed: %d\n", (int)sStatus);
        break;
      }

      fprintf(stderr, "DSP memory %#08x..%#08x, used %10u: %s\n",
              (unsigned)sMemStat.base, (unsigned)(sMemStat.base+sMemStat.size),
              (unsigned)sMemStat.used, sMemStat.name);
    }
  }

  return 0;
}




int codecEngineInit(bool _verbose)
{
  CERuntime_init(); /* init Codec Engine */

  if (_verbose)
  {
    Diags_setMask("xdc.runtime.Main+EX1234567");
    Diags_setMask(Engine_MODNAME"+EX1234567");
  }

  return 0;
}

int codecEngineFini()
{
  return 0;
}


int codecEngineOpen(CodecEngine* _ce, const CodecEngineConfig* _config)
{
  if (_ce == NULL || _config == NULL)
    return EINVAL;

  if (_ce->m_handle != NULL)
    return EALREADY;

  Engine_Error ceError;
  Engine_Desc desc;
  Engine_initDesc(&desc);
  desc.name = "dsp-server";
  desc.remoteName = strdup(_config->m_serverPath);
  errno = 0;

  ceError = Engine_add(&desc);
  if (ceError != Engine_EOK)
  {
    free(desc.remoteName);
    fprintf(stderr, "Engine_add(%s) failed: %d/%"PRIi32"\n", _config->m_serverPath, errno, ceError);
    return ENOMEM;
  }
  free(desc.remoteName);

  if ((_ce->m_handle = Engine_open("dsp-server", NULL, &ceError)) == NULL)
  {
    fprintf(stderr, "Engine_open(%s) failed: %d/%"PRIi32"\n", _config->m_serverPath, errno, ceError);
    return ENOMEM;
  }

  return 0;
}

int codecEngineClose(CodecEngine* _ce)
{
  if (_ce == NULL)
    return EINVAL;

  if (_ce->m_handle == NULL)
    return EALREADY;

  Engine_close(_ce->m_handle);
  _ce->m_handle = NULL;

  return 0;
}


int codecEngineStart(CodecEngine* _ce, const CodecEngineConfig* _config,
                     size_t _srcWidth, size_t _srcHeight,
                     size_t _srcLineLength, size_t _srcImageSize, uint32_t _srcFormat,
                     size_t _dstWidth, size_t _dstHeight,
                     size_t _dstLineLength, size_t _dstImageSize, uint32_t _dstFormat)
{
  int res;

  if (_ce == NULL || _config == NULL)
    return EINVAL;

  if (_ce->m_handle == NULL)
    return ENOTCONN;

  if ((res = do_memoryAlloc(_ce, _srcImageSize, _dstImageSize)) != 0)
    return res;

  if ((res = do_setupCodec(_ce, _config->m_codecName,
                           _srcWidth, _srcHeight, _srcLineLength, _srcImageSize, _srcFormat,
                           _dstWidth, _dstHeight, _dstLineLength, _dstImageSize, _dstFormat)) != 0)
  {
    do_memoryFree(_ce);
    return res;
  }

  return 0;
}

int codecEngineStop(CodecEngine* _ce)
{
  if (_ce == NULL)
    return EINVAL;

  if (_ce->m_handle == NULL)
    return ENOTCONN;

  do_releaseCodec(_ce);
  do_memoryFree(_ce);

  return 0;
}

int codecEngineTranscodeFrame(CodecEngine* _ce,
                              const void* _srcFramePtr, size_t _srcFrameSize,
                              void* _dstFramePtr, size_t _dstFrameSize, size_t* _dstFrameUsed,
                              float _detectHueFrom, float _detectHueTo,
                              float _detectSatFrom, float _detectSatTo,
                              float _detectValFrom, float _detectValTo,
                              int* _targetX, int* _targetMass)
{
  if (_ce == NULL || _targetX == NULL || _targetMass == NULL)
    return EINVAL;

  if (_ce->m_handle == NULL)
    return ENOTCONN;

  return do_transcodeFrame(_ce,
                           _srcFramePtr, _srcFrameSize,
                           _dstFramePtr, _dstFrameSize, _dstFrameUsed,
                           _detectHueFrom, _detectHueTo,
                           _detectSatFrom, _detectSatTo,
                           _detectValFrom, _detectValTo,
                           _targetX, _targetMass);
}

int codecEngineReportLoad(CodecEngine* _ce)
{
  if (_ce == NULL)
    return EINVAL;

  if (_ce->m_handle == NULL)
    return ENOTCONN;

  return do_reportLoad(_ce);
}


