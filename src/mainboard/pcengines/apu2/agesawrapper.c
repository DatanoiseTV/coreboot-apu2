/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdint.h>
#include <string.h>
#include <config.h>
#include <cpu/x86/mtrr.h>
#include "agesawrapper.h"
#include <northbridge/amd/pi/BiosCallOuts.h>
#include "cpuRegisters.h"
#include "cpuCacheInit.h"
#include "cpuApicUtilities.h"
#include "cpuEarlyInit.h"
#include "cpuLateInit.h"
#include "Dispatcher.h"
#include "cpuCacheInit.h"
#include "amdlib.h"
#include "PlatformGnbPcieComplex.h"
#include "Filecode.h"
#include "heapManager.h"
#include "FchPlatform.h"
#include "Fch.h"
#include <cpu/amd/pi/s3_resume.h>
#include <arch/io.h>
#include <device/device.h>
#include "hudson.h"
#include <lib.h> /* memory test prototypes */

VOID FchInitS3LateRestore (IN FCH_DATA_BLOCK *FchDataPtr);
VOID FchInitS3EarlyRestore (IN FCH_DATA_BLOCK *FchDataPtr);

PSO_ENTRY OurPlatformMemoryConfiguration[] = {
  //
  // The following macros are supported (use comma to separate macros):
  //
  // MEMCLK_DIS_MAP(SocketID, ChannelID, MemClkDisBit0CSMap,..., MemClkDisBit7CSMap)
  //      The MemClk pins are identified based on BKDG definition of Fn2x88[MemClkDis] bitmap.
  //      AGESA will base on this value to disable unused MemClk to save power.
  //      Example:
  //      BKDG definition of Fn2x88[MemClkDis] bitmap for AM3 package is like below:
  //           Bit AM3/S1g3 pin name
  //           0   M[B,A]_CLK_H/L[0]
  //           1   M[B,A]_CLK_H/L[1]
  //           2   M[B,A]_CLK_H/L[2]
  //           3   M[B,A]_CLK_H/L[3]
  //           4   M[B,A]_CLK_H/L[4]
  //           5   M[B,A]_CLK_H/L[5]
  //           6   M[B,A]_CLK_H/L[6]
  //           7   M[B,A]_CLK_H/L[7]
  //      And platform has the following routing:
  //           CS0   M[B,A]_CLK_H/L[4]
  //           CS1   M[B,A]_CLK_H/L[2]
  //           CS2   M[B,A]_CLK_H/L[3]
  //           CS3   M[B,A]_CLK_H/L[5]
  //      Then platform can specify the following macro:
  //      MEMCLK_DIS_MAP(ANY_SOCKET, ANY_CHANNEL, 0x00, 0x00, 0x02, 0x04, 0x01, 0x08, 0x00, 0x00)
  //
  // CKE_TRI_MAP(SocketID, ChannelID, CKETriBit0CSMap, CKETriBit1CSMap,..., CKETriBit3CSMap)
  //      The CKE pins are identified based on BKDG definition of Fn2x9C_0C[CKETri] bitmap.
  //      AGESA will base on this value to tristate unused CKE to save power.
  //
  // ODT_TRI_MAP(SocketID, ChannelID, ODTTriBit0CSMap,..., ODTTriBit3CSMap)
  //      The ODT pins are identified based on BKDG definition of Fn2x9C_0C[ODTTri] bitmap.
  //      AGESA will base on this value to tristate unused ODT pins to save power.
  //
  // CS_TRI_MAP(SocketID, ChannelID, CSTriBit0CSMap,..., CSTriBit7CSMap)
  //      The Chip select pins are identified based on BKDG definition of Fn2x9C_0C[ChipSelTri] bitmap.
  //      AGESA will base on this value to tristate unused Chip select to save power.
  //
  // NUMBER_OF_DIMMS_SUPPORTED(SocketID, ChannelID, NumberOfDimmSlotsPerChannel)
  //      Specifies the number of DIMMs (DRAM devices) per channel.
  //      Note: The "NumberOfDimmSlotsPerChannel" parameter includes both slotted and soldered-down DIMMs
  //
  // NUMBER_OF_CHIP_SELECTS_SUPPORTED(SocketID, ChannelID, NumberOfChipSelectsPerChannel)
  //      Specifies the number of Chip selects per channel.
  //
  // NUMBER_OF_CHANNELS_SUPPORTED(SocketID, NumberOfChannelsPerSocket)
  //      Specifies the number of channels per socket.
  //
  // OVERRIDE_DDR_BUS_SPEED(SocketID, ChannelID, USER_MEMORY_TIMING_MODE, MEMORY_BUS_SPEED)
  //      Specifies DDR bus speed of channel ChannelID on socket SocketID.
  //
  // DRAM_TECHNOLOGY(SocketID, TECHNOLOGY_TYPE)
  //      Specifies the DRAM technology type of socket SocketID (DDR2, DDR3,...)
  //
  // MOTHER_BOARD_LAYERS (MOTHER_BOARD_LAYER_DESIGN),
  //      Specifies the design of mother board layer (LAYERS_4, LAYERS_6)
  //      Note: If the design is not specified, LAYERS_4 is chosen by default.
  //
  // WRITE_LEVELING_SEED(SocketID, ChannelID, DimmID, Byte0Seed, Byte1Seed, Byte2Seed, Byte3Seed, Byte4Seed, Byte5Seed,
  //      Byte6Seed, Byte7Seed, ByteEccSeed)
  //      Specifies the write leveling seed for a channel of a socket.
  //
  // HW_RXEN_SEED(SocketID, ChannelID, DimmID, Byte0Seed, Byte1Seed, Byte2Seed, Byte3Seed, Byte4Seed, Byte5Seed,
  //      Byte6Seed, Byte7Seed, ByteEccSeed)
  //      Speicifes the HW RXEN training seed for a channel of a socket
  //
  NUMBER_OF_DIMMS_SUPPORTED (ANY_SOCKET, ANY_CHANNEL, 1),       //only one supported
  NUMBER_OF_CHANNELS_SUPPORTED (ANY_SOCKET, 1),
  MOTHER_BOARD_LAYERS (LAYERS_6),
  PSO_END
};

#define FILECODE UNASSIGNED_FILE_FILECODE

#ifndef __PRE_RAM__
/* ACPI table pointers returned by AmdInitLate */
static void *DmiTable    = NULL;
static void *AcpiPstate  = NULL;
static void *AcpiSrat    = NULL;
static void *AcpiSlit    = NULL;

static void *AcpiWheaMce = NULL;
static void *AcpiWheaCmc = NULL;
static void *AcpiAlib    = NULL;
static void *AcpiIvrs    = NULL;
#endif

AGESA_STATUS agesawrapper_amdinitcpuio(void)
{
	AGESA_STATUS                  Status;
	UINT64                        MsrReg;
	UINT32                        PciData;
	PCI_ADDR                      PciAddress;
	AMD_CONFIG_PARAMS             StdHeader;

//	/* Enable legacy video routing: D18F1xF4 VGA Enable */
//	PciAddress.AddressValue = MAKE_SBDFO (0, 0, 0x18, 1, 0xF4);
//	PciData = 1;
//	LibAmdPciWrite(AccessWidth32, PciAddress, &PciData, &StdHeader);

	/* The platform BIOS needs to ensure the memory ranges of AVALON legacy
	 * devices (TPM, HPET, BIOS RAM, Watchdog Timer, I/O APIC and ACPI) are
	 * set to non-posted regions.
	 */
	PciAddress.AddressValue = MAKE_SBDFO (0, 0, 0x18, 1, 0x84);
	PciData = 0x00FEDF00; /* last address before processor local APIC at FEE00000 */
	PciData |= 1 << 7;    /* set NP (non-posted) bit */
	LibAmdPciWrite(AccessWidth32, PciAddress, &PciData, &StdHeader);
	PciAddress.AddressValue = MAKE_SBDFO (0, 0, 0x18, 1, 0x80);
	PciData = (0xFED00000 >> 8) | 3; /* lowest NP address is HPET at FED00000 */
	LibAmdPciWrite(AccessWidth32, PciAddress, &PciData, &StdHeader);

	/* Map the remaining PCI hole as posted MMIO */
	PciAddress.AddressValue = MAKE_SBDFO (0, 0, 0x18, 1, 0x8C);
	PciData = 0x00FECF00; /* last address before non-posted range */
	LibAmdPciWrite(AccessWidth32, PciAddress, &PciData, &StdHeader);
	LibAmdMsrRead (0xC001001A, &MsrReg, &StdHeader);
	MsrReg = (MsrReg >> 8) | 3;
	PciAddress.AddressValue = MAKE_SBDFO (0, 0, 0x18, 1, 0x88);
	PciData = (UINT32)MsrReg;
	LibAmdPciWrite(AccessWidth32, PciAddress, &PciData, &StdHeader);

	/* Send all IO (0000-FFFF) to southbridge. */
	PciAddress.AddressValue = MAKE_SBDFO (0, 0, 0x18, 1, 0xC4);
	PciData = 0x0000F000;
	LibAmdPciWrite(AccessWidth32, PciAddress, &PciData, &StdHeader);
	PciAddress.AddressValue = MAKE_SBDFO (0, 0, 0x18, 1, 0xC0);
	PciData = 0x00000003;
	LibAmdPciWrite(AccessWidth32, PciAddress, &PciData, &StdHeader);
	Status = AGESA_SUCCESS;
	return Status;
}

AGESA_STATUS agesawrapper_amdinitmmio(void)
{
	AGESA_STATUS                  Status;
	UINT64                        MsrReg;
	UINT32                        PciData;
	PCI_ADDR                      PciAddress;
	AMD_CONFIG_PARAMS             StdHeader;

	/*
	  Set the MMIO Configuration Base Address and Bus Range onto MMIO configuration base
	  Address MSR register.
	*/
	MsrReg = CONFIG_MMCONF_BASE_ADDRESS | (LibAmdBitScanReverse (CONFIG_MMCONF_BUS_NUMBER) << 2) | 1;
	LibAmdMsrWrite (0xC0010058, &MsrReg, &StdHeader);

	/*
	  Set the NB_CFG MSR register. Enable CF8 extended configuration cycles.
	*/
	LibAmdMsrRead (0xC001001F, &MsrReg, &StdHeader);
	MsrReg = MsrReg | 0x0000400000000000;
	LibAmdMsrWrite (0xC001001F, &MsrReg, &StdHeader);

	/* For serial port */
	PciData = 0xFF03FFD5;
	PciAddress.AddressValue = MAKE_SBDFO (0, 0, 0x14, 0x3, 0x44);
	LibAmdPciWrite(AccessWidth32, PciAddress, &PciData, &StdHeader);

	/* PSP */
	//PciData = 0xD;
	//PciAddress.AddressValue = MAKE_SBDFO (0, 0, 0x8, 0x0, 0x48);
	//LibAmdPciWrite(AccessWidth32, PciAddress, &PciData, &StdHeader);

	/* Set ROM cache onto WP to decrease post time */
	MsrReg = (0x0100000000ull - CACHE_ROM_SIZE) | 5ull;
	LibAmdMsrWrite (0x20C, &MsrReg, &StdHeader);
	MsrReg = ((1ULL << CONFIG_CPU_ADDR_BITS) - CACHE_ROM_SIZE) | 0x800ull;
	LibAmdMsrWrite (0x20D, &MsrReg, &StdHeader);

	Status = AGESA_SUCCESS;
	return Status;
}

AGESA_STATUS agesawrapper_amdinitreset(void)
{
	AGESA_STATUS status;
	AMD_INTERFACE_PARAMS AmdParamStruct;
	AMD_RESET_PARAMS AmdResetParams;

	LibAmdMemFill (&AmdParamStruct,
		       0,
		       sizeof (AMD_INTERFACE_PARAMS),
		       &(AmdParamStruct.StdHeader));

	LibAmdMemFill (&AmdResetParams,
		       0,
		       sizeof (AMD_RESET_PARAMS),
		       &(AmdResetParams.StdHeader));

	AmdParamStruct.AgesaFunctionName = AMD_INIT_RESET;
	AmdParamStruct.AllocationMethod = ByHost;
	AmdParamStruct.NewStructSize = sizeof(AMD_RESET_PARAMS);
	AmdParamStruct.NewStructPtr = &AmdResetParams;
	AmdParamStruct.StdHeader.AltImageBasePtr = 0;
	AmdParamStruct.StdHeader.CalloutPtr = (CALLOUT_ENTRY) &GetBiosCallout;
	AmdParamStruct.StdHeader.Func = 0;
	AmdParamStruct.StdHeader.ImageBasePtr = 0;
	AmdCreateStruct (&AmdParamStruct);

	AmdResetParams.FchInterface.Xhci0Enable = IS_ENABLED(CONFIG_HUDSON_XHCI_ENABLE);
	AmdResetParams.FchInterface.Xhci1Enable = FALSE;

	status = AmdInitReset ((AMD_RESET_PARAMS *)AmdParamStruct.NewStructPtr);
	if (status != AGESA_SUCCESS) agesawrapper_amdreadeventlog(AmdParamStruct.StdHeader.HeapStatus);
	AmdReleaseStruct (&AmdParamStruct);
	return status;
}

AGESA_STATUS agesawrapper_amdinitearly(void)
{
	AGESA_STATUS status;
	AMD_INTERFACE_PARAMS AmdParamStruct;
	AMD_EARLY_PARAMS     *AmdEarlyParamsPtr;

	LibAmdMemFill (&AmdParamStruct,
		       0,
		       sizeof (AMD_INTERFACE_PARAMS),
		       &(AmdParamStruct.StdHeader));

	AmdParamStruct.AgesaFunctionName = AMD_INIT_EARLY;
	AmdParamStruct.AllocationMethod = PreMemHeap;
	AmdParamStruct.StdHeader.AltImageBasePtr = 0;
	AmdParamStruct.StdHeader.CalloutPtr = (CALLOUT_ENTRY) &GetBiosCallout;
	AmdParamStruct.StdHeader.Func = 0;
	AmdParamStruct.StdHeader.ImageBasePtr = 0;
	AmdCreateStruct (&AmdParamStruct);

	AmdEarlyParamsPtr = (AMD_EARLY_PARAMS *)AmdParamStruct.NewStructPtr;
	OemCustomizeInitEarly (AmdEarlyParamsPtr);

	status = AmdInitEarly ((AMD_EARLY_PARAMS *)AmdParamStruct.NewStructPtr);
	//PspBarInitEarly ();
	if (status != AGESA_SUCCESS) agesawrapper_amdreadeventlog(AmdParamStruct.StdHeader.HeapStatus);
	AmdReleaseStruct (&AmdParamStruct);

	return status;
}

AGESA_STATUS agesawrapper_amdinitpost(void)
{
	AGESA_STATUS status;
	AMD_INTERFACE_PARAMS  AmdParamStruct;
	AMD_POST_PARAMS       *PostParams;

	LibAmdMemFill (&AmdParamStruct,
		       0,
		       sizeof (AMD_INTERFACE_PARAMS),
		       &(AmdParamStruct.StdHeader));

	AmdParamStruct.AgesaFunctionName = AMD_INIT_POST;
	AmdParamStruct.AllocationMethod = PreMemHeap;
	AmdParamStruct.StdHeader.AltImageBasePtr = 0;
	AmdParamStruct.StdHeader.CalloutPtr = (CALLOUT_ENTRY) &GetBiosCallout;
	AmdParamStruct.StdHeader.Func = 0;
	AmdParamStruct.StdHeader.ImageBasePtr = 0;

	AmdCreateStruct (&AmdParamStruct);
	PostParams = (AMD_POST_PARAMS *)AmdParamStruct.NewStructPtr;

	// Do not use IS_ENABLED here.  CONFIG_GFXUMA should always have a value.  Allow
	// the compiler to flag the error if CONFIG_GFXUMA is not set.
	PostParams->MemConfig.UmaMode = CONFIG_GFXUMA ? UMA_AUTO : UMA_NONE;
	PostParams->MemConfig.UmaSize = 0;

//	PostParams->MemConfig.EnableMemClr = 0;		// Don't clear memory for higher speed

	//
	// Use the table for this board
	//
	PostParams->MemConfig.PlatformMemoryConfiguration = OurPlatformMemoryConfiguration;

	status = AmdInitPost (PostParams);

	printk(
			BIOS_SPEW,
			"setup_uma_memory: syslimit 0x%08llX, bottomio 0x%08lx\n",
			(unsigned long long)(PostParams->MemConfig.SysLimit) << 16,
			(unsigned long)(PostParams->MemConfig.BottomIo) << 16
	);
	printk(
			BIOS_SPEW,
			"setup_uma_memory: uma size %luMB, uma start 0x%08lx\n",
			(unsigned long)(PostParams->MemConfig.UmaSize) >> (20 - 16),
			(unsigned long)(PostParams->MemConfig.UmaBase) << 16
	);

#if defined(__PRE_RAM__)
	printk(BIOS_INFO, "Checking memory between 0x%08lx and 0x%08lx\n", (unsigned long) 1024*1024, (unsigned long)(PostParams->MemConfig.Sub4GCacheTop));
	ram_check(1024*1024, PostParams->MemConfig.Sub4GCacheTop);
#endif

	if (status != AGESA_SUCCESS) agesawrapper_amdreadeventlog(PostParams->StdHeader.HeapStatus);
	AmdReleaseStruct (&AmdParamStruct);

	/* Initialize heap space */
#if defined(__PRE_RAM__)
	printk(BIOS_INFO, "Checking memory between 0x%08lx and 0x%08lx\n", (unsigned long) 1024*1024, (unsigned long)(PostParams->MemConfig.Sub4GCacheTop));
	ram_check(1024*1024, PostParams->MemConfig.Sub4GCacheTop);
#endif

	EmptyHeap();

	return status;
}

AGESA_STATUS agesawrapper_amdinitenv(void)
{
	AGESA_STATUS status;
	AMD_INTERFACE_PARAMS AmdParamStruct;
	AMD_ENV_PARAMS       *EnvParam;

	LibAmdMemFill (&AmdParamStruct,
		       0,
		       sizeof (AMD_INTERFACE_PARAMS),
		       &(AmdParamStruct.StdHeader));

	AmdParamStruct.AgesaFunctionName = AMD_INIT_ENV;
	AmdParamStruct.AllocationMethod = PostMemDram;
	AmdParamStruct.StdHeader.AltImageBasePtr = 0;
	AmdParamStruct.StdHeader.CalloutPtr = (CALLOUT_ENTRY) &GetBiosCallout;
	AmdParamStruct.StdHeader.Func = 0;
	AmdParamStruct.StdHeader.ImageBasePtr = 0;
	printk(BIOS_DEBUG, "agesawrapper_amdinitenv calling AmdCreateStruct\n");
	status = AmdCreateStruct (&AmdParamStruct);
	printk(BIOS_DEBUG, "agesawrapper_amdinitenv returning from AmdCreateStruct\n");
	EnvParam = (AMD_ENV_PARAMS *)AmdParamStruct.NewStructPtr;

	EnvParam->GnbEnvConfiguration.IommuSupport = TRUE;
	EnvParam->FchInterface.FchPowerFail = 1;			// System powered by default

	printk(BIOS_DEBUG, "agesawrapper_amdinitenv calling AGESA\n");
	status = AmdInitEnv (EnvParam);
	printk(BIOS_DEBUG, "agesawrapper_amdinitenv returned from AGESA\n");
	if (status != AGESA_SUCCESS) agesawrapper_amdreadeventlog(EnvParam->StdHeader.HeapStatus);
	/* Initialize Subordinate Bus Number and Secondary Bus Number
	 * In platform BIOS this address is allocated by PCI enumeration code
	 Modify D1F0x18
	*/

	return status;
}

#ifndef __PRE_RAM__
VOID* agesawrapper_getlateinitptr (int pick)
{
	switch (pick) {
	case PICK_DMI:
		return DmiTable;
	case PICK_PSTATE:
		return AcpiPstate;
	case PICK_SRAT:
		return AcpiSrat;
	case PICK_SLIT:
		return AcpiSlit;
	case PICK_WHEA_MCE:
		return AcpiWheaMce;
	case PICK_WHEA_CMC:
		return AcpiWheaCmc;
	case PICK_ALIB:
		return AcpiAlib;
	case PICK_IVRS:
		return AcpiIvrs;
	default:
		return NULL;
	}
}
#endif

AGESA_STATUS agesawrapper_amdinitmid(void)
{
	AGESA_STATUS status;
	AMD_INTERFACE_PARAMS AmdParamStruct;

	/* Enable MMIO on AMD CPU Address Map Controller */
	agesawrapper_amdinitcpuio ();

	LibAmdMemFill (&AmdParamStruct,
		       0,
		       sizeof (AMD_INTERFACE_PARAMS),
		       &(AmdParamStruct.StdHeader));

	AmdParamStruct.AgesaFunctionName = AMD_INIT_MID;
	AmdParamStruct.AllocationMethod = PostMemDram;
	AmdParamStruct.StdHeader.AltImageBasePtr = 0;
	AmdParamStruct.StdHeader.CalloutPtr = (CALLOUT_ENTRY) &GetBiosCallout;
	AmdParamStruct.StdHeader.Func = 0;
	AmdParamStruct.StdHeader.ImageBasePtr = 0;

	AmdCreateStruct (&AmdParamStruct);

	((AMD_MID_PARAMS *)AmdParamStruct.NewStructPtr)->GnbMidConfiguration.iGpuVgaMode = 0;/* 0 iGpuVgaAdapter, 1 iGpuVgaNonAdapter; */
	status = AmdInitMid ((AMD_MID_PARAMS *)AmdParamStruct.NewStructPtr);
	if (status != AGESA_SUCCESS) agesawrapper_amdreadeventlog(AmdParamStruct.StdHeader.HeapStatus);
	AmdReleaseStruct (&AmdParamStruct);

	return status;
}

#ifndef __PRE_RAM__
AGESA_STATUS agesawrapper_amdinitlate(void)
{
	AGESA_STATUS Status;
	AMD_INTERFACE_PARAMS AmdParamStruct;
	AMD_LATE_PARAMS *AmdLateParams;

	LibAmdMemFill (&AmdParamStruct,
		       0,
		       sizeof (AMD_INTERFACE_PARAMS),
		       &(AmdParamStruct.StdHeader));

	AmdParamStruct.AgesaFunctionName = AMD_INIT_LATE;
	AmdParamStruct.AllocationMethod = PostMemDram;
	AmdParamStruct.StdHeader.AltImageBasePtr = 0;
	AmdParamStruct.StdHeader.CalloutPtr = (CALLOUT_ENTRY) &GetBiosCallout;
	AmdParamStruct.StdHeader.HeapStatus = HEAP_SYSTEM_MEM;
	AmdParamStruct.StdHeader.Func = 0;
	AmdParamStruct.StdHeader.ImageBasePtr = 0;

	/* NOTE: if not call amdcreatestruct, the initializer(AmdInitLateInitializer) would not be called */
	AmdCreateStruct(&AmdParamStruct);
	AmdLateParams = (AMD_LATE_PARAMS *)AmdParamStruct.NewStructPtr;
	Status = AmdInitLate(AmdLateParams);
	if (Status != AGESA_SUCCESS) {
		agesawrapper_amdreadeventlog(AmdLateParams->StdHeader.HeapStatus);
		printk(BIOS_INFO,"\nAmdInitLate returned:  %d\n", Status);

		//ASSERT(Status == AGESA_SUCCESS);
	}

	DmiTable    = AmdLateParams->DmiTable;
	AcpiPstate  = AmdLateParams->AcpiPState;
	AcpiSrat    = AmdLateParams->AcpiSrat;
	AcpiSlit    = AmdLateParams->AcpiSlit;

	AcpiWheaMce = AmdLateParams->AcpiWheaMce;
	AcpiWheaCmc = AmdLateParams->AcpiWheaCmc;
	AcpiAlib    = AmdLateParams->AcpiAlib;
	AcpiIvrs    = AmdLateParams->AcpiIvrs;

	printk(BIOS_DEBUG, "DmiTable:%x, AcpiPstatein: %x, AcpiSrat:%x,"
	       "AcpiSlit:%x, Mce:%x, Cmc:%x,"
	       "Alib:%x, AcpiIvrs:%x in %s\n",
	       (unsigned int)DmiTable, (unsigned int)AcpiPstate, (unsigned int)AcpiSrat,
	       (unsigned int)AcpiSlit, (unsigned int)AcpiWheaMce, (unsigned int)AcpiWheaCmc,
	       (unsigned int)AcpiAlib, (unsigned int)AcpiIvrs, __func__);

	/* AmdReleaseStruct (&AmdParamStruct); */
	return Status;
}
#endif

AGESA_STATUS agesawrapper_amdlaterunaptask (
	UINT32 Func,
	UINT32 Data,
	VOID *ConfigPtr
	)
{
	AGESA_STATUS Status;
	AP_EXE_PARAMS ApExeParams;

	LibAmdMemFill (&ApExeParams,
		       0,
		       sizeof (AP_EXE_PARAMS),
		       &(ApExeParams.StdHeader));

	ApExeParams.StdHeader.AltImageBasePtr = 0;
	ApExeParams.StdHeader.CalloutPtr = (CALLOUT_ENTRY) &GetBiosCallout;
	ApExeParams.StdHeader.Func = 0;
	ApExeParams.StdHeader.ImageBasePtr = 0;
	ApExeParams.FunctionNumber = Func;
	ApExeParams.RelatedDataBlock = ConfigPtr;

	Status = AmdLateRunApTask (&ApExeParams);
	if (Status != AGESA_SUCCESS) {
		/* agesawrapper_amdreadeventlog(); */
		ASSERT(Status == AGESA_SUCCESS);
	}

	return Status;
}

#if CONFIG_HAVE_ACPI_RESUME

AGESA_STATUS agesawrapper_amdinitresume(void)
{
	AGESA_STATUS status;
	AMD_INTERFACE_PARAMS AmdParamStruct;
	AMD_RESUME_PARAMS     *AmdResumeParamsPtr;
	S3_DATA_TYPE            S3DataType;

	LibAmdMemFill (&AmdParamStruct,
		       0,
		       sizeof (AMD_INTERFACE_PARAMS),
		       &(AmdParamStruct.StdHeader));

	AmdParamStruct.AgesaFunctionName = AMD_INIT_RESUME;
	AmdParamStruct.AllocationMethod = PreMemHeap;
	AmdParamStruct.StdHeader.AltImageBasePtr = 0;
	AmdParamStruct.StdHeader.CalloutPtr = (CALLOUT_ENTRY) &GetBiosCallout;
	AmdParamStruct.StdHeader.Func = 0;
	AmdParamStruct.StdHeader.ImageBasePtr = 0;
	AmdCreateStruct (&AmdParamStruct);

	AmdResumeParamsPtr = (AMD_RESUME_PARAMS *)AmdParamStruct.NewStructPtr;

	AmdResumeParamsPtr->S3DataBlock.NvStorageSize = 0;
	AmdResumeParamsPtr->S3DataBlock.VolatileStorageSize = 0;
	S3DataType = S3DataTypeNonVolatile;
	OemAgesaGetS3Info (S3DataType,
			   (u32 *) &AmdResumeParamsPtr->S3DataBlock.NvStorageSize,
			   (void **) &AmdResumeParamsPtr->S3DataBlock.NvStorage);

	status = AmdInitResume ((AMD_RESUME_PARAMS *)AmdParamStruct.NewStructPtr);

	if (status != AGESA_SUCCESS) agesawrapper_amdreadeventlog(AmdParamStruct.StdHeader.HeapStatus);
	AmdReleaseStruct (&AmdParamStruct);

	return status;
}

#ifndef __PRE_RAM__
AGESA_STATUS agesawrapper_fchs3earlyrestore(void)
{
	AGESA_STATUS status = AGESA_SUCCESS;

	FCH_DATA_BLOCK      FchParams;
	AMD_CONFIG_PARAMS StdHeader;

	StdHeader.HeapStatus = HEAP_SYSTEM_MEM;
	StdHeader.HeapBasePtr = GetHeapBase(&StdHeader) + 0x10;
	StdHeader.AltImageBasePtr = 0;
	StdHeader.CalloutPtr = (CALLOUT_ENTRY) &GetBiosCallout;
	StdHeader.Func = 0;
	StdHeader.ImageBasePtr = 0;

	FchParams.StdHeader = &StdHeader;
	s3_resume_init_data(&FchParams);

	FchInitS3EarlyRestore(&FchParams);

	return status;
}
#endif

AGESA_STATUS agesawrapper_amds3laterestore(void)
{
	AGESA_STATUS Status;
	AMD_INTERFACE_PARAMS    AmdInterfaceParams;
	AMD_S3LATE_PARAMS       AmdS3LateParams;
	AMD_S3LATE_PARAMS       *AmdS3LateParamsPtr;
	S3_DATA_TYPE          S3DataType;

	agesawrapper_amdinitcpuio();
	LibAmdMemFill (&AmdS3LateParams,
		       0,
		       sizeof (AMD_S3LATE_PARAMS),
		       &(AmdS3LateParams.StdHeader));
	AmdInterfaceParams.StdHeader.ImageBasePtr = 0;
	AmdInterfaceParams.AllocationMethod = ByHost;
	AmdInterfaceParams.AgesaFunctionName = AMD_S3LATE_RESTORE;
	AmdInterfaceParams.NewStructPtr = &AmdS3LateParams;
	AmdInterfaceParams.StdHeader.CalloutPtr = (CALLOUT_ENTRY) &GetBiosCallout;
	AmdS3LateParamsPtr = &AmdS3LateParams;
	AmdInterfaceParams.NewStructSize = sizeof (AMD_S3LATE_PARAMS);

	AmdCreateStruct (&AmdInterfaceParams);

	AmdS3LateParamsPtr->S3DataBlock.VolatileStorageSize = 0;
	S3DataType = S3DataTypeVolatile;

	OemAgesaGetS3Info (S3DataType,
			   (u32 *) &AmdS3LateParamsPtr->S3DataBlock.VolatileStorageSize,
			   (void **) &AmdS3LateParamsPtr->S3DataBlock.VolatileStorage);

	Status = AmdS3LateRestore (AmdS3LateParamsPtr);
	if (Status != AGESA_SUCCESS) {
		agesawrapper_amdreadeventlog(AmdInterfaceParams.StdHeader.HeapStatus);
		ASSERT(Status == AGESA_SUCCESS);
	}

	return Status;
}

#ifndef __PRE_RAM__

extern UINT8 picr_data[FCH_INT_TABLE_SIZE], intr_data[FCH_INT_TABLE_SIZE];

AGESA_STATUS agesawrapper_fchs3laterestore(void)
{
	AGESA_STATUS status = AGESA_SUCCESS;

	FCH_DATA_BLOCK      FchParams;
	AMD_CONFIG_PARAMS StdHeader;
	UINT8 byte;

	StdHeader.HeapStatus = HEAP_SYSTEM_MEM;
	StdHeader.HeapBasePtr = GetHeapBase(&StdHeader) + 0x10;
	StdHeader.AltImageBasePtr = 0;
	StdHeader.CalloutPtr = (CALLOUT_ENTRY) &GetBiosCallout;
	StdHeader.Func = 0;
	StdHeader.ImageBasePtr = 0;

	FchParams.StdHeader = &StdHeader;
	s3_resume_init_data(&FchParams);
	FchInitS3LateRestore(&FchParams);
	/* PIC IRQ routine */
	for (byte = 0x0; byte < sizeof(picr_data); byte ++) {
		outb(byte, 0xC00);
		outb(picr_data[byte], 0xC01);
	}

	/* APIC IRQ routine */
	for (byte = 0x0; byte < sizeof(intr_data); byte ++) {
		outb(byte | 0x80, 0xC00);
		outb(intr_data[byte], 0xC01);
	}

	return status;
}
#endif

#ifndef __PRE_RAM__

AGESA_STATUS agesawrapper_amdS3Save(void)
{
	AGESA_STATUS Status;
	AMD_S3SAVE_PARAMS *AmdS3SaveParamsPtr;
	AMD_INTERFACE_PARAMS  AmdInterfaceParams;
	S3_DATA_TYPE          S3DataType;

	LibAmdMemFill (&AmdInterfaceParams,
		       0,
		       sizeof (AMD_INTERFACE_PARAMS),
		       &(AmdInterfaceParams.StdHeader));

	AmdInterfaceParams.StdHeader.ImageBasePtr = 0;
	AmdInterfaceParams.StdHeader.HeapStatus = HEAP_SYSTEM_MEM;
	AmdInterfaceParams.StdHeader.CalloutPtr = (CALLOUT_ENTRY) &GetBiosCallout;
	AmdInterfaceParams.AllocationMethod = PostMemDram;
	AmdInterfaceParams.AgesaFunctionName = AMD_S3_SAVE;
	AmdInterfaceParams.StdHeader.AltImageBasePtr = 0;
	AmdInterfaceParams.StdHeader.Func = 0;

	AmdCreateStruct(&AmdInterfaceParams);
	AmdS3SaveParamsPtr = (AMD_S3SAVE_PARAMS *)AmdInterfaceParams.NewStructPtr;
	AmdS3SaveParamsPtr->StdHeader = AmdInterfaceParams.StdHeader;

	Status = AmdS3Save(AmdS3SaveParamsPtr);
	if (Status != AGESA_SUCCESS) {
		agesawrapper_amdreadeventlog(AmdInterfaceParams.StdHeader.HeapStatus);
		ASSERT(Status == AGESA_SUCCESS);
	}

	S3DataType = S3DataTypeNonVolatile;
	printk(BIOS_DEBUG, "NvStorageSize=%x, NvStorage=%x\n",
	       (unsigned int)AmdS3SaveParamsPtr->S3DataBlock.NvStorageSize,
	       (unsigned int)AmdS3SaveParamsPtr->S3DataBlock.NvStorage);

//	Status = OemAgesaSaveS3Info (
//		S3DataType,
//		AmdS3SaveParamsPtr->S3DataBlock.NvStorageSize,
//		AmdS3SaveParamsPtr->S3DataBlock.NvStorage);
//	PspMboxBiosCmdS3Info (AmdS3SaveParamsPtr->S3DataBlock.NvStorage, AmdS3SaveParamsPtr->S3DataBlock.NvStorageSize);

	printk(BIOS_DEBUG, "VolatileStorageSize=%x, VolatileStorage=%x\n",
	       (unsigned int)AmdS3SaveParamsPtr->S3DataBlock.VolatileStorageSize,
	       (unsigned int)AmdS3SaveParamsPtr->S3DataBlock.VolatileStorage);

	if (AmdS3SaveParamsPtr->S3DataBlock.VolatileStorageSize != 0) {
		S3DataType = S3DataTypeVolatile;

		Status = OemAgesaSaveS3Info (
			S3DataType,
			AmdS3SaveParamsPtr->S3DataBlock.VolatileStorageSize,
			AmdS3SaveParamsPtr->S3DataBlock.VolatileStorage);
	}
	OemAgesaSaveMtrr();

	AmdReleaseStruct (&AmdInterfaceParams);

	return Status;
}

#endif  /* #ifndef __PRE_RAM__ */
#endif  /* CONFIG_HAVE_ACPI_RESUME */

AGESA_STATUS agesawrapper_amdreadeventlog (UINT8 HeapStatus)
{
	AGESA_STATUS Status;
	EVENT_PARAMS AmdEventParams;

	LibAmdMemFill (&AmdEventParams,
		       0,
		       sizeof (EVENT_PARAMS),
		       &(AmdEventParams.StdHeader));

	AmdEventParams.StdHeader.AltImageBasePtr = 0;
	AmdEventParams.StdHeader.CalloutPtr = (CALLOUT_ENTRY) &GetBiosCallout;
	AmdEventParams.StdHeader.Func = 0;
	AmdEventParams.StdHeader.ImageBasePtr = 0;
	AmdEventParams.StdHeader.HeapStatus = HeapStatus;
	Status = AmdReadEventLog (&AmdEventParams);
	while (AmdEventParams.EventClass != 0) {
		printk(BIOS_DEBUG,"\nEventLog:  EventClass = %x, EventInfo = %x.\n", (unsigned int)AmdEventParams.EventClass,(unsigned int)AmdEventParams.EventInfo);
		printk(BIOS_DEBUG,"  Param1 = %x, Param2 = %x.\n",(unsigned int)AmdEventParams.DataParam1, (unsigned int)AmdEventParams.DataParam2);
		printk(BIOS_DEBUG,"  Param3 = %x, Param4 = %x.\n",(unsigned int)AmdEventParams.DataParam3, (unsigned int)AmdEventParams.DataParam4);
		Status = AmdReadEventLog (&AmdEventParams);
	}

	return Status;
}
