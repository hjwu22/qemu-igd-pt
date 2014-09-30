/*
 * ACPI implementation
 *
 * Copyright (c) 2006 Fabrice Bellard
 * Copyright (c) 2009 Isaku Yamahata <yamahata at valinux co jp>
 *               VA Linux Systems Japan K.K.
 * Copyright (C) 2012 Jason Baron <jbaron@redhat.com>
 *
 * This is based on acpi.c, but heavily rewritten.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>
 *
 * Contributions after 2012-01-13 are licensed under the terms of the
 * GNU GPL, version 2 or (at your option) any later version.
 *
 */
#include "config-host.h"
#include "hw/hw.h"
#include "hw/i386/pc.h"
#include "hw/i2c/pm_smbus.h"
#include "hw/pci/pci.h"
#include "sysemu/sysemu.h"
#include "hw/i2c/i2c.h"
#include "hw/i2c/smbus.h"

#include "hw/i386/ich9.h"


//#define DEBUG
#ifdef DEBUG
#define DPRINTF(fmt, ...) \
    do { fprintf(stderr, "smbus: " fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
    do { } while (0)
#endif

#define TYPE_ICH9_SMB_DEVICE "ICH9 SMB"
#define ICH9_SMB_DEVICE(obj) \
     OBJECT_CHECK(ICH9SMBState, (obj), TYPE_ICH9_SMB_DEVICE)

typedef struct ICH9SMBState {
    PCIDevice dev;
	MemoryRegion bar0;
    PMSMBus smb;
} ICH9SMBState;

static const VMStateDescription vmstate_ich9_smbus = {
    .name = "ich9_smb",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields = (VMStateField[]) {
        VMSTATE_PCI_DEVICE(dev, struct ICH9SMBState),
        VMSTATE_END_OF_LIST()
    }
};

static void ich9_smbus_write_config(PCIDevice *d, uint32_t address,
                                    uint32_t val, int len)
{
    ICH9SMBState *s = ICH9_SMB_DEVICE(d);
    
    pci_default_write_config(d, address, val, len);
    if (range_covers_byte(address, len, ICH9_SMB_HOSTC)) {
        uint8_t hostc = s->dev.config[ICH9_SMB_HOSTC];
        if ((hostc & ICH9_SMB_HOSTC_HST_EN) &&
            !(hostc & ICH9_SMB_HOSTC_I2C_EN)) {
            memory_region_set_enabled(&s->smb.io, true);
        } else {
            memory_region_set_enabled(&s->smb.io, false);
        }
    }
	DPRINTF("%s(@0x%lx, 0x%lx, %d)\n", __func__, address, val, len);
}

static int ich9_smbus_initfn(PCIDevice *d)
{
    ICH9SMBState *s = ICH9_SMB_DEVICE(d);

    /* TODO? D31IP.SMIP in chipset configuration space */
    pci_config_set_interrupt_pin(d->config, 0x01); /* interrupt pin 1 */

    pci_set_byte(d->config + ICH9_SMB_HOSTC, 0);
    /* TODO bar0, bar1: 64bit BAR support*/
#ifdef CONFIG_INTEL_IGD_PASSTHROUGH

	//memory_region_init(&s->bar0, OBJECT(dev), "smbus-bar0", 0x2000000);

    /* XXX: add byte swapping apertures */
    //memory_region_add_subregion(&s->pci_bar, 0, &s->cirrus_linear_io);
    //memory_region_add_subregion(&s->pci_bar, 0x1000000,
     //                           &s->cirrus_linear_bitblt_io);

     /* setup memory space */
     /* memory #0 LFB */
     /* memory #1 memory-mapped I/O */
     /* XXX: s->vga.vram_size must be a power of two */
     //pci_register_bar(&d->dev, 0, PCI_BASE_ADDRESS_MEM_PREFETCH, &s->pci_bar);
     //if (device_id == CIRRUS_ID_CLGD5446) {
     //    pci_register_bar(&d->dev, 1, 0, &s->cirrus_mmio_io);
     //}
#endif
    pm_smbus_init(&d->qdev, &s->smb);
    pci_register_bar(d, ICH9_SMB_SMB_BASE_BAR, PCI_BASE_ADDRESS_SPACE_IO,
                     &s->smb.io);
    return 0;
}

static void ich9_smb_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->vendor_id = PCI_VENDOR_ID_INTEL;
#ifdef CONFIG_INTEL_IGD_PASSTHROUGH
	k->device_id = __host_pci_read_config(0, 0x1f, 3, 0x02, 2);
    k->revision =  __host_pci_read_config(0, 0x1f, 3, 0x08, 2);
	k->subsystem_vendor_id = __host_pci_read_config(0, 0x1f, 3, 0x2c, 2);
	k->subsystem_id =  __host_pci_read_config(0, 0x1f, 3, 0x2e, 2);
#else
    k->device_id = PCI_DEVICE_ID_INTEL_ICH9_6;
    k->revision = ICH9_A2_SMB_REVISION;
#endif
    k->class_id = PCI_CLASS_SERIAL_SMBUS;
    dc->vmsd = &vmstate_ich9_smbus;
    dc->desc = "ICH9 SMBUS Bridge";
    k->init = ich9_smbus_initfn;
    k->config_write = ich9_smbus_write_config;
    /*
     * Reason: part of ICH9 southbridge, needs to be wired up by
     * pc_q35_init()
     */
    dc->cannot_instantiate_with_device_add_yet = true;
}

I2CBus *ich9_smb_init(PCIBus *bus, int devfn, uint32_t smb_io_base)
{
    PCIDevice *d =
        pci_create_simple_multifunction(bus, devfn, true, TYPE_ICH9_SMB_DEVICE);
    ICH9SMBState *s = ICH9_SMB_DEVICE(d);
    return s->smb.smbus;
}

static const TypeInfo ich9_smb_info = {
    .name   = TYPE_ICH9_SMB_DEVICE,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(ICH9SMBState),
    .class_init = ich9_smb_class_init,
};

static void ich9_smb_register(void)
{
    type_register_static(&ich9_smb_info);
}

type_init(ich9_smb_register);
