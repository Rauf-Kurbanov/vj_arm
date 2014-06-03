# --COPYRIGHT--,BSD
#  Copyright (c) $(CPYYEAR), Texas Instruments Incorporated
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#  *  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#
#  *  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  *  Neither the name of Texas Instruments Incorporated nor the names of
#     its contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
#  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
#  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# --/COPYRIGHT--
#
#  ======== OMAP3530/ex01_universal_copy_remote_undefined/makefile ========
#  GNUmake-based makefile
#


#  ======== standard macros ========
# use these on Linux
CP      ?= cp
ECHO    ?= echo
MKDIR   ?= mkdir
RM      ?= rm -f
RMDIR   ?= rm -rf

include products.mak

#
#  ======== define all the repositories needed by this example ========
#
XDCTOOLS_REPO   ?= $(XDC_INSTALL_DIR)/packages
CE_REPO         ?= $(CE_INSTALL_DIR)/packages
SUFFIX          ?= v5T

#
#  Make sure CE_INSTALL_DIR was set properly. By using "-include" to include
#  xdcpaths.mak, no error message will be given if xdcpaths.mak is not found,
#  and in that case, the user must have set the environment variables, for
#  example, on the command line.
# 
ERRMSG = which is invalid (could not find file "$(TEST_FILE)"). Set this in <codec engine examples>/xdcpaths.mak! If you intended to use xdcpaths.mak, also please make sure that EXAMPLES_ROOTDIR is set correctly in this makefile. See the build documentation to correct this error.
TEST_FILE := $(CE_REPO)/ti/sdo/ce/package.xdc
ifeq ($(wildcard $(TEST_FILE)),)
    $(error CE_INSTALL_DIR is set to "$(CE_INSTALL_DIR)", $(ERRMSG))
endif

REPO_PATH = \
	$(XDCTOOLS_REPO) \
	$(CE_REPO) \
	$(LINK_INSTALL_DIR)/packages \
	$(XDAIS_INSTALL_DIR)/packages \
	$(FC_INSTALL_DIR)/packages \
	$(CMEM_INSTALL_DIR)/packages \
	$(EXAMPLES_ROOTDIR) \
	$(EXTRA_PACKAGES_DIR)


PROFILE = release
#PROFILE = debug

#
#  ======== common defines ========
#
ALL.defs =
ALL.incs = -I. $(addprefix -I,$(REPO_PATH))
ALL.libs = $(addprefix -L,$(REPO_PATH))

EXTLIBS_$(PROFILE) = \
	osal/$(PROFILE)/lib/osal.a \
	osal/$(PROFILE)/lib/cstubs.a

ifneq (clean,$(MAKECMDGOALS))
ifneq (,$(PROFILE))
ifeq (,$(wildcard bin/$(PROFILE)))
    $(shell $(MKDIR) -p bin/$(PROFILE))
endif
endif
endif

.PHONY: nothing

#
#  ======== toolchain macros ========
#  Note: You do not need to set CGTOOLS_V5T and CC_V5T if they have been
#  set in xdcpaths.mak.
#
CGTOOLS_V5T ?= __your_CGTOOLS_V5T__
CC_V5T ?= __your_CC_V5T__
CC = $(CGTOOLS_V5T)/$(CC_V5T)

CCPROFILE.debug = -g -D_DEBUG_=1 
CCPROFILE.release = -O2
CFLAGS = -fPIC -Wall -fno-strict-aliasing $(CCPROFILE.$(PROFILE)) \
    -I. -I./include $(addprefix -I,$(REPO_PATH))
CPPFLAGS = -Dxdc_target_name__=GCArmv5T \
    -Dxdc_target_types__=gnu/targets/arm/std.h

LNKPROFILE.debug = -g
LNKPROFILE.release =
LDFLAGS = $(LNKPROFILE.$(PROFILE))
LDLIBS = -lpthread -lrt -ldl -lv4l2

#
#  $(CE_REPO)/ti/sdo/ce/utils/rtcfg/
#      rtcfg.c
#      rtcfg_fcinit.c	
#      rtcfg_local_config.c
#
#      The source files in $(CE_REPO)/ti/sdo/ce/utils/rtcfg are run-time
#      configuration files.  These files do not need to be modified to run this
#      example. If you want to modify the configuration in any of these files,
#      copy it to this directory and modify as needed. The vpath variable
#      below sets the search path for these source files.
#
#  $(CURDIR)/
#      main.c
#
#      The file main.c contains the main application source code.
#
#      The file, rtcfg_remote_config.c, contains application and device
#      specific configuration.  This file is located in
#      <CE_INSTALL_DIR>/packages/ti/sdo/ce/utils/rtcfg, and is located by
#      make through the vpath variable set below.  If needed, this file can
#      be copied to this directroy and modified.
#
#haar.cpp  image.c  main.cpp  rectangles.cpp  stdio-wrapper.c
APP.srcs = \
	image.c \
   	stdio-wrapper.c  \
	v4l2.c \
	fb.c \
	ce.c \
	rc.c \
	rover.c

APP.srccpp = \
	main.cpp \
	rectangles.cpp \
	haar.cpp

APP.objs = $(addprefix bin/$(PROFILE)/, \
    $(patsubst %.c,%.o$(SUFFIX),$(APP.srcs)))

APP.objscpp = $(addprefix bin/$(PROFILE)/, \
    $(patsubst %.cpp,%.o$(SUFFIX),$(APP.srccpp)))	


#
#  The vpath variable lets us maintain some of the same sources for both XDC
#  build and gmake build, and use default run-time configuration files.
#
vpath %.c $(CE_REPO)/ti/sdo/ce/utils/rtcfg

#
#  ======== linker command files ========
#
#  Set up path to find linker command files, and which linker command
#  files will be used (eg, debug or release).
#

#  Path for linker command files. We use several linker command files,
#  keeping those containing Codec Engine and Framework Components libraries
#  in a common area where they can be used by multiple apps. The linker
#  command files in this directory just contain the codec libraries needed
#  by this app.
#
#  The common linker command files, in ti/sdo/ce/utils/rtcfg/linux/OMAPL138,
#  shouldn't need modification, but in the chance that they did, they could be
#  copied to this directory and modified as needed. The vpath will cause
#  any modified linker command files to be found first.
#
vpath %.cmd $(CE_REPO)/ti/sdo/ce/utils/rtcfg/linux/OMAPL138

# Codec Engine libraries
CE_LINKER_FILE = ce_remote_$(PROFILE).cmd

# Framework Compenents libraries
FC_LINKER_FILE = fc_$(PROFILE).cmd

PROGRAM = vj_demo

#
#  ======== make commands ========
#
all:
	@$(ECHO) "#"
	@$(ECHO) "# Making $@ ..."
	@$(MAKE) bin/$(PROFILE)/$(PROGRAM).xv5T


#
#  ======== build rules ========
#
bin/$(PROFILE)/$(PROGRAM).xv5T: $(APP.objs) $(APP.objscpp) $(EXTLIBS_$(PROFILE)) $(CE_LINKER_FILE) $(FC_LINKER_FILE)
	@$(ECHO) "#"
	@$(ECHO) "# Making $@ ..."
	$(CXX) -o $@ $^ $(EXTLIBS_$(PROFILE)) $(LDLIBS) $(ALL.defs) $(ALL.libs)

.SECONDARY:
bin/$(PROFILE)/%.ov5T: %.h
bin/$(PROFILE)/%.ov5T: %.c
	@$(ECHO) "#"
	@$(ECHO) "# Making $@ ..."
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

bin/$(PROFILE)/%.ov5T: %.cpp
	@$(ECHO) "#"
	@$(ECHO) "# Making $@ ..."
	$(CXX) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

#
# OSAL
#
ifneq (clean,$(MAKECMDGOALS))
ifneq (,$(PROFILE))
ifeq (,$(wildcard $(PROFILE)))
    $(shell $(MKDIR) -p osal)
    $(shell $(MKDIR) -p osal/$(PROFILE))
endif
endif
endif

CROSS_COMPILE = $(subst -gcc,-,$(CGTOOLS_V5T)/$(CC_V5T))

osal/debug/lib/osal.a:
	@echo "Making osal..."
	$(MAKE) -C osal/debug \
            -f $(OSAL_INSTALL_DIR)/packages/linuxdist/build/Makefile \
            OSAL_INSTALL_DIR=$(OSAL_INSTALL_DIR) \
            CROSS_COMPILE=$(CROSS_COMPILE) \
            PRECONFIG=$(OSAL_INSTALL_DIR)/packages/linuxdist/preconfig/development \
            CFLAGS=-g

osal/debug/lib/cstubs.a:
	$(MAKE) -C osal/debug \
            -f $(OSAL_INSTALL_DIR)/packages/linuxdist/cstubs/Makefile \
            OSAL_INSTALL_DIR=$(OSAL_INSTALL_DIR) \
            CROSS_COMPILE=$(CROSS_COMPILE) \
            CFLAGS=-g

osal/release/lib/osal.a:
	@echo "Making osal..."
	$(MAKE) -C osal/release \
            -f $(OSAL_INSTALL_DIR)/packages/linuxdist/build/Makefile \
            OSAL_INSTALL_DIR=$(OSAL_INSTALL_DIR) \
            CROSS_COMPILE=$(CROSS_COMPILE) \
            PRECONFIG=$(OSAL_INSTALL_DIR)/packages/linuxdist/preconfig/production \
            CFLAGS=-O2

osal/release/lib/cstubs.a:
	$(MAKE) -C osal/release \
            -f $(OSAL_INSTALL_DIR)/packages/linuxdist/cstubs/Makefile \
            OSAL_INSTALL_DIR=$(OSAL_INSTALL_DIR) \
            CROSS_COMPILE=$(CROSS_COMPILE) \
            CFLAGS=-O2

#
#  ======== clean ========
#
clean::
	$(RMDIR) bin
	$(RMDIR) osal

ifneq (clean,$(MAKECMDGOALS))
ifneq (,$(PROFILE))
ifeq (,$(wildcard bin/$(PROFILE)))
    $(shell $(MKDIR) -p bin/$(PROFILE))
endif
endif
endif

