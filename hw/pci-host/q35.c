/*
 * QEMU MCH/ICH9 PCI Bridge Emulation
 *
 * Copyright (c) 2006 Fabrice Bellard
 * Copyright (c) 2009, 2010, 2011
 *               Isaku Yamahata <yamahata at valinux co jp>
 *               VA Linux Systems Japan K.K.
 * Copyright (C) 2012 Jason Baron <jbaron@redhat.com>
 *               2013 Andrew Barnes <andy@outsideglobe.com> IGD Support
 *
 * This is based on piix_pci.c, but heavily modified.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "hw/hw.h"
#include "hw/pci-host/q35.h"
#include "qapi/visitor.h"
#include "hw/pci/pci.h" //IGD PASSTHROUGH
#include "qemu/error-report.h"
#include "config-host.h"
#define DEBUG_Q35
#ifdef DEBUG_Q35
# define Q35_DPRINTF(format, ...)\
do {printf("Q35: " format, ## __VA_ARGS__);} while(0)
#else
# define Q35_DPRINTF(format, ...) do { } while (0)
#endif
 

#ifdef CONFIG_INTEL_IGD_PASSTHROUGH
static void mch_update_gfx_stolen(PCIDevice *d);
static void mch_update_gfx_gtt_stolen(PCIDevice *d);
#endif

/****************************************************************************
 * Q35 host
 */


static void q35_host_realize(DeviceState *dev, Error **errp)
{
    PCIHostState *pci = PCI_HOST_BRIDGE(dev);
    Q35PCIHost *s = Q35_HOST_DEVICE(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    sysbus_add_io(sbd, MCH_HOST_BRIDGE_CONFIG_ADDR, &pci->conf_mem);
    sysbus_init_ioports(sbd, MCH_HOST_BRIDGE_CONFIG_ADDR, 4);

    sysbus_add_io(sbd, MCH_HOST_BRIDGE_CONFIG_DATA, &pci->data_mem);
    sysbus_init_ioports(sbd, MCH_HOST_BRIDGE_CONFIG_DATA, 4);

    if (pcie_host_init(PCIE_HOST_BRIDGE(s)) < 0) {
        error_setg(errp, "failed to initialize pcie host");
        return;
    }
    pci->bus = pci_bus_new(DEVICE(s), "pcie.0",
                           s->mch.pci_address_space, s->mch.address_space_io,
                           0, TYPE_PCIE_BUS);
    qdev_set_parent_bus(DEVICE(&s->mch), BUS(pci->bus));
    qdev_init_nofail(DEVICE(&s->mch));
}

static const char *q35_host_root_bus_path(PCIHostState *host_bridge,
                                          PCIBus *rootbus)
{
    Q35PCIHost *s = Q35_HOST_DEVICE(host_bridge);

     /* For backwards compat with old device paths */
    if (s->mch.short_root_bus) {
        return "0000";
    }
    return "0000:00";
}

static void q35_host_get_pci_hole_start(Object *obj, Visitor *v,
                                        void *opaque, const char *name,
                                        Error **errp)
{
    Q35PCIHost *s = Q35_HOST_DEVICE(obj);
    uint32_t value = s->mch.pci_info.w32.begin;

    visit_type_uint32(v, &value, name, errp);
}

static void q35_host_get_pci_hole_end(Object *obj, Visitor *v,
                                      void *opaque, const char *name,
                                      Error **errp)
{
    Q35PCIHost *s = Q35_HOST_DEVICE(obj);
    uint32_t value = s->mch.pci_info.w32.end;

    visit_type_uint32(v, &value, name, errp);
}

static void q35_host_get_pci_hole64_start(Object *obj, Visitor *v,
                                          void *opaque, const char *name,
                                          Error **errp)
{
    PCIHostState *h = PCI_HOST_BRIDGE(obj);
    Range w64;

    pci_bus_get_w64_range(h->bus, &w64);

    visit_type_uint64(v, &w64.begin, name, errp);
}

static void q35_host_get_pci_hole64_end(Object *obj, Visitor *v,
                                        void *opaque, const char *name,
                                        Error **errp)
{
    PCIHostState *h = PCI_HOST_BRIDGE(obj);
    Range w64;

    pci_bus_get_w64_range(h->bus, &w64);

    visit_type_uint64(v, &w64.end, name, errp);
}

static void q35_host_get_mmcfg_size(Object *obj, Visitor *v,
                                    void *opaque, const char *name,
                                    Error **errp)
{
    PCIExpressHost *e = PCIE_HOST_BRIDGE(obj);
    uint32_t value = e->size;

    visit_type_uint32(v, &value, name, errp);
}

static Property mch_props[] = {
    DEFINE_PROP_UINT64(PCIE_HOST_MCFG_BASE, Q35PCIHost, parent_obj.base_addr,
                        MCH_HOST_BRIDGE_PCIEXBAR_DEFAULT),
    DEFINE_PROP_SIZE(PCI_HOST_PROP_PCI_HOLE64_SIZE, Q35PCIHost,
                     mch.pci_hole64_size, DEFAULT_PCI_HOLE64_SIZE),
    DEFINE_PROP_UINT32("short_root_bus", Q35PCIHost, mch.short_root_bus, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static void q35_host_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIHostBridgeClass *hc = PCI_HOST_BRIDGE_CLASS(klass);

    hc->root_bus_path = q35_host_root_bus_path;
    dc->realize = q35_host_realize;
    dc->props = mch_props;
    set_bit(DEVICE_CATEGORY_BRIDGE, dc->categories);
    dc->fw_name = "pci";
}

static void q35_host_initfn(Object *obj)
{
    Q35PCIHost *s = Q35_HOST_DEVICE(obj);
    PCIHostState *phb = PCI_HOST_BRIDGE(obj);

    memory_region_init_io(&phb->conf_mem, obj, &pci_host_conf_le_ops, phb,
                          "pci-conf-idx", 4);
    memory_region_init_io(&phb->data_mem, obj, &pci_host_data_le_ops, phb,
                          "pci-conf-data", 4);

    object_initialize(&s->mch, sizeof(s->mch), TYPE_MCH_PCI_DEVICE);
    object_property_add_child(OBJECT(s), "mch", OBJECT(&s->mch), NULL);
    qdev_prop_set_uint32(DEVICE(&s->mch), "addr", PCI_DEVFN(0, 0));
    qdev_prop_set_bit(DEVICE(&s->mch), "multifunction", false);

    object_property_add(obj, PCI_HOST_PROP_PCI_HOLE_START, "int",
                        q35_host_get_pci_hole_start,
                        NULL, NULL, NULL, NULL);

    object_property_add(obj, PCI_HOST_PROP_PCI_HOLE_END, "int",
                        q35_host_get_pci_hole_end,
                        NULL, NULL, NULL, NULL);

    object_property_add(obj, PCI_HOST_PROP_PCI_HOLE64_START, "int",
                        q35_host_get_pci_hole64_start,
                        NULL, NULL, NULL, NULL);

    object_property_add(obj, PCI_HOST_PROP_PCI_HOLE64_END, "int",
                        q35_host_get_pci_hole64_end,
                        NULL, NULL, NULL, NULL);

    object_property_add(obj, PCIE_HOST_MCFG_SIZE, "int",
                        q35_host_get_mmcfg_size,
                        NULL, NULL, NULL, NULL);

    /* Leave enough space for the biggest MCFG BAR */
    /* TODO: this matches current bios behaviour, but
     * it's not a power of two, which means an MTRR
     * can't cover it exactly.
     */
    s->mch.pci_info.w32.begin = MCH_HOST_BRIDGE_PCIEXBAR_DEFAULT +
        MCH_HOST_BRIDGE_PCIEXBAR_MAX;
    s->mch.pci_info.w32.end = IO_APIC_DEFAULT_ADDRESS;
}

static const TypeInfo q35_host_info = {
    .name       = TYPE_Q35_HOST_DEVICE,
    .parent     = TYPE_PCIE_HOST_BRIDGE,
    .instance_size = sizeof(Q35PCIHost),
    .instance_init = q35_host_initfn,
    .class_init = q35_host_class_init,
};

/****************************************************************************
 * MCH D0:F0
 */

/* PCIe MMCFG */
static void mch_update_pciexbar(MCHPCIState *mch)
{
    PCIDevice *pci_dev = PCI_DEVICE(mch);
    BusState *bus = qdev_get_parent_bus(DEVICE(mch));
    PCIExpressHost *pehb = PCIE_HOST_BRIDGE(bus->parent);

    uint64_t pciexbar;
    int enable;
    uint64_t addr;
    uint64_t addr_mask;
    uint32_t length;

    pciexbar = pci_get_quad(pci_dev->config + MCH_HOST_BRIDGE_PCIEXBAR);
    enable = pciexbar & MCH_HOST_BRIDGE_PCIEXBAREN;
    addr_mask = MCH_HOST_BRIDGE_PCIEXBAR_ADMSK;
    switch (pciexbar & MCH_HOST_BRIDGE_PCIEXBAR_LENGTH_MASK) {
    case MCH_HOST_BRIDGE_PCIEXBAR_LENGTH_256M:
        length = 256 * 1024 * 1024;
        break;
    case MCH_HOST_BRIDGE_PCIEXBAR_LENGTH_128M:
        length = 128 * 1024 * 1024;
        addr_mask |= MCH_HOST_BRIDGE_PCIEXBAR_128ADMSK |
            MCH_HOST_BRIDGE_PCIEXBAR_64ADMSK;
        break;
    case MCH_HOST_BRIDGE_PCIEXBAR_LENGTH_64M:
        length = 64 * 1024 * 1024;
        addr_mask |= MCH_HOST_BRIDGE_PCIEXBAR_64ADMSK;
        break;
    case MCH_HOST_BRIDGE_PCIEXBAR_LENGTH_RVD:
    default:
        enable = 0;
        length = 0;
        abort();
        break;
    }
    addr = pciexbar & addr_mask;
    pcie_host_mmcfg_update(pehb, enable, addr, length);
    /* Leave enough space for the MCFG BAR */
    /*
     * TODO: this matches current bios behaviour, but it's not a power of two,
     * which means an MTRR can't cover it exactly.
     */
    if (enable) {
        mch->pci_info.w32.begin = addr + length;
    } else {
        mch->pci_info.w32.begin = MCH_HOST_BRIDGE_PCIEXBAR_DEFAULT;
    }
}

/* PAM */
static void mch_update_pam(MCHPCIState *mch)
{
    PCIDevice *pd = PCI_DEVICE(mch);
    int i;

    memory_region_transaction_begin();
    for (i = 0; i < 13; i++) {
        pam_update(&mch->pam_regions[i], i,
                   pd->config[MCH_HOST_BRIDGE_PAM0 + ((i + 1) / 2)]);
    }
    memory_region_transaction_commit();
}

/* SMRAM */
static void mch_update_smram(MCHPCIState *mch)
{
    PCIDevice *pd = PCI_DEVICE(mch);

    memory_region_transaction_begin();
    smram_update(&mch->smram_region, pd->config[MCH_HOST_BRDIGE_SMRAM],
                    mch->smm_enabled);
    memory_region_transaction_commit();
}

static void mch_set_smm(int smm, void *arg)
{
    MCHPCIState *mch = arg;
    PCIDevice *pd = PCI_DEVICE(mch);

    memory_region_transaction_begin();
    smram_set_smm(&mch->smm_enabled, smm, pd->config[MCH_HOST_BRDIGE_SMRAM],
                    &mch->smram_region);
    memory_region_transaction_commit();
}

static uint32_t mch_read_config(PCIDevice *d,
                                 uint32_t address, int len)
{

    uint32_t val;
	
	MCHPCIState *mch = MCH_PCI_DEVICE(d);
#ifdef CONFIG_INTEL_IGD_PASSTHROUGH
    
#define VID         0x0
#define DID         0x2
#define RID         0x8
#define HDR         0xE
#define SVID        0x2C
#define SID         0x2E
#define MCH_PAVAC   0x58

		switch (address)
		{
			case VID:		
			case DID:		
			case RID:		
			case HDR:		
			case SVID:			
			case SID:      	
			case MCH_PAVAC:	
				val = host_pci_read_config(d,
									   	address, len);
				break;
			default:
				val = pci_default_read_config(d, address, len);
	  	}
#else
	val = pci_default_read_config(d, address, len);
#endif

	Q35_DPRINTF("%s(%04x:%02x:%02x.%x, @0x%x, len=0x%x) %x\n", 
				__func__, 0x0000, 0x00, 
				PCI_SLOT(d->devfn), PCI_FUNC(d->devfn), 
				address, len, val);

	return val;
}
static void mch_write_config(PCIDevice *d,
                              uint32_t address, uint32_t val, int len)
{
    
	Q35_DPRINTF("%s(%04x:%02x:%02x.%x, @0x%x, len=0x%x) %x\n",                                                             
			__func__, 0x0000, 0x00,                                                                            
			PCI_SLOT(d->devfn), PCI_FUNC(d->devfn),                                                        
			address, len, val);
	MCHPCIState *mch = MCH_PCI_DEVICE(d);
#ifdef CONFIG_INTEL_IGD_PASSTHROUGH

	switch (address)
	{
		
	  case MCH_PAVAC:
		host_pci_write_config(d, address, len, val);
		return;
		break;
	}
#endif
 
    pci_default_write_config(d, address, val, len);

    if (ranges_overlap(address, len, MCH_HOST_BRIDGE_PAM0,
                       MCH_HOST_BRIDGE_PAM_SIZE)) {
        mch_update_pam(mch);
    }

    if (ranges_overlap(address, len, MCH_HOST_BRIDGE_PCIEXBAR,
                       MCH_HOST_BRIDGE_PCIEXBAR_SIZE)) {
        mch_update_pciexbar(mch);
    }

    if (ranges_overlap(address, len, MCH_HOST_BRDIGE_SMRAM,
                       MCH_HOST_BRDIGE_SMRAM_SIZE)) {
        mch_update_smram(mch);
    }
	
#ifdef CONFIG_INTEL_IGD_PASSTHROUGH
	if (ranges_overlap(address, len, 0xB0, 0x4)) {
        mch_update_gfx_stolen(d);
    }

	if (ranges_overlap(address, len, 0xB4, 0x4)) {
        mch_update_gfx_gtt_stolen(d);
    }
#endif
}

#ifdef CONFIG_INTEL_IGD_PASSTHROUGH

#define BUG_ON(cond)\
    assert(cond)

static void mch_init_gfx_gtt_stolen(PCIDevice *d)
{
	MCHPCIState *mch = MCH_PCI_DEVICE(d);

	unsigned short ggc = mch_read_config(d, 0x50, 2);
	unsigned short ggms = (ggc & Q35_MASK(16, 9, 8)) >> 8;
    Q35_DPRINTF("%s: ggc 0x%x, ggms 0x%x\n",__func__, ggc, ggms);

    BUG_ON((ggms <= 0x2) && "unexpected GGMS value\n" );

    mch->GFX_GTT_stolen_size = ggms << 20;
	mch->GFX_GTT_stolen_base = mch_read_config(d, 0xB4, 4) & (~0x1);
    
    memory_region_init_ram(&mch->GFX_GTT_stolen, OBJECT(mch), "gfx-gtt-region",
						 mch->GFX_GTT_stolen_size);
	memory_region_add_subregion_overlap(mch->system_memory, mch->GFX_GTT_stolen_base,
									&mch->GFX_GTT_stolen, 1);
	memory_region_set_enabled(&mch->GFX_GTT_stolen, false);
    
    pci_set_long(d->wmask + 0xB4, 0xffffffff);

	Q35_DPRINTF("GFX GTT Stolen size: %016llx, GFX GTT Stolen base: %016llX\n",
		        mch->GFX_GTT_stolen_base, mch->GFX_GTT_stolen_size);
}

static void mch_init_gfx_stolen(PCIDevice *d)
{
	MCHPCIState *mch = MCH_PCI_DEVICE(d);

	unsigned short ggc = mch_read_config(d, 0x50, 2);
	unsigned short gms = (ggc & Q35_MASK(16, 7, 3)) >> 3;
	
    Q35_DPRINTF("%s: ggc 0x%x, gms 0x%x\n", __func__, ggc, gms);
	BUG_ON((gms <= 0x10) && "unexpected GMS value");
    
    mch->GFX_stolen_size = gms << 25;
	mch->GFX_stolen_base = mch_read_config(d, 0xB0, 4) & (~0x1);

	memory_region_init_ram(&mch->GFX_stolen, OBJECT(mch), "gfx-region",
						 mch->GFX_stolen_size);
	memory_region_add_subregion_overlap(mch->system_memory, mch->GFX_stolen_base,
									&mch->GFX_stolen, 1);
	memory_region_set_enabled(&mch->GFX_stolen, false);

	pci_set_long(d->wmask + 0xB0, 0xffffffff);
	Q35_DPRINTF("GFX Stolen Base: %016llx, GFX_stolen_size: %016llX\n",
		        mch->GFX_stolen_base, mch->GFX_stolen_size);
	
}


static void mch_update_gfx_stolen(PCIDevice *d)
{
	MCHPCIState *mch = MCH_PCI_DEVICE(d);
    if(!mch->GFX_stolen_base) 
        mch_init_gfx_stolen(d);

	memory_region_transaction_begin();
	memory_region_set_enabled(&mch->GFX_stolen, true);
	memory_region_transaction_commit();
}
static void mch_update_gfx_gtt_stolen(PCIDevice *d)
{   
	MCHPCIState *mch = MCH_PCI_DEVICE(d);
    if(!mch->GFX_GTT_stolen_base)
        mch_init_gfx_gtt_stolen(d);

	memory_region_transaction_begin();
	memory_region_set_enabled(&mch->GFX_GTT_stolen, true);
	memory_region_transaction_commit();

}
#endif

static void mch_update(MCHPCIState *mch)
{
    mch_update_pciexbar(mch);
    mch_update_pam(mch);
    mch_update_smram(mch);
	
}

static int mch_post_load(void *opaque, int version_id)
{
    MCHPCIState *mch = opaque;
    mch_update(mch);
    return 0;
}

static const VMStateDescription vmstate_mch = {
    .name = "mch",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .post_load = mch_post_load,
    .fields = (VMStateField []) {
        VMSTATE_PCI_DEVICE(parent_obj, MCHPCIState),
        VMSTATE_UINT8(smm_enabled, MCHPCIState),
        VMSTATE_END_OF_LIST()
    }
};


static void mch_reset(DeviceState *qdev)
{
    PCIDevice *d = PCI_DEVICE(qdev);
    MCHPCIState *mch = MCH_PCI_DEVICE(d);

    pci_set_quad(d->config + MCH_HOST_BRIDGE_PCIEXBAR,
                 MCH_HOST_BRIDGE_PCIEXBAR_DEFAULT);

    d->config[MCH_HOST_BRDIGE_SMRAM] = MCH_HOST_BRIDGE_SMRAM_DEFAULT;

    mch_update(mch);
}

#ifdef CONFIG_INTEL_IGD_PASSTHROUGH

static inline void set_intel_config(PCIDevice *d)
{
	
	 MCHPCIState *mch = MCH_PCI_DEVICE(d);

    /* DEVEN */
    pci_set_long(d->wmask + D0F0_DEVEN, 0x0 | D0F0_DEVEN_D0EN|D0F0_DEVEN_D2EN|D0F0_DEVEN_D3EN);
    pci_set_long(d->config + D0F0_DEVEN, D0F0_DEVEN_D0EN | 
		D0F0_DEVEN_D2EN |
		D0F0_DEVEN_D3EN | D0F0_DEVEN_D1F0EN);
	
    /* DMIBAR */
    pci_set_quad(d->wmask + D0F0_DMIBAR, 
					(D0F0_DMIBAR_DMIBAR | D0F0_DMIBAR_DMIBAREN)); // set writable

    /* TSEG - G Memory Base Register */
    pci_set_long(d->wmask + D0F0_TSEG, 
					(D0F0_TSEG_TSEGMB | D0F0_TSEG_LOCK));

}

#endif

static int mch_init(PCIDevice *d)
{
    int i;
    MCHPCIState *mch = MCH_PCI_DEVICE(d);

#ifdef CONFIG_INTEL_IGD_PASSTHROUGH
		int fd;
		char dir[128], name[128];
		
		d->pt_domain = 0;
		d->pt_bus = 0;
		d->pt_devfn = PCI_DEVFN(0x0, 0);

		mch->smm_enabled = 1;	
		
		snprintf(dir, sizeof(dir), "/sys/bus/pci/devices/%04x:%02x:%02x.%x/",
				 0, 0, 0x0, 0);
		snprintf(name, sizeof(name), "%sconfig", dir);
	
		fd = open(name, O_RDONLY);
		if (fd < 0) {
			error_report("%s: Initializing pass through MCH failed\n", __func__);
			return -1;
		} else
			d->pt_dev_fd = fd;
        
        mch->GFX_stolen_size = 0;
        mch->GFX_stolen_base = 0;
        mch->GFX_GTT_stolen_size = 0;
        mch->GFX_GTT_stolen_base = 0;
#endif


    /* setup pci memory mapping */
    pc_pci_as_mapping_init(OBJECT(mch), mch->system_memory,
                           mch->pci_address_space);

    /* smram */
    cpu_smm_register(&mch_set_smm, mch);
    memory_region_init_alias(&mch->smram_region, OBJECT(mch), "smram-region",
                             mch->pci_address_space, 0xa0000, 0x20000);
    memory_region_add_subregion_overlap(mch->system_memory, 0xa0000,
                                        &mch->smram_region, 1);
	
    memory_region_set_enabled(&mch->smram_region, false);
	
    init_pam(DEVICE(mch), mch->ram_memory, mch->system_memory, mch->pci_address_space,
             &mch->pam_regions[0], PAM_BIOS_BASE, PAM_BIOS_SIZE);
    for (i = 0; i < 12; ++i) {
        init_pam(DEVICE(mch), mch->ram_memory, mch->system_memory, mch->pci_address_space,
                 &mch->pam_regions[i+1], PAM_EXPAN_BASE + i * PAM_EXPAN_SIZE,
                 PAM_EXPAN_SIZE);
    }

#ifdef CONFIG_INTEL_IGD_PASSTHROUGH
    /* FIXME: Move this to Seabios */
	set_intel_config(d);
#endif

    return 0;
}

uint64_t mch_mcfg_base(void)
{
    bool ambiguous;
    Object *o = object_resolve_path_type("", TYPE_MCH_PCI_DEVICE, &ambiguous);
    if (!o) {
        return 0;
    }
    return MCH_HOST_BRIDGE_PCIEXBAR_DEFAULT;
}

static void mch_class_init(ObjectClass *klass, void *data)
{
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);

    k->init = mch_init;
    k->config_write = mch_write_config;
	k->config_read = mch_read_config;
    dc->reset = mch_reset;
    set_bit(DEVICE_CATEGORY_BRIDGE, dc->categories);
    dc->desc = "Host bridge";
    dc->vmsd = &vmstate_mch;
    k->vendor_id = PCI_VENDOR_ID_INTEL;
#ifdef CONFIG_INTEL_IGD_PASSTHROUGH
	k->device_id = __host_pci_read_config(0, 0, 0, 0x02, 2);
    k->revision =  __host_pci_read_config(0, 0, 0, 0x08, 2);
#else
	k->device_id = PCI_DEVICE_ID_INTEL_Q35_MCH;
    k->revision = MCH_HOST_BRIDGE_REVISION_DEFAULT;
#endif
    k->class_id = PCI_CLASS_BRIDGE_HOST;
    /*
     * PCI-facing part of the host bridge, not usable without the
     * host-facing part, which can't be device_add'ed, yet.
     */
    dc->cannot_instantiate_with_device_add_yet = true;
}

static const TypeInfo mch_info = {
    .name = TYPE_MCH_PCI_DEVICE,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(MCHPCIState),
    .class_init = mch_class_init,
};

static void q35_register(void)
{
    type_register_static(&mch_info);
    type_register_static(&q35_host_info);
}

type_init(q35_register);
