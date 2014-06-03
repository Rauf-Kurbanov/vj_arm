# 
#  Copyright (c) 2012, Texas Instruments Incorporated
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
# 
#

#
#  ======== products.mak ========
#

# Optional, but useful when many/all dependent components are in one folder
#
DEPOT_DSP?=/opt/trik-dsp
DEPOT_SDK?=/opt/trik-sdk
DEPOT_TRIK?=$(CURDIR)/../../

XDC_INSTALL_DIR?=$(DEPOT_DSP)/xdctools_3_24_07_73
CE_INSTALL_DIR?=$(DEPOT_DSP)/codec_engine_3_23_00_07
BIOS_INSTALL_DIR?=$(DEPOT_DSP)/bios_6_35_01_29
FC_INSTALL_DIR?=$(DEPOT_DSP)/framework_components_3_23_03_17
XDAIS_INSTALL_DIR?=$(DEPOT_DSP)/xdais_7_23_00_06
IPC_INSTALL_DIR?=$(DEPOT_DSP)/ipc_1_25_02_12
LINK_INSTALL_DIR?=$(DEPOT_DSP)/syslink_2_21_01_05
OSAL_INSTALL_DIR?=$(DEPOT_DSP)/osal_1_23_00_04
CMEM_INSTALL_DIR?=$(DEPOT_DSP)/linuxutils_3_23_00_01
EDMA3_LLD_INSTALL_DIR?=$(DEPOT_DSP)/edma3_lld_02_11_07_04
EXTRA_PACKAGES_DIR?=$(DEPOT_TRIK)

# Note that GCC targets are 'special' and require more than one var to be set.
#
# The CGTOOLS_* var points at the base of the toolchain.
# The CC_* var points at the gcc binary (e.g. bin/arm-none-linux-gnueabi-gcc)
#
CGTOOLS_V5T = $(DEPOT_SDK)/sysroots/i686-oesdk-linux/usr/bin/armv5te-oe-linux-gnueabi/
CC_V5T      = arm-oe-linux-gnueabi-gcc

# The AR_* var points at the ar binary (e.g. bin/arm-none-linux-gnueabi-ar)
# We can often auto-determine this based on the value of CC_V5T.
# The magic make cmd replaces the "-gcc" at the end of CC_V5T var with "-ar".
AR_V5T      = $(CC_V5T:-gcc=-ar)

# don't modify this, it's derived from the GCC vars above
gnu.targets.arm.GCArmv5T = $(CGTOOLS_V5T);LONGNAME=$(CC_V5T);profiles.release.compileOpts.copts=-O2 -ffunction-sections

# Use this goal to print your product variables.
.show-products::
	@echo "LINK_INSTALL_DIR            = $(LINK_INSTALL_DIR)"
	@echo "CMEM_INSTALL_DIR            = $(CMEM_INSTALL_DIR)"
	@echo "XDAIS_INSTALL_DIR           = $(XDAIS_INSTALL_DIR)"
	@echo "IPC_INSTALL_DIR             = $(IPC_INSTALL_DIR)"
	@echo "EDMA3_LLD_INSTALL_DIR       = $(EDMA3_LLD_INSTALL_DIR)"
	@echo "FC_INSTALL_DIR              = $(FC_INSTALL_DIR)"
	@echo "OSAL_INSTALL_DIR            = $(OSAL_INSTALL_DIR)"
	@echo "CGTOOLS_V5T                 = $(CGTOOLS_V5T)"
	@echo "CC_V5T                      = $(CC_V5T)"
	@echo "AR_V5T                      = $(AR_V5T)"
	@echo "gnu.targets.arm.GCArmv5T    = $(gnu.targets.arm.GCArmv5T)"


# look for other products.mak file to override local settings
ifneq (,$(wildcard $(EXBASE)/../products.mak))
    include $(EXBASE)/../products.mak
else
    ifneq (,$(wildcard $(EXBASE)/../../products.mak))
        include $(EXBASE)/../../products.mak/
    else
        ifneq (,$(wildcard $(EXBASE)/../../../products.mak))
            include $(EXBASE)/../../../products.mak/
        endif
    endif
endif
