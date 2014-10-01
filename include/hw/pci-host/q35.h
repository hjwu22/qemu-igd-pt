/*
 * q35.h
 *
 * Copyright (c) 2009 Isaku Yamahata <yamahata at valinux co jp>
 *                    VA Linux Systems Japan K.K.
 * Copyright (C) 2012 Jason Baron <jbaron@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>
 */

#ifndef HW_Q35_H
#define HW_Q35_H

#include "hw/hw.h"
#include "qemu/range.h"
#include "hw/isa/isa.h"
#include "hw/sysbus.h"
#include "hw/i386/pc.h"
#include "hw/isa/apm.h"
#include "hw/pci/pci.h"
#include "hw/pci/pcie_host.h"
#include "hw/acpi/acpi.h"
#include "hw/acpi/ich9.h"
#include "hw/pci-host/pam.h"

#define TYPE_Q35_HOST_DEVICE "q35-pcihost"
#define Q35_HOST_DEVICE(obj) \
     OBJECT_CHECK(Q35PCIHost, (obj), TYPE_Q35_HOST_DEVICE)

#define TYPE_MCH_PCI_DEVICE "mch"
#define MCH_PCI_DEVICE(obj) \
     OBJECT_CHECK(MCHPCIState, (obj), TYPE_MCH_PCI_DEVICE)

typedef struct MCHPCIState {
    /*< private >*/
    PCIDevice parent_obj;
    /*< public >*/

    MemoryRegion *ram_memory;
    MemoryRegion *pci_address_space;
    MemoryRegion *system_memory;
    MemoryRegion *address_space_io;
    PAMMemoryRegion pam_regions[13];
    MemoryRegion smram_region;
#ifdef CONFIG_INTEL_IGD_PASSTHROUGH
	MemoryRegion GFX_stolen;
	ram_addr_t GFX_stolen_base;/* BDSM */
	ram_addr_t GFX_stolen_size;
	MemoryRegion GFX_GTT_stolen;
	ram_addr_t GFX_GTT_stolen_base; /*BGSM*/
	ram_addr_t GFX_GTT_stolen_size;
#endif
    PcPciInfo pci_info;
    uint8_t smm_enabled;
    ram_addr_t below_4g_mem_size;
    ram_addr_t above_4g_mem_size;
    uint64_t pci_hole64_size;
    PcGuestInfo *guest_info;
    uint32_t short_root_bus;
} MCHPCIState;

typedef struct Q35PCIHost {
    /*< private >*/
    PCIExpressHost parent_obj;
    /*< public >*/

    MCHPCIState mch;
} Q35PCIHost;

#define Q35_MASK(bit, ms_bit, ls_bit) \
((uint##bit##_t)(((1ULL << ((ms_bit) + 1)) - 1) & ~((1ULL << ls_bit) - 1)))




/* PCI configuration */
#define MCH_HOST_BRIDGE                        "MCH"

#define MCH_HOST_BRIDGE_CONFIG_ADDR            0xcf8
#define MCH_HOST_BRIDGE_CONFIG_DATA            0xcfc

#ifdef CONFIG_INTEL_IGD_PASSTHROUGH

/* D0:F0 configuration space */
#define D0F0_VID                               0x00                         /* 16 bits RO */
#define D0F0_VID_SIZE                          2                            /* 2 Bytes */


#define D0F0_DID                               0x02                         /* 16 bits RO */
#define D0F0_DID_SIZE                          2                            /* 2 Bytes */


#define D0F0_PCICMD                            0x04                         /* 16 bits */
                                                                            /* 15:10 RO RESERVED */
#define D0F0_PCICMD_FB2B                       0x200                        /* 9 RO=0 Fast back-to-back Enable */
#define D0F0_PCICMD_SERR                       0x100                        /* 8 RW */
#define D0F0_PCICMD_ADSTEP                     0x80                         /* 7 RO=0 Address/Data Stepping Enable */
#define D0F0_PCICMD_PERR                       0x40                         /* 6 RW */
#define D0F0_PCICMD_VGASNOOP                   0x20                         /* 5 RO=0 VGA Palette Snoop Enable */
#define D0F0_PCICMD_MWI                        0x10                         /* 4 RO=0 Memory Write and Invalidate Enable */
                                                                            /* 3 RO RESERVED */
#define D0F0_PCICMD_BME                        0x04                         /* 2 RO=1 Bus Master Enable */
#define D0F0_PCICMD_MAE                        0x02                         /* 1 RO=1 Memory Access Enable */
#define D0F0_PCICMD_IOAE                       0x01                         /* 0 RO=0 I/O Access Enable */
#define D0F0_PCICMD_SIZE                       2                            /* 2 Bytes */


#define D0F0_PCISTS                            0x06                         /* 16 bits */
#define D0F0_PCISTS_DPE                        0x8000                       /* 15 RW1C Detected Parity Error */
#define D0F0_PCISTS_SSE                        0x4000                       /* 14 RW1C Signaled System Error */
#define D0F0_PCISTS_RMAS                       0x2000                       /* 13 RW1C Received Master Abort Status */
#define D0F0_PCISTS_RTAS                       0x1000                       /* 12 RW1C Reveived Target Abort Status */
#define D0F0_PCISTS_STAS                       0x800                        /* 11 RO=0 Signaled Target Abort Status */
#define D0F0_PCISTS_DEVT_MASK                  0x600                        /* 10:9 RO=DEVSEL_FAST DEVSEL Timing */
#define D0F0_PCISTS_DEVT_FAST                  0x000                        /* DEVSEL_FAST=00 */
#define D0F0_PCISTS_DPD                        0x100                        /* 8 RW1C Master Data parity Error Detected */
#define D0F0_PCISTS_FB2B                       0x80                         /* 7 RO=1 Fast Back-to-Back */
                                                                            /* 6 bitpos RO RESERVED */
#define D0F0_PCISTS_MC66                       0x20                         /* 5 RO=0 66 MHz Capable */
#define D0F0_PCISTS_CLIST                      0x10                         /* 4 RO=1 Capability List */
                                                                            /* 3:0 bitpos RO RESERVED */
#define D0F0_PCISTS_SIZE                       2                            /* 2 Bytes */


#define D0F0_RID                               0x08                         /* 8 bits RO */
#define D0F0_RID_SIZE                          1                            /* 1 Byte */


#define D0F0_CC                                0x09                         /* 24 bits */
#define D0F0_CC_BCC                            0x800000                     /* 23:16 RO=06 Base Class Code indicating a Bridge Device */
#define D0F0_CC_SUBCC                          0x8000                       /* 15:8 RO=00 Sub-Class Code indicating a  Host Bridge */
#define D0F0_CC_PI                             0x80                         /* 7:0 RO=00 Programming Interface */
#define D0F0_CC_SIZE                           3                            /* 3 Bytes */


#define D0F0_HDR                               0x0e                         /* 7:0 RO=00 indicating single function */
#define D0F0_HDR_SIZE                          1                            /* 1 Byte */


#define D0F0_SVID                              0x2c                         /* 15:0 RW-O Subsytem Vendor ID */
#define D0F0_SVID_SIZE                         2                            /* 2 Bytes */


#define D0F0_SID                               0x2e                         /* 15:0 RW-O Subsytem ID */
#define D0F0_SID_SIZE                          2                            /* 2 Bytes */


#define D0F0_PXPEPBAR                          0x40                         /* 64 bits PCI Express Egrees Port Base Address Register */
                                                                            /* 63:39 RO RESERVED */
#define D0F0_PXPEPBAR_PXPEPBAR                 Q35_MASK(64, 38, 12)         /* 38:12 RW PXPEPBAR */
                                                                            /* 11:1 RO RESERVED */
#define D0F0_PXPEPBAR_PXPEPBAREN               0x01                         /* 0 RW PXPEPBAR Enable */
#define D0F0_PXPEPBAR_SIZE                     8                            /* 8 Bytes */


#define D0F0_MCHBAR                            0x48                         /* 64 bits Host Memory Mapped Register Range Base */
                                                                            /* 63:39 RO RESERVED */
#define D0F0_MCHBAR_MCHBAR                     Q35_MASK(64, 38, 15)         /* 38:15 RW MCHBAR */
                                                                            /* 14:1 RO RESERVED */
#define D0F0_MCHBAR_MCHBAREN                   0x01                         /* 0 RW MCHBAR Enable */
#define D0F0_MCHBAR_SIZE                       8                            /* 8 Bytes */


#define D0F0_GGC                               0x50                         /* 16 bits GMCH Graphics Control Register */
                                                                            /* 15 RO RESERVED */
#define D0F0_GGC_VAMEN                         (0x1 << 14)                     /* 14 RW-L Versatile Acceleration Mode Enabled */
                                                                            /* 13:10 RO RESERVED */                                                                            /* 2 RO RESERVED */
#define D0F0_GGC_IVD                           (0x1 << 2)                   /* 1 RW-L IGD VGA Disable */
#define D0F0_GGC_GGCLCK                        0x1                          /* 0 RW-KL GGC Lock */
#define D0F0_GGC_SIZE                          2                            /* 2 Bytes */

#define MCH_GFX_GTT_STOLEN_SIZE_DEFAULT ((1 << 27) + (1 << 21))
#define GFX_STOLEN_SIZE 480 * 1024 * 1024
#define GFX_GTT_STOLEN_BASE 					0xC0000000
#define GFX_GTT_STOLEN_SIZE						0x20000
#define GFX_STOLEN_BASE							(GFX_GTT_STOLEN_BASE + GFX_GTT_STOLEN_SIZE)


#define D0F0_DEVEN                             0x54                         /* 32 bits Device Enable Register */
                                                                            /* 31:15 RO RESERVED */
                                                                            /* 14 RO RESERVED */
#define D0F0_DEVEN_D6F0EN                      (1 << 13)                       /* 13 RW-L PEG60 Enable */
                                                                            /* 12:8 RO RESERVED */
                                                                            
#define D0F0_DEVEN_D4EN                        (0x1 << 7)						/* 7 RW-L DEV4 */
																				/* 6:6 RO RESERVED */

/* Should notice following bit this is HASWELL specific */
#define D0F0_DEVEN_D3EN                        (0x1 << 5)                	 	/* 5 RW-L Internal HD Audio Controller */

#define D0F0_DEVEN_D2EN                        (0x1 << 4)						/* 4 RW-L Internal Graphics Engine */
#define D0F0_DEVEN_D1F0EN                      (0x1 << 3)                       /* 3 RW-L PEG10 Enable */
#define D0F0_DEVEN_D1F1EN                      (0x1 << 2)                       /* 2 RW-L PEG11 Enable */
#define D0F0_DEVEN_D1F2EN                      (0x1 << 1)                       /* 1 RW-L PEG12 Enable */
#define D0F0_DEVEN_D0EN                        0x1                          /* 0 RO=1 Host Bridge */
#define D0F0_DEVEN_SIZE                        4                            /* 4 Bytes */


#define D0F0_PAVPC                             0x58                         /* Protected Audio Video Path Control */

#define D0F0_DMIBAR                            0x68                         /* 64 bits Root Complex Register Range Base Address Register */
                                                                            /* 63:39 RO RESERVED */
#define D0F0_DMIBAR_DMIBAR                     Q35_MASK(64, 38, 12)         /* 38:12 DMI Base Address */
                                                                            /* 11:1 RO RESERVED */
#define D0F0_DMIBAR_DMIBAREN                   0x01                         /* 0 RW DMIBAR Enable */
#define D0F0_DMIBAR_SIZE                       8                            /* 8 Bytes */


#define D0F0_TOM                               0xa0                         /* 64 bits Top of memory register */
                                                                            /* 63:39 RO Reserved */
#define D0F0_TOM_TOM                           Q35_MASK(64,38,20)           /* 38:20 RW-L Top of Memory */
                                                                            /* 19:1 RO Reserved */
#define D0F0_TOM_LOCK                          0x1                          /* 0 RW-KL Lock */
#define D0F0_TOM_SIZE                          8                            /* 8 Bytes */


#define D0F0_TOUUD                             0xa8                         /* 64 bits Top of Upper Usable DRAM Register */
                                                                            /* 63:39 RO Reserved */
#define D0F0_TOUUD_TOUUD                       Q35_MASK(64,38,20)           /* 38:20 RW-L Top of Upper Usable DRAM Register */
                                                                            /* 19:1 RO Reserved */
#define D0F0_TOUUD_LOCK                        0x1                          /* 0 RW-KL Lock */
#define D0F0_TOUUD_SIZE                        8                            /* 8 Bytes */


#define D0F0_BDSM                              0xb0                         /* 32 bits Base Data of Stolen Memory Register */
#define D0F0_BDSM_BDSM                         Q35_MASK(32,31,20)           /* 31:20 RW-L Graphics Base of Stolen Memory (BDSM) */
                                                                            /* 19:1 RO Reserved */
#define D0F0_BDSM_LOCK                         0x1                          /* 0 RW-KL Lock */
#define D0F0_BDSM_SIZE                         4                            /* 4 Bytes */


#define D0F0_BGSM                              0xb4                         /* 32 bits Base of GTT Stolen Memory Register */
#define D0F0_BGSM_BGSM                         Q35_MASK(32,31,20)           /* 31:20 RW-L Graphics Base of GTT Stolen Memory (BGSM) */
                                                                            /* 19:1 RO Reserved */
#define D0F0_BGSM_LOCK                         0x1                          /* 0 RW-KL Lock */
#define D0F0_BGSM_SIZE                         4                            /* 4 Bytes */


#define D0F0_TSEG                              0xb8                         /* 32 bits G Memory Base Register */
#define D0F0_TSEG_TSEGMB                       Q35_MASK(32,31,20)           /* 31:20 RW-L TSEG Memory Base (TSEGMB) */
                                                                            /* 19:1 RO Reserved */
#define D0F0_TSEG_LOCK                         0x1                          /* 0 RW-KL Lock */
#define D0F0_TSEG_SIZE                         4                            /* 4 Bytes */


#define D0F0_TOLUD                             0xbc                         /* 32 bits Top of Low Usable DRAM */
#define D0F0_TOLUD_TOLUD                       Q35_MASK(32, 31, 20)         /* 31:20 RW-L TOLUD */
                                                                            /* 19:1 RO RESERVED */
#define D0F0_TOLUD_LOCK                        0x1                          /* 0 RW-KL Lock */
#define D0F0_TOLUD_SIZE                        4                            /* 4 Bytes */
#endif

#define MCH_HOST_BRIDGE_REVISION_DEFAULT       0x0

/* so as gen4 mch*/
#define MCH_HOST_BRIDGE_PCIEXBAR               0x60    /* 64bit register */
#define MCH_HOST_BRIDGE_PCIEXBAR_SIZE          8       /* 64bit register */
#define MCH_HOST_BRIDGE_PCIEXBAR_DEFAULT       0xb0000000
#define MCH_HOST_BRIDGE_PCIEXBAR_MAX           (0x10000000) /* 256M */

#define MCH_HOST_BRIDGE_PCIEXBAR_ADMSK         Q35_MASK(64, 35, 28)
#define MCH_HOST_BRIDGE_PCIEXBAR_128ADMSK      ((uint64_t)(1 << 26))
#define MCH_HOST_BRIDGE_PCIEXBAR_64ADMSK       ((uint64_t)(1 << 25))
#define MCH_HOST_BRIDGE_PCIEXBAR_LENGTH_MASK   ((uint64_t)(0x3 << 1))
#define MCH_HOST_BRIDGE_PCIEXBAR_LENGTH_256M   ((uint64_t)(0x0 << 1))
#define MCH_HOST_BRIDGE_PCIEXBAR_LENGTH_128M   ((uint64_t)(0x1 << 1))
#define MCH_HOST_BRIDGE_PCIEXBAR_LENGTH_64M    ((uint64_t)(0x2 << 1))
#define MCH_HOST_BRIDGE_PCIEXBAR_LENGTH_RVD    ((uint64_t)(0x3 << 1))
#define MCH_HOST_BRIDGE_PCIEXBAREN             ((uint64_t)1)

#define MCH_HOST_BRIDGE_PAM_NB                 7
#define MCH_HOST_BRIDGE_PAM_SIZE               7
#define MCH_HOST_BRIDGE_PAM0                   0x90
#define MCH_HOST_BRIDGE_PAM_BIOS_AREA          0xf0000
#define MCH_HOST_BRIDGE_PAM_AREA_SIZE          0x10000 /* 16KB */
#define MCH_HOST_BRIDGE_PAM1                   0x91
#define MCH_HOST_BRIDGE_PAM_EXPAN_AREA         0xc0000
#define MCH_HOST_BRIDGE_PAM_EXPAN_SIZE         0x04000
#define MCH_HOST_BRIDGE_PAM2                   0x92
#define MCH_HOST_BRIDGE_PAM3                   0x93
#define MCH_HOST_BRIDGE_PAM4                   0x94
#define MCH_HOST_BRIDGE_PAM_EXBIOS_AREA        0xe0000
#define MCH_HOST_BRIDGE_PAM_EXBIOS_SIZE        0x04000
#define MCH_HOST_BRIDGE_PAM5                   0x95
#define MCH_HOST_BRIDGE_PAM6                   0x96
#define MCH_HOST_BRIDGE_PAM_WE_HI              ((uint8_t)(0x2 << 4))
#define MCH_HOST_BRIDGE_PAM_RE_HI              ((uint8_t)(0x1 << 4))
#define MCH_HOST_BRIDGE_PAM_HI_MASK            ((uint8_t)(0x3 << 4))
#define MCH_HOST_BRIDGE_PAM_WE_LO              ((uint8_t)0x2)
#define MCH_HOST_BRIDGE_PAM_RE_LO              ((uint8_t)0x1)
#define MCH_HOST_BRIDGE_PAM_LO_MASK            ((uint8_t)0x3)
#define MCH_HOST_BRIDGE_PAM_WE                 ((uint8_t)0x2)
#define MCH_HOST_BRIDGE_PAM_RE                 ((uint8_t)0x1)
#define MCH_HOST_BRIDGE_PAM_MASK               ((uint8_t)0x3)

#define MCH_HOST_BRDIGE_SMRAM                  0x88
//#define MCH_HOST_BRDIGE_SMRAM                  0x90
#define MCH_HOST_BRDIGE_SMRAM_SIZE             1
#define MCH_HOST_BRIDGE_SMRAM_DEFAULT          ((uint8_t)0x1a)
#define MCH_HOST_BRIDGE_SMRAM_D_OPEN           ((uint8_t)(1 << 6))
#define MCH_HOST_BRIDGE_SMRAM_D_CLS            ((uint8_t)(1 << 5))
#define MCH_HOST_BRIDGE_SMRAM_D_LCK            ((uint8_t)(1 << 4))
#define MCH_HOST_BRIDGE_SMRAM_G_SMRAME         ((uint8_t)(1 << 3))
#define MCH_HOST_BRIDGE_SMRAM_C_BASE_SEG_MASK  ((uint8_t)0x7)
#define MCH_HOST_BRIDGE_SMRAM_C_BASE_SEG       ((uint8_t)0x2)  /* hardwired to b010 */
#define MCH_HOST_BRIDGE_SMRAM_C_BASE           0xa0000
#define MCH_HOST_BRIDGE_SMRAM_C_END            0xc0000
#define MCH_HOST_BRIDGE_SMRAM_C_SIZE           0x20000
#define MCH_HOST_BRIDGE_UPPER_SYSTEM_BIOS_END  0x100000

#define MCH_HOST_BRIDGE_ESMRAMC                0x9e
#define MCH_HOST_BRDIGE_ESMRAMC_H_SMRAME       ((uint8_t)(1 << 6))
#define MCH_HOST_BRDIGE_ESMRAMC_E_SMERR        ((uint8_t)(1 << 5))
#define MCH_HOST_BRDIGE_ESMRAMC_SM_CACHE       ((uint8_t)(1 << 4))
#define MCH_HOST_BRDIGE_ESMRAMC_SM_L1          ((uint8_t)(1 << 3))
#define MCH_HOST_BRDIGE_ESMRAMC_SM_L2          ((uint8_t)(1 << 2))
#define MCH_HOST_BRDIGE_ESMRAMC_TSEG_SZ_MASK   ((uint8_t)(0x3 << 1))
#define MCH_HOST_BRDIGE_ESMRAMC_TSEG_SZ_1MB    ((uint8_t)(0x0 << 1))
#define MCH_HOST_BRDIGE_ESMRAMC_TSEG_SZ_2MB    ((uint8_t)(0x1 << 1))
#define MCH_HOST_BRDIGE_ESMRAMC_TSEG_SZ_8MB    ((uint8_t)(0x2 << 1))
#define MCH_HOST_BRDIGE_ESMRAMC_T_EN           ((uint8_t)1)



/* D1:F0 PCIE* port*/
#define MCH_PCIE_DEV                           1
#define MCH_PCIE_FUNC                          0

uint64_t mch_mcfg_base(void);

#endif /* HW_Q35_H */
