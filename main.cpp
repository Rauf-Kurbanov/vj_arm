#include <stdio.h>
#include <stdlib.h>
#include "image.h"
#include "stdio-wrapper.h"
#include "haar.hpp"

#define INPUT_FILENAME "Face.pgm"
#define OUTPUT_FILENAME "Output.pgm"

using namespace std;
//=====================================================

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <getopt.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include <poll.h>
#include <stdint.h>
#include <linux/input.h>
#include <fcntl.h>

#include "internal/ce.h"
#include "internal/fb.h"
#include "internal/v4l2.h"
#include "internal/rc.h"
#include "internal/rover.h"
#if 0 //cyclop wheels
float PK = 0.35;
float DK = -0.007;
float IK = 0.005;
int SPEED = 40;
#endif

/*
//monster wheels
float PK = 0.425;
float DK = -0.0075;
float IK = 0.005;
int SPEED = 60;
*/

//ecilop wheels
float PK = 0.45;
float DK = -0.0073;
float IK = 0.005;
int SPEED = 70;

//auto hsv range detector stuff
bool autoDetectHsv = false;
int	autoDetectHue = 0;
int autoDetectHueTolerance = 0; 
int autoDetectSat = 0;
int autoDetectSatTolerance = 0;  
int autoDetectVal = 0;
int autoDetectValTolerance = 0;

//buttons stuff
struct pollfd fds;
const char* m_path = "/dev/input/by-path/platform-gpio-keys-event";

//rover stuff
int roadWidth = 200;
int inverseMotorCoeff = 1; 
int irrEnable = 0;

static sig_atomic_t s_terminate = false;
static void sigterm_action(int _signal, siginfo_t* _siginfo, void* _context)
{
  (void)_signal;
  (void)_siginfo;
  (void)_context;
  s_terminate = true;
}

static void sigactions_setup()
{
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_sigaction = &sigterm_action;
  action.sa_flags = SA_SIGINFO|SA_RESTART;

  if (sigaction(SIGTERM, &action, NULL) != 0)
    fprintf(stderr, "sigaction(SIGTERM) failed: %d\n", errno);
  if (sigaction(SIGINT,  &action, NULL) != 0)
    fprintf(stderr, "sigaction(SIGINT) failed: %d\n", errno);
}


static bool s_cfgVerbose = false;
static CodecEngineConfig s_cfgCodecEngine = { "dsp_server.xe674", "vidtranscode_cv" };
static V4L2Config s_cfgV4L2Input = { "/dev/video0", 320, 240, V4L2_PIX_FMT_YUYV };
static FBConfig s_cfgFBOutput = { "/dev/fb0" };
static RoverConfig s_cfgRoverOutput = { { 2, 0x48, 0x14, 0x10, 0x64 }, //msp left1
                                        { 2, 0x48, 0x16, 0x10, 0x64 }, //msp left2  //0x16 for scorpio
                                        { 2, 0x48, 0x15, 0x10, 0x64 }, //msp right1
                                        { 2, 0x48, 0x17, 0x10, 0x64 }, //msp right2 //0x17 for scorpio
                                        { "/sys/class/pwm/ecap.0/duty_ns",     2300000, 1600000, 0, 1400000, 700000  }, //up-down m1
                                        { "/sys/class/pwm/ecap.1/duty_ns",     700000,  1400000, 0, 1600000, 2300000 }, //up-down m2
                                        { "/sys/class/pwm/ehrpwm.1:1/duty_ns", 700000,  1400000, 0, 1600000, 2300000 }, //squeeze
                                        { 2, 0x48, 0x20, 0xf0, 0x200}, //IR rangefinder
                                        0, 20, 2000};
static RCConfig s_cfgRCInput = { 4444, false, false, /*27, 7, 75, 25, 70, 30*/0, 179, 20, 20, 20, 20};

static int mainLoop(CodecEngine* _ce, V4L2Input* _v4l2Src, FBOutput* _fbDst, RCInput* _rc, RoverOutput* _rover);


static bool parse_args(int _argc, char* const _argv[])
{
  int opt;
  int longopt;

  static const char* optstring = "vh";
  static const struct option longopts[] =
  {
    { "ce-server",		1,	NULL,	0   }, // 0
    { "ce-codec",		1,	NULL,	0   },
    { "v4l2-path",		1,	NULL,	0   }, // 2
    { "v4l2-width",		1,	NULL,	0   },
    { "v4l2-height",		1,	NULL,	0   },
    { "v4l2-format",		1,	NULL,	0   },
    { "fb-path",		1,	NULL,	0   }, // 6
    { "rover-m1-path",		1,	NULL,	0   }, // 7
    { "rover-m1-back-full",	1,	NULL,	0   },
    { "rover-m1-back-zero",	1,	NULL,	0   },
    { "rover-m1-neutral",	1,	NULL,	0   },
    { "rover-m1-forward-zero",	1,	NULL,	0   },
    { "rover-m1-forward-full",	1,	NULL,	0   },
    { "rover-m2-path",		1,	NULL,	0   }, // 13
    { "rover-m2-back-full",	1,	NULL,	0   },
    { "rover-m2-back-zero",	1,	NULL,	0   },
    { "rover-m2-neutral",	1,	NULL,	0   },
    { "rover-m2-forward-zero",	1,	NULL,	0   },
    { "rover-m2-forward-full",	1,	NULL,	0   },
    { "rover-m3-path",		1,	NULL,	0   }, // 19
    { "rover-m3-back-full",	1,	NULL,	0   },
    { "rover-m3-back-zero",	1,	NULL,	0   },
    { "rover-m3-neutral",	1,	NULL,	0   },
    { "rover-m3-forward-zero",	1,	NULL,	0   },
    { "rover-m3-forward-full",	1,	NULL,	0   },
    { "rover-msp-m1-i2c-bus",	1,	NULL,	0   }, // 25
    { "rover-msp-m1-i2c-dev",	1,	NULL,	0   },
    { "rover-msp-m1-i2c-cmd",	1,	NULL,	0   },
    { "rover-msp-m1-min",	1,	NULL,	0   },
    { "rover-msp-m1-max",	1,	NULL,	0   },
    { "rover-msp-m2-i2c-bus",	1,	NULL,	0   }, // 30
    { "rover-msp-m2-i2c-dev",	1,	NULL,	0   },
    { "rover-msp-m2-i2c-cmd",	1,	NULL,	0   },
    { "rover-msp-m2-min",	1,	NULL,	0   },
    { "rover-msp-m2-max",	1,	NULL,	0   },
    { "rover-msp-m3-i2c-bus",	1,	NULL,	0   }, // 35
    { "rover-msp-m3-i2c-dev",	1,	NULL,	0   },
    { "rover-msp-m3-i2c-cmd",	1,	NULL,	0   },
    { "rover-msp-m3-min",	1,	NULL,	0   },
    { "rover-msp-m3-max",	1,	NULL,	0   },
    { "rover-msp-m4-i2c-bus",	1,	NULL,	0   }, // 40
    { "rover-msp-m4-i2c-dev",	1,	NULL,	0   },
    { "rover-msp-m4-i2c-cmd",	1,	NULL,	0   },
    { "rover-msp-m4-min",	1,	NULL,	0   },
    { "rover-msp-m4-max",	1,	NULL,	0   },
    { "rover-zero-x",		1,	NULL,	0   }, // 45
    { "rover-zero-y",		1,	NULL,	0   },
    { "rover-zero-mass",	1,	NULL,	0   },
    { "rc-port",		1,	NULL,	0   }, // 48
    { "rc-stdin",		1,	NULL,	0   },
    { "rc-manual",		1,	NULL,	0   },

    { "target-sat",		1,	NULL,	0   }, // 51
    { "target-sat-tolerance",	1,	NULL,	0   },
    { "target-val",		1,	NULL,	0   }, // 53
    { "target-val-tolerance",	1,	NULL,	0   },
    { "pk",		1,	NULL,	0   }, 
    { "dk",		1,	NULL,	0   }, //56
    { "ik",		1,	NULL,	0   },
    { "speed",		1,	NULL,	0   }, //58
    { "road-width",	1,	NULL,	0   },
    { "target-hue",		1,	NULL,	0   }, // 60
    { "target-hue-tolerance",	1,	NULL,	0   },
    { "inverse-motors",	1,	NULL,	0   }, // 62
    { "irr-enable",	1,	NULL,	0   }, // 63
    { "verbose",		0,	NULL,	'v' },
    { "help",			0,	NULL,	'h' },
    {NULL, 0, NULL, 0}
   
  };

#warning TODO Detailed MSP motors configuration

  while ((opt = getopt_long(_argc, _argv, optstring, longopts, &longopt)) != -1)
  {
    switch (opt)
    {
      case 'v':	s_cfgVerbose = true;		break;

      case 0:
        switch (longopt)
        {
          case 0: s_cfgCodecEngine.m_serverPath = optarg;	break;
          case 1: s_cfgCodecEngine.m_codecName = optarg;	break;

          case 2: s_cfgV4L2Input.m_path = optarg;		break;
          case 3: s_cfgV4L2Input.m_width = atoi(optarg);	break;
          case 4: s_cfgV4L2Input.m_height = atoi(optarg);	break;
          case 5:
            if      (!strcasecmp(optarg, "rgb888"))	s_cfgV4L2Input.m_format = V4L2_PIX_FMT_RGB24;
            else if (!strcasecmp(optarg, "rgb565"))	s_cfgV4L2Input.m_format = V4L2_PIX_FMT_RGB565;
            else if (!strcasecmp(optarg, "rgb565x"))	s_cfgV4L2Input.m_format = V4L2_PIX_FMT_RGB565X;
            else if (!strcasecmp(optarg, "yuv444"))	s_cfgV4L2Input.m_format = V4L2_PIX_FMT_YUV32;
            else if (!strcasecmp(optarg, "yuv422"))	s_cfgV4L2Input.m_format = V4L2_PIX_FMT_YUYV;
            else
            {
              fprintf(stderr, "Unknown v4l2 format '%s'\n"
                              "Known formats: rgb888, rgb565, rgb565x, yuv444, yuv422\n",
                      optarg);
              return false;
            }
            break;

          case 6: s_cfgFBOutput.m_path = optarg;		break;

          case 7:  s_cfgRoverOutput.m_motor1.m_path = optarg;			break;
          case 8:  s_cfgRoverOutput.m_motor1.m_powerBackFull = atoi(optarg);	break;
          case 9:  s_cfgRoverOutput.m_motor1.m_powerBackZero = atoi(optarg);	break;
          case 10: s_cfgRoverOutput.m_motor1.m_powerNeutral = atoi(optarg);	break;
          case 11: s_cfgRoverOutput.m_motor1.m_powerForwardZero = atoi(optarg);	break;
          case 12: s_cfgRoverOutput.m_motor1.m_powerForwardFull = atoi(optarg);	break;

          case 13: s_cfgRoverOutput.m_motor2.m_path = optarg;			break;
          case 14: s_cfgRoverOutput.m_motor2.m_powerBackFull = atoi(optarg);	break;
          case 15: s_cfgRoverOutput.m_motor2.m_powerBackZero = atoi(optarg);	break;
          case 16: s_cfgRoverOutput.m_motor2.m_powerNeutral = atoi(optarg);	break;
          case 17: s_cfgRoverOutput.m_motor2.m_powerForwardZero = atoi(optarg);	break;
          case 18: s_cfgRoverOutput.m_motor2.m_powerForwardFull = atoi(optarg);	break;

          case 19: s_cfgRoverOutput.m_motor3.m_path = optarg;			break;
          case 20: s_cfgRoverOutput.m_motor3.m_powerBackFull = atoi(optarg);	break;
          case 21: s_cfgRoverOutput.m_motor3.m_powerBackZero = atoi(optarg);	break;
          case 22: s_cfgRoverOutput.m_motor3.m_powerNeutral = atoi(optarg);	break;
          case 23: s_cfgRoverOutput.m_motor3.m_powerForwardZero = atoi(optarg);	break;
          case 24: s_cfgRoverOutput.m_motor3.m_powerForwardFull = atoi(optarg);	break;

          case 25: s_cfgRoverOutput.m_motorMsp1.m_mspI2CBusId    = atoi(optarg);	break;
          case 26: s_cfgRoverOutput.m_motorMsp1.m_mspI2CDeviceId = atoi(optarg);	break;
          case 27: s_cfgRoverOutput.m_motorMsp1.m_mspI2CMotorCmd = atoi(optarg);	break;
          case 28: s_cfgRoverOutput.m_motorMsp1.m_powerMin       = atoi(optarg);	break;
          case 29: s_cfgRoverOutput.m_motorMsp1.m_powerMax       = atoi(optarg);	break;

          case 30: s_cfgRoverOutput.m_motorMsp2.m_mspI2CBusId    = atoi(optarg);	break;
          case 31: s_cfgRoverOutput.m_motorMsp2.m_mspI2CDeviceId = atoi(optarg);	break;
          case 32: s_cfgRoverOutput.m_motorMsp2.m_mspI2CMotorCmd = atoi(optarg);	break;
          case 33: s_cfgRoverOutput.m_motorMsp2.m_powerMin       = atoi(optarg);	break;
          case 34: s_cfgRoverOutput.m_motorMsp2.m_powerMax       = atoi(optarg);	break;

          case 35: s_cfgRoverOutput.m_motorMsp3.m_mspI2CBusId    = atoi(optarg);	break;
          case 36: s_cfgRoverOutput.m_motorMsp3.m_mspI2CDeviceId = atoi(optarg);	break;
          case 37: s_cfgRoverOutput.m_motorMsp3.m_mspI2CMotorCmd = atoi(optarg);	break;
          case 38: s_cfgRoverOutput.m_motorMsp3.m_powerMin       = atoi(optarg);	break;
          case 39: s_cfgRoverOutput.m_motorMsp3.m_powerMax       = atoi(optarg);	break;

          case 40: s_cfgRoverOutput.m_motorMsp4.m_mspI2CBusId    = atoi(optarg);	break;
          case 41: s_cfgRoverOutput.m_motorMsp4.m_mspI2CDeviceId = atoi(optarg);	break;
          case 42: s_cfgRoverOutput.m_motorMsp4.m_mspI2CMotorCmd = atoi(optarg);	break;
          case 43: s_cfgRoverOutput.m_motorMsp4.m_powerMin       = atoi(optarg);	break;
          case 44: s_cfgRoverOutput.m_motorMsp4.m_powerMax       = atoi(optarg);	break;

          case 45: s_cfgRoverOutput.m_zeroX    = atoi(optarg);	break;
          case 46: s_cfgRoverOutput.m_zeroY    = atoi(optarg);	break;
          case 47: s_cfgRoverOutput.m_zeroMass = atoi(optarg);	break;

          case 48: s_cfgRCInput.m_port = atoi(optarg);				break;
          case 49: s_cfgRCInput.m_stdin = atoi(optarg);				break;
          case 50: s_cfgRCInput.m_manualMode = atoi(optarg);			break;
          case 51: s_cfgRCInput.m_autoTargetDetectSat = atof(optarg);		break;
          case 52: s_cfgRCInput.m_autoTargetDetectSatTolerance = atof(optarg);	break;
          case 53: s_cfgRCInput.m_autoTargetDetectVal = atof(optarg);		break;
          case 54: s_cfgRCInput.m_autoTargetDetectValTolerance = atof(optarg);	break;
          case 55: PK = atof(optarg);		break;
          case 56: DK = atof(optarg);	break;
          case 57: IK = atof(optarg);		break;
          case 58: SPEED = atof(optarg);	break;
          case 59: roadWidth = atof(optarg);	break;
          case 60: s_cfgRCInput.m_autoTargetDetectHue = atof(optarg);		break;
          case 61: s_cfgRCInput.m_autoTargetDetectHueTolerance = atof(optarg);	break;
          case 62: inverseMotorCoeff = atof(optarg) == 0 ? 1 : -1;	break;
          case 63: irrEnable = atoi(optarg);	break;
          default:
            return false;
        }
        break;

      case 'h':
      default:
        return false;
    }
  }

  return true;
}



int main(int _argc, char* const _argv[])
{
  int res = 0;
  int exit_code = EX_OK;

  sigactions_setup();

  if ((fds.fd = open(m_path, O_RDONLY)) < 0)
  {
    fprintf(stderr, "Open rover pause button failed.\n");

	  return 1;
  } 

  fds.events = POLLIN;

  if (!parse_args(_argc, _argv))
  {
    fprintf(stderr, "Usage:\n"
                    "    %s <opts>\n"
                    " where opts are:\n"
                    "   --ce-server    <dsp-server-name>\n"
                    "   --ce-codec     <dsp-codec-name>\n"
                    "   --v4l2-path    <input-device-path>\n"
                    "   --v4l2-width   <input-width>\n"
                    "   --v4l2-height  <input-height>\n"
                    "   --v4l2-format  <input-pixel-format>\n"
                    "   --fb-path      <output-device-path>\n"
                    "   --rover-mN-path         <rover-motorN-path>\n"
                    "   --rover-mN-back-full    <rover-motorN-full-back-power>\n"
                    "   --rover-mN-back-zero    <rover-motorN-slow-back-power>\n"
                    "   --rover-mN-neutral      <rover-motorN-neutral-power>\n"
                    "   --rover-mN-forward-zero <rover-motorN-slow-forward-power>\n"
                    "   --rover-mN-forward-full <rover-motorN-full-forward-power>\n"
                    "   --rover-msp-mN-i2c-bus  <rover-msp-motorN-i2c-bus-id>\n"
                    "   --rover-msp-mN-i2c-dev  <rover-msp-motorN-i2c-device-id>\n"
                    "   --rover-msp-mN-i2c-cmd  <rover-msp-motorN-i2c-command>\n"
                    "   --rover-msp-mN-min      <rover-msp-motorN-min-power>\n"
                    "   --rover-msp-mN-max      <rover-msp-motorN-max-power>\n"
                    "   --rover-zero-x          <rover-center-X>\n"
                    "   --rover-zero-y          <rover-center-Y>\n"
                    "   --rover-zero-mass       <rover-center-mass>\n"
                    "   --rc-port               <remote-control-port>\n"
                    "   --rc-stdin              <remote-control-via-stdin 0/1>\n"
                    "   --rc-manual             <remote-control-manual-mode 0/1>\n"
                    "   --target-hue            <target-hue>\n"
                    "   --target-hue-tolerance  <target-hue-tolerance>\n"
                    "   --target-sat            <target-saturation>\n"
                    "   --target-sat-tolerance  <target-saturation-tolerance>\n"
                    "   --target-val            <target-value>\n"
                    "   --target-val-tolerance  <target-value-tolerance>\n"
                    "   --pk                    <pk>\n" 
                    "   --dk                    <dk>\n" 
                    "   --ik                    <ik>\n" 
                    "   --speed                 <speed>\n" 
                    "   --road-width            <road-width>\n"      
                    "   --inverse-motors        <inverse-motors 0/1>\n"      
                    "   --irr-enable            <ir-rangefinder-enbale 0/1>\n"      
                    "   --verbose\n"
                    "   --help\n",
            _argv[0]);
    exit_code = EX_USAGE;
    goto exit;
  }


  if ((res = codecEngineInit(s_cfgVerbose)) != 0)
  {
    fprintf(stderr, "codecEngineInit() failed: %d\n", res);
    exit_code = EX_PROTOCOL;
    goto exit;
  }

  if ((res = v4l2InputInit(s_cfgVerbose)) != 0)
  {
    fprintf(stderr, "v4l2InputInit() failed: %d\n", res);
    exit_code = EX_SOFTWARE;
    goto exit_ce_fini;
  }

  if ((res = fbOutputInit(s_cfgVerbose)) != 0)
  {
    fprintf(stderr, "fbOutputInit() failed: %d\n", res);
    exit_code = EX_SOFTWARE;
    goto exit_v4l2_fini;
  }

  if ((res = rcInputInit(s_cfgVerbose)) != 0)
  {
    fprintf(stderr, "rcInputInit() failed: %d\n", res);
    exit_code = EX_SOFTWARE;
    goto exit_fb_fini;
  }

  if ((res = roverOutputInit(s_cfgVerbose)) != 0)
  {
    fprintf(stderr, "roverOutputInit() failed: %d\n", res);
    exit_code = EX_SOFTWARE;
    goto exit_rc_fini;
  }


  CodecEngine codecEngine;
  memset(&codecEngine, 0, sizeof(codecEngine));
  if ((res = codecEngineOpen(&codecEngine, &s_cfgCodecEngine)) != 0)
  {
    fprintf(stderr, "codecEngineOpen() failed: %d\n", res);
    exit_code = EX_PROTOCOL;
    goto exit_rover_fini;
  }

  V4L2Input v4l2Src;
  memset(&v4l2Src, 0, sizeof(v4l2Src));
  v4l2Src.m_fd = -1;
  if ((res = v4l2InputOpen(&v4l2Src, &s_cfgV4L2Input)) != 0)
  {
    fprintf(stderr, "v4l2InputOpen() failed: %d\n", res);
    exit_code = EX_NOINPUT;
    goto exit_ce_close;
  }

  FBOutput fbDst;
  memset(&fbDst, 0, sizeof(fbDst));
  fbDst.m_fd = -1;
  if ((res = fbOutputOpen(&fbDst, &s_cfgFBOutput)) != 0)
  {
    fprintf(stderr, "fbOutputOpen() failed: %d\n", res);
    exit_code = EX_IOERR;
    goto exit_v4l2_close;
  }

  RCInput rc;
  memset(&rc, 0, sizeof(rc));
  rc.m_stdinFd = -1;
  rc.m_serverFd = -1;
  rc.m_connectionFd = -1;
  if ((res = rcInputOpen(&rc, &s_cfgRCInput)) != 0)
  {
    fprintf(stderr, "rcInputOpen() failed: %d\n", res);
    exit_code = EX_IOERR;
    goto exit_fb_close;
  }

  RoverOutput rover;
  memset(&rover, 0, sizeof(rover));
  rover.m_motorMsp1.m_i2cBusFd = -1;
  rover.m_motorMsp2.m_i2cBusFd = -1;
  rover.m_motorMsp3.m_i2cBusFd = -1;
  rover.m_motorMsp4.m_i2cBusFd = -1;
  rover.m_motor1.m_fd = -1;
  rover.m_motor2.m_fd = -1;
  rover.m_motor3.m_fd = -1;
  if ((res = roverOutputOpen(&rover, &s_cfgRoverOutput)) != 0)
  {
    fprintf(stderr, "roverOutputOpen() failed: %d\n", res);
    exit_code = EX_IOERR;
    goto exit_rc_close;
  }


  size_t srcWidth, srcHeight, srcLineLength, srcImageSize;
  size_t dstWidth, dstHeight, dstLineLength, dstImageSize;
  uint32_t srcFormat, dstFormat;
  if ((res = v4l2InputGetFormat(&v4l2Src, &srcWidth, &srcHeight, &srcLineLength, &srcImageSize, &srcFormat)) != 0)
  {
    fprintf(stderr, "v4l2InputGetFormat() failed: %d\n", res);
    exit_code = EX_PROTOCOL;
    goto exit_rover_close;
  }
  if ((res = fbOutputGetFormat(&fbDst, &dstWidth, &dstHeight, &dstLineLength, &dstImageSize, &dstFormat)) != 0)
  {
    fprintf(stderr, "fbOutputGetFormat() failed: %d\n", res);
    exit_code = EX_PROTOCOL;
    goto exit_rover_close;
  }
  if ((res = codecEngineStart(&codecEngine, &s_cfgCodecEngine,
                              srcWidth, srcHeight, srcLineLength, srcImageSize, srcFormat,
                              dstWidth, dstHeight, dstLineLength, dstImageSize, dstFormat)) != 0)
  {
    fprintf(stderr, "codecEngineStart() failed: %d\n", res);
    exit_code = EX_PROTOCOL;
    goto exit_rover_close;
  }

  if ((res = v4l2InputStart(&v4l2Src)) != 0)
  {
    fprintf(stderr, "v4l2InputStart() failed: %d\n", res);
    exit_code = EX_NOINPUT;
    goto exit_ce_stop;
  }

  if ((res = fbOutputStart(&fbDst)) != 0)
  {
    fprintf(stderr, "fbOutputStart() failed: %d\n", res);
    exit_code = EX_IOERR;
    goto exit_v4l2_stop;
  }

  if ((res = rcInputStart(&rc)) != 0)
  {
    fprintf(stderr, "rcInputStart() failed: %d\n", res);
    exit_code = EX_IOERR;
    goto exit_fb_stop;
  }

  if ((res = roverOutputStart(&rover)) != 0)
  {
    fprintf(stderr, "roverOutputStart() failed: %d\n", res);
    exit_code = EX_IOERR;
    goto exit_rc_stop;
  }


  printf("Entering main loop\n");
  struct timespec lastReportTime;
  if (clock_gettime(CLOCK_MONOTONIC, &lastReportTime) != 0)
    fprintf(stderr, "clock_gettime(CLOCK_MONOTONIC) failed: %d\n", errno);

  while (!s_terminate)
  {
    struct timespec currentTime;
    if (clock_gettime(CLOCK_MONOTONIC, &currentTime) != 0)
      fprintf(stderr, "clock_gettime(CLOCK_MONOTONIC) failed: %d\n", errno);
    else if (currentTime.tv_sec > lastReportTime.tv_sec+5) // approx check that ~5sec elapsed
    {
      unsigned long long elapsedMs = currentTime.tv_sec - lastReportTime.tv_sec;
      elapsedMs *= 1000;
      elapsedMs += currentTime.tv_nsec/1000000;
      elapsedMs -= lastReportTime.tv_nsec/1000000;

      lastReportTime = currentTime;
      if ((res = codecEngineReportLoad(&codecEngine)) != 0)
        fprintf(stderr, "codecEngineReportLoad() failed: %d\n", res);

      if ((res = v4l2InputReportFPS(&v4l2Src, elapsedMs)) != 0)
        fprintf(stderr, "v4l2InputReportFPS() failed: %d\n", res);
    }


    if ((res = mainLoop(&codecEngine, &v4l2Src, &fbDst, &rc, &rover)) != 0)
    {
      fprintf(stderr, "mainLoop() failed: %d\n", res);
      exit_code = EX_SOFTWARE;
      break;
    }
  }
  printf("Left main loop\n");


//exit_rover_stop:
  if ((res = roverOutputStop(&rover)) != 0)
    fprintf(stderr, "roverOutputStop() failed: %d\n", res);

exit_rc_stop:
  if ((res = rcInputStop(&rc)) != 0)
    fprintf(stderr, "rcInputStop() failed: %d\n", res);

exit_fb_stop:
  if ((res = fbOutputStop(&fbDst)) != 0)
    fprintf(stderr, "fbOutputStop() failed: %d\n", res);

exit_v4l2_stop:
  if ((res = v4l2InputStop(&v4l2Src)) != 0)
    fprintf(stderr, "v4l2InputStop() failed: %d\n", res);

exit_ce_stop:
  if ((res = codecEngineStop(&codecEngine)) != 0)
    fprintf(stderr, "codecEngineStop() failed: %d\n", res);


exit_rover_close:
  if ((res = roverOutputClose(&rover)) != 0)
    fprintf(stderr, "roverOutputClose() failed: %d\n", res);

exit_rc_close:
  if ((res = rcInputClose(&rc)) != 0)
    fprintf(stderr, "rcInputClose() failed: %d\n", res);

exit_fb_close:
  if ((res = fbOutputClose(&fbDst)) != 0)
    fprintf(stderr, "fbOutputClose() failed: %d\n", res);

exit_v4l2_close:
  if ((res = v4l2InputClose(&v4l2Src)) != 0)
    fprintf(stderr, "v4l2InputClose() failed: %d\n", res);

exit_ce_close:
  if ((res = codecEngineClose(&codecEngine)) != 0)
    fprintf(stderr, "codecEngineClose() failed: %d\n", res);


exit_rover_fini:
  if ((res = roverOutputFini()) != 0)
    fprintf(stderr, "roverOutputFini() failed: %d\n", res);

exit_rc_fini:
  if ((res = rcInputFini()) != 0)
    fprintf(stderr, "rcInputFini() failed: %d\n", res);

exit_fb_fini:
  if ((res = fbOutputFini()) != 0)
    fprintf(stderr, "fbOutputFini() failed: %d\n", res);

exit_v4l2_fini:
  if ((res = v4l2InputFini()) != 0)
    fprintf(stderr, "v4l2InputFini() failed: %d\n", res);

exit_ce_fini:
  if ((res = codecEngineFini()) != 0)
    fprintf(stderr, "codecEngineFini() failed: %d\n", res);


exit:
  return exit_code;

  //===========================================================================
  int flag;

  int mode = 1;
  int i;

  /* detection parameters */
  float scaleFactor = 1.2;
  int minNeighbours = 1;


  printf("-- entering main function --\r\n");

  printf("-- loading image --\r\n");

  MyImage imageObj;
  MyImage *image = &imageObj;

  flag = readPgm((char *)"Face.pgm", image);
  if (flag == -1)
  {
      printf( "Unable to open input image\n");
      return 1;
  }

  printf("-- loading cascade classifier --\r\n");

  myCascade cascadeObj;
  myCascade *cascade = &cascadeObj;
  MySize minSize = {20, 20};
  MySize maxSize = {0, 0};

  /* classifier properties */
  cascade->n_stages=25;
  cascade->total_nodes=2913;
  cascade->orig_window_size.height = 24;
  cascade->orig_window_size.width = 24;


  readTextClassifier();

  std::vector<MyRect> result;

  printf("-- detecting faces --\r\n");

  result = detectObjects(image, minSize, maxSize, cascade, scaleFactor, minNeighbours);

  for(i = 0; i < result.size(); i++ )
  {
      MyRect r = result[i];
      drawRectangle(image, r);
  }

  printf("-- saving output --\r\n");
  flag = writePgm((char *)OUTPUT_FILENAME, image);

  printf("-- image saved --\r\n");

  /* delete image and free classifier */
  releaseTextClassifier();
  freeImage(image);

  return 0;
  //===========================================================================
}




static int mainLoopV4L2Frame(CodecEngine* _ce, V4L2Input* _v4l2Src, FBOutput* _fbDst, RCInput* _rc, RoverOutput* _rover)
{
  int res;

  const void* frameSrcPtr;
  size_t frameSrcSize;
  size_t frameSrcIndex;
  if ((res = v4l2InputGetFrame(_v4l2Src, &frameSrcPtr, &frameSrcSize, &frameSrcIndex)) != 0)
  {
    fprintf(stderr, "v4l2InputGetFrame() failed: %d\n", res);
    return res;
  }

  void* frameDstPtr;
  size_t frameDstSize;
  if ((res = fbOutputGetFrame(_fbDst, &frameDstPtr, &frameDstSize)) != 0)
  {
    fprintf(stderr, "fbOutputGetFrame() failed: %d\n", res);
    return res;
  }

  float detectHueFrom;
  float detectHueTo;
  float detectSatFrom;
  float detectSatTo;
  float detectValFrom;
  float detectValTo;

  if ((res = rcInputGetAutoTargetDetect(_rc, &detectHueFrom, &detectHueTo, &detectSatFrom, &detectSatTo, &detectValFrom, &detectValTo)) != 0)
  {
    fprintf(stderr, "rcInputGetAutoTargetDetect() failed: %d\n", res);
    return res;
  }

  size_t frameDstUsed = frameDstSize;
  int targetY; //tmp
  int targetX;
  int targetMass;

  if ((res = codecEngineTranscodeFrame(_ce,
                                       frameSrcPtr, frameSrcSize,
                                       frameDstPtr, frameDstSize, &frameDstUsed,
                                       detectHueFrom, detectHueTo,
                                       detectSatFrom, detectSatTo,
                                       detectValFrom, detectValTo,
                                       &targetX, &targetMass)) != 0)
  {
    fprintf(stderr, "codecEngineTranscodeFrame(%p[%zu] -> %p[%zu]) failed: %d\n",
            frameSrcPtr, frameSrcSize, frameDstPtr, frameDstSize, res);
    return res;
  }

  if(autoDetectHsv)
  {
    _rc->m_autoTargetDetectHue = autoDetectHue;
    _rc->m_autoTargetDetectHueTolerance = autoDetectHueTolerance;
    _rc->m_autoTargetDetectSat = autoDetectSat;
    _rc->m_autoTargetDetectSatTolerance = autoDetectSatTolerance;
    _rc->m_autoTargetDetectVal = autoDetectVal;
    _rc->m_autoTargetDetectValTolerance = autoDetectValTolerance;

    fprintf(stderr, "hsv   : %d %d %d\n", autoDetectHue, autoDetectSat, autoDetectVal);
    fprintf(stderr, "hsvTol: %d %d %d\n", autoDetectHueTolerance, autoDetectSatTolerance, autoDetectValTolerance);
  }
  
  //fprintf log
  //fprintf(stderr, "Target detected at %d x %d\n", targetX, targetMass);

  if (s_cfgVerbose)
  {
    fprintf(stderr, "Transcoded frame %p[%zu] -> %p[%zu/%zu]\n",
            frameSrcPtr, frameSrcSize, frameDstPtr, frameDstSize, frameDstUsed);
    if (targetMass > 0)
      fprintf(stderr, "Target detected at %d x %d @ %d\n", targetX, targetY, targetMass);
  }

  if ((res = fbOutputPutFrame(_fbDst)) != 0)
  {
    fprintf(stderr, "fbOutputPutFrame() failed: %d\n", res);
    return res;
  }

  if ((res = v4l2InputPutFrame(_v4l2Src, frameSrcIndex)) != 0)
  {
    fprintf(stderr, "v4l2InputPutFrame() failed: %d\n", res);
    return res;
  }


  if (!rcInputIsManualMode(_rc))
  {
    if ((res = roverOutputControlAuto(_rover, targetX, targetMass)) != 0)
    {
      fprintf(stderr, "roverOutputControlAuto() failed: %d\n", res);
      return res;
    }
  }

  return 0;
}

static int mainLoopRCManualModeUpdate(CodecEngine* _ce, V4L2Input* _v4l2Src, FBOutput* _fbDst, RCInput* _rc, RoverOutput* _rover)
{
  int res;

  if (rcInputIsManualMode(_rc))
  {
    int ctrlChasisLR;
    int ctrlChasisFB;
    int ctrlHand;
    int ctrlArm;

    if ((res = rcInputGetManualCommand(_rc, &ctrlChasisLR, &ctrlChasisFB, &ctrlHand, &ctrlArm)) != 0)
    {
      fprintf(stderr, "rcInputGetManualCommand() failed: %d\n", res);
      return res;
    }
    if ((res = roverOutputControlManual(_rover, ctrlChasisLR, ctrlChasisFB, ctrlHand, ctrlArm)) != 0)
    {
      fprintf(stderr, "roverOutputControlManual() failed: %d\n", res);
      return res;
    }
  }

  return 0;
}

static int mainLoopRCStdin(CodecEngine* _ce, V4L2Input* _v4l2Src, FBOutput* _fbDst, RCInput* _rc, RoverOutput* _rover)
{
  fprintf(stderr, "in mainLoopRCStdin\n");

  int res;

  if ((res = rcInputReadStdin(_rc)) != 0)
  {
    fprintf(stderr, "rcInputReadStdin() failed: %d\n", res);
    return res;
  }

  return mainLoopRCManualModeUpdate(_ce, _v4l2Src, _fbDst, _rc, _rover);
}

static int mainLoopRCServer(CodecEngine* _ce, V4L2Input* _v4l2Src, FBOutput* _fbDst, RCInput* _rc, RoverOutput* _rover)
{
  int res;

  if ((res = rcInputAcceptConnection(_rc)) != 0)
  {
    fprintf(stderr, "rcInputAcceptConnection() failed: %d\n", res);
    return res;
  }
  return 0;
}

static int mainLoopRCConnection(CodecEngine* _ce, V4L2Input* _v4l2Src, FBOutput* _fbDst, RCInput* _rc, RoverOutput* _rover)
{
  int res;

  if ((res = rcInputReadConnection(_rc)) != 0)
  {
    fprintf(stderr, "rcInputReadConnection() failed: %d\n", res);
    return res;
  }

  return mainLoopRCManualModeUpdate(_ce, _v4l2Src, _fbDst, _rc, _rover);
}


static int mainLoop(CodecEngine* _ce, V4L2Input* _v4l2Src, FBOutput* _fbDst, RCInput* _rc, RoverOutput* _rover)
{
  int res;

  int maxFd = 0;
  fd_set fdsIn;
  FD_ZERO(&fdsIn);

  FD_SET(_v4l2Src->m_fd, &fdsIn);
  if (maxFd < _v4l2Src->m_fd)
    maxFd = _v4l2Src->m_fd;

  FD_SET(_rc->m_serverFd, &fdsIn);
  if (maxFd < _rc->m_serverFd)
    maxFd = _rc->m_serverFd;

  if (_rc->m_stdinFd != -1)
  {
    FD_SET(_rc->m_stdinFd, &fdsIn);
    if (maxFd < _rc->m_stdinFd)
      maxFd = _rc->m_stdinFd;
  }

  if (_rc->m_connectionFd != -1)
  {
    FD_SET(_rc->m_connectionFd, &fdsIn);
    if (maxFd < _rc->m_connectionFd)
      maxFd = _rc->m_connectionFd;
  }

  struct input_event mev[1];

//  autoDetectHsv = false;
//  if (poll(&fds, 1, 0) > 0) //check for was pause button pressed
//  {
//    int res;
//    if ((res = read(fds.fd, mev, sizeof(struct input_event))) != 1)
//    {
//      if (mev[0].type == 1 && mev[0].code == 62 && mev[0].value == 1)
//        roverSetPause(_rover);
//      if (mev[0].type == 1 && mev[0].code == 64 && mev[0].value == 1)
//        autoDetectHsv = true;
//    }
//  }


  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;

  if ((res = select(maxFd+1, &fdsIn, NULL, NULL, &timeout)) < 0)
  {
    res = errno;
    fprintf(stderr, "select() failed: %d\n", res);
    return res;
  }

  bool hasAnything = false;
  if (FD_ISSET(_rc->m_serverFd, &fdsIn))
  {
    if ((res = mainLoopRCServer(_ce, _v4l2Src, _fbDst, _rc, _rover)) != 0)
    {
      fprintf(stderr, "mainLoopRCServer() failed: %d\n", res);
      return res;
    }
    hasAnything = true;
  }

  if (_rc->m_stdinFd != -1 && FD_ISSET(_rc->m_stdinFd, &fdsIn))
  {
    fprintf(stderr, "mainLoopRCStdin1\n");
    if ((res = mainLoopRCStdin(_ce, _v4l2Src, _fbDst, _rc, _rover)) != 0)
    {
      fprintf(stderr, "mainLoopRCStdin() failed: %d\n", res);
      return res;
    }
    fprintf(stderr, "mainLoopRCStdin2\n");
    hasAnything = true;
  }

  if (_rc->m_connectionFd != -1 && FD_ISSET(_rc->m_connectionFd, &fdsIn))
  {
    if ((res = mainLoopRCConnection(_ce, _v4l2Src, _fbDst, _rc, _rover)) != 0)
    {
      fprintf(stderr, "mainLoopRCConnection() failed: %d\n", res);
      return res;
    }
    hasAnything = true;
  }

  // don't go for video frame if did any other action
  if (!hasAnything && FD_ISSET(_v4l2Src->m_fd, &fdsIn))
  {
    if ((res = mainLoopV4L2Frame(_ce, _v4l2Src, _fbDst, _rc, _rover)) != 0)
    {
      fprintf(stderr, "mainLoopV4L2Frame() failed: %d\n", res);
      return res;
    }
    hasAnything = true;
  }
  
  return 0;
}


