/*
 * vfio based device assignment support
 *
 * Copyright Red Hat, Inc. 2012
 *
 * Authors:
 *  Alex Williamson <alex.williamson@redhat.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 * Based on qemu-kvm device-assignment:
 *  Adapted for KVM by Qumranet.
 *  Copyright (c) 2007, Neocleus, Alex Novik (alex@neocleus.com)
 *  Copyright (c) 2007, Neocleus, Guy Zana (guy@neocleus.com)
 *  Copyright (C) 2008, Qumranet, Amit Shah (amit.shah@qumranet.com)
 *  Copyright (C) 2008, Red Hat, Amit Shah (amit.shah@redhat.com)
 *  Copyright (C) 2008, IBM, Muli Ben-Yehuda (muli@il.ibm.com)
  * Copyright (C) 2013, QNAP, Henry Wu (henrywu@qnap.com)
 */

#include <dirent.h>
#include <linux/vfio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "exec/address-spaces.h"
#include "exec/memory.h"
#include "hw/pci/msi.h"
#include "hw/pci/msix.h"
#include "hw/pci/pci.h"
#include "qemu-common.h"
#include "qemu/error-report.h"
#include "qemu/event_notifier.h"
#include "qemu/queue.h"
#include "qemu/range.h"
#include "sysemu/kvm.h"
#include "sysemu/sysemu.h"
#include "hw/i386/pc.h"
#include "hw/loader.h"
#include "vfio-intel-igd.h"
#include "ui/console.h"
#include "hw/pci/pci_bus.h"
#include "hw/pci-host/q35.h"

#define DEBUG_VFIO
#ifdef DEBUG_VFIO
#define DPRINTF(fmt, ...) \
    do { fprintf(stderr, "vfio: " fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
    do { } while (0)
#endif

/* Extra debugging, trap acceleration paths for more logging */
#define VFIO_ALLOW_MMAP 1
#define VFIO_ALLOW_KVM_INTX 1
#define VFIO_ALLOW_KVM_MSI 1
#define VFIO_ALLOW_KVM_MSIX 1

struct VFIODevice;

typedef struct VFIOQuirk {
    MemoryRegion mem;
    struct VFIODevice *vdev;
    QLIST_ENTRY(VFIOQuirk) next;
    struct {
        uint32_t base_offset:TARGET_PAGE_BITS;
        uint32_t address_offset:TARGET_PAGE_BITS;
        uint32_t address_size:3;
        uint32_t bar:3;

        uint32_t address_match;
        uint32_t address_mask;

        uint32_t address_val:TARGET_PAGE_BITS;
        uint32_t data_offset:TARGET_PAGE_BITS;
        uint32_t data_size:3;

        uint8_t flags;
        uint8_t read_flags;
        uint8_t write_flags;
    } data;
} VFIOQuirk;

typedef struct VFIOBAR {
    off_t fd_offset; /* offset of BAR within device fd */
    int fd; /* device fd, allows us to pass VFIOBAR as opaque data */
    MemoryRegion mem; /* slow, read/write access */
    MemoryRegion mmap_mem; /* direct mapped access */
    void *mmap;
    size_t size;
    uint32_t flags; /* VFIO region flags (rd/wr/mmap) */
    uint8_t nr; /* cache the BAR number for debug */
    bool ioport;
    bool mem64;
    QLIST_HEAD(, VFIOQuirk) quirks;
} VFIOBAR;

typedef struct VFIOVGARegion {
    MemoryRegion mem;
    off_t offset;
    int nr;
    QLIST_HEAD(, VFIOQuirk) quirks;
} VFIOVGARegion;

typedef struct VFIOVGA {
    off_t fd_offset;
    int fd;
    VFIOVGARegion region[QEMU_PCI_VGA_NUM_REGIONS];
} VFIOVGA;

typedef struct VFIOINTx {
    bool pending; /* interrupt pending */
    bool kvm_accel; /* set when QEMU bypass through KVM enabled */
    uint8_t pin; /* which pin to pull for qemu_set_irq */
    EventNotifier interrupt; /* eventfd triggered on interrupt */
    EventNotifier unmask; /* eventfd for unmask on QEMU bypass */
    PCIINTxRoute route; /* routing info for QEMU bypass */
    uint32_t mmap_timeout; /* delay to re-enable mmaps after interrupt */
    QEMUTimer *mmap_timer; /* enable mmaps after periods w/o interrupts */
} VFIOINTx;

typedef struct VFIOMSIVector {
    EventNotifier interrupt; /* eventfd triggered on interrupt */
	EventNotifier kvm_interrupt; /* eventfd triggered for KVM irqfd bypass */
    struct VFIODevice *vdev; /* back pointer to device */
    MSIMessage msg; /* cache the MSI message so we know when it changes */
    int virq; /* KVM irqchip route for QEMU bypass */
    bool use;
} VFIOMSIVector;

enum {
    VFIO_INT_NONE = 0,
    VFIO_INT_INTx = 1,
    VFIO_INT_MSI  = 2,
    VFIO_INT_MSIX = 3,
};

struct VFIOGroup;

typedef struct VFIOType1 {
    MemoryListener listener;
    int error;
    bool initialized;
	bool enabled;
} VFIOType1;

typedef struct VFIOContainer {
    int fd; /* /dev/vfio/vfio, empowered by the attached groups */
    struct {
        /* enable abstraction to support various iommu backends */
        union {
            VFIOType1 type1;
        };
        void (*release)(struct VFIOContainer *);
    } iommu_data;
    QLIST_HEAD(, VFIOGroup) group_list;
    QLIST_ENTRY(VFIOContainer) next;
} VFIOContainer;

/* Cache of MSI-X setup plus extra mmap and memory region for split BAR map */
typedef struct VFIOMSIXInfo {
    uint8_t table_bar;
    uint8_t pba_bar;
    uint16_t entries;
    uint32_t table_offset;
    uint32_t pba_offset;
    MemoryRegion mmap_mem;
    void *mmap;
} VFIOMSIXInfo;

typedef struct VFIODevice {
    PCIDevice pdev;
    int fd;
    VFIOINTx intx;
    unsigned int config_size;
    uint8_t *emulated_config_bits; /* QEMU emulated bits, little-endian */
    off_t config_offset; /* Offset of config space region within device fd */
    unsigned int rom_size;
    off_t rom_offset; /* Offset of ROM region within device fd */
    void *rom;
    int msi_cap_size;
    VFIOMSIVector *msi_vectors;
    VFIOMSIXInfo *msix;
    int nr_vectors; /* Number of MSI/MSIX vectors currently in use */
    int interrupt; /* Current interrupt type */
    VFIOBAR bars[PCI_NUM_REGIONS - 1]; /* No ROM */
    VFIOVGA vga; /* 0xa0000, 0x3b0, 0x3c0 */
	MemoryRegion GFX_stolen;
    PCIHostDeviceAddress host;
    QLIST_ENTRY(VFIODevice) next;
    struct VFIOGroup *group;
    EventNotifier err_notifier;
    uint32_t features;
#define VFIO_FEATURE_ENABLE_VGA_BIT 1
#define VFIO_FEATURE_ENABLE_VGA (1 << VFIO_FEATURE_ENABLE_VGA_BIT)
    int32_t bootindex;
    uint8_t pm_cap;
    bool reset_works;
    bool has_vga;
    bool pci_aer;
    bool has_flr;
    bool has_pm_reset;
    bool needs_reset;
    bool rom_read_failed;
	ram_addr_t opRegion_base;
	Notifier pre_mapping_notifier;
	Notifier post_mapping_notifier;
	MemoryRegion *GFX_GTT_Stolen;
	ram_addr_t GFX_GTT_Stolen_base;
	GraphicHwOps hw_ops;
} VFIODevice;

typedef struct VFIOGroup {
    int fd;
    int groupid;
    VFIOContainer *container;
    QLIST_HEAD(, VFIODevice) device_list;
    QLIST_ENTRY(VFIOGroup) next;
    QLIST_ENTRY(VFIOGroup) container_next;
} VFIOGroup;

typedef struct VFIORomBlacklistEntry {
    uint16_t vendor_id;
    uint16_t device_id;
} VFIORomBlacklistEntry;

/*
 * List of device ids/vendor ids for which to disable
 * option rom loading. This avoids the guest hangs during rom
 * execution as noticed with the BCM 57810 card for lack of a
 * more better way to handle such issues.
 * The  user can still override by specifying a romfile or
 * rombar=1.
 * Please see https://bugs.launchpad.net/qemu/+bug/1284874
 * for an analysis of the 57810 card hang. When adding
 * a new vendor id/device id combination below, please also add
 * your card/environment details and information that could
 * help in debugging to the bug tracking this issue
 */
static const VFIORomBlacklistEntry romblacklist[] = {
    /* Broadcom BCM 57810 */
    { 0x14e4, 0x168e }
};

#define MSIX_CAP_LENGTH 12

static QLIST_HEAD(, VFIOContainer)
    container_list = QLIST_HEAD_INITIALIZER(container_list);

static QLIST_HEAD(, VFIOGroup)
    group_list = QLIST_HEAD_INITIALIZER(group_list);

#ifdef CONFIG_KVM
/*
 * We have a single VFIO pseudo device per KVM VM.  Once created it lives
 * for the life of the VM.  Closing the file descriptor only drops our
 * reference to it and the device's reference to kvm.  Therefore once
 * initialized, this file descriptor is only released on QEMU exit and
 * we'll re-use it should another vfio device be attached before then.
 */
static int vfio_kvm_device_fd = -1;
#endif

static void vfio_disable_interrupts(VFIODevice *vdev);
static uint32_t vfio_pci_read_config(PCIDevice *pdev, uint32_t addr, int len);
static void vfio_pci_write_config(PCIDevice *pdev, uint32_t addr,
                                  uint32_t val, int len);
static void vfio_mmap_set_enabled(VFIODevice *vdev, bool enabled);



/*
 * Common VFIO interrupt disable
 */
static void vfio_disable_irqindex(VFIODevice *vdev, int index)
{
    struct vfio_irq_set irq_set = {
        .argsz = sizeof(irq_set),
        .flags = VFIO_IRQ_SET_DATA_NONE | VFIO_IRQ_SET_ACTION_TRIGGER,
        .index = index,
        .start = 0,
        .count = 0,
    };

    ioctl(vdev->fd, VFIO_DEVICE_SET_IRQS, &irq_set);
}

/*
 * INTx
 */
static void vfio_unmask_intx(VFIODevice *vdev)
{
    struct vfio_irq_set irq_set = {
        .argsz = sizeof(irq_set),
        .flags = VFIO_IRQ_SET_DATA_NONE | VFIO_IRQ_SET_ACTION_UNMASK,
        .index = VFIO_PCI_INTX_IRQ_INDEX,
        .start = 0,
        .count = 1,
    };

    ioctl(vdev->fd, VFIO_DEVICE_SET_IRQS, &irq_set);
}

#ifdef CONFIG_KVM /* Unused outside of CONFIG_KVM code */
static void vfio_mask_intx(VFIODevice *vdev)
{
    struct vfio_irq_set irq_set = {
        .argsz = sizeof(irq_set),
        .flags = VFIO_IRQ_SET_DATA_NONE | VFIO_IRQ_SET_ACTION_MASK,
        .index = VFIO_PCI_INTX_IRQ_INDEX,
        .start = 0,
        .count = 1,
    };

    ioctl(vdev->fd, VFIO_DEVICE_SET_IRQS, &irq_set);
}
#endif

/*
 * Disabling BAR mmaping can be slow, but toggling it around INTx can
 * also be a huge overhead.  We try to get the best of both worlds by
 * waiting until an interrupt to disable mmaps (subsequent transitions
 * to the same state are effectively no overhead).  If the interrupt has
 * been serviced and the time gap is long enough, we re-enable mmaps for
 * performance.  This works well for things like graphics cards, which
 * may not use their interrupt at all and are penalized to an unusable
 * level by read/write BAR traps.  Other devices, like NICs, have more
 * regular interrupts and see much better latency by staying in non-mmap
 * mode.  We therefore set the default mmap_timeout such that a ping
 * is just enough to keep the mmap disabled.  Users can experiment with
 * other options with the x-intx-mmap-timeout-ms parameter (a value of
 * zero disables the timer).
 */
static void vfio_intx_mmap_enable(void *opaque)
{
    VFIODevice *vdev = opaque;

    if (vdev->intx.pending) {
        timer_mod(vdev->intx.mmap_timer,
                       qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + vdev->intx.mmap_timeout);
        return;
    }

    vfio_mmap_set_enabled(vdev, true);
}

static void vfio_intx_interrupt(void *opaque)
{
    VFIODevice *vdev = opaque;

    if (!event_notifier_test_and_clear(&vdev->intx.interrupt)) {
        return;
    }

    DPRINTF("%s(%04x:%02x:%02x.%x) Pin %c\n", __func__, vdev->host.domain,
            vdev->host.bus, vdev->host.slot, vdev->host.function,
            'A' + vdev->intx.pin);

    vdev->intx.pending = true;
    pci_irq_assert(&vdev->pdev);
    vfio_mmap_set_enabled(vdev, false);
    if (vdev->intx.mmap_timeout) {
        timer_mod(vdev->intx.mmap_timer,
                       qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + vdev->intx.mmap_timeout);
    }
}

static void vfio_eoi(VFIODevice *vdev)
{
    if (!vdev->intx.pending) {
        return;
    }

    DPRINTF("%s(%04x:%02x:%02x.%x) EOI\n", __func__, vdev->host.domain,
            vdev->host.bus, vdev->host.slot, vdev->host.function);

    vdev->intx.pending = false;
    pci_irq_deassert(&vdev->pdev);
    vfio_unmask_intx(vdev);
}

static void vfio_enable_intx_kvm(VFIODevice *vdev)
{
#ifdef CONFIG_KVM
    struct kvm_irqfd irqfd = {
        .fd = event_notifier_get_fd(&vdev->intx.interrupt),
        .gsi = vdev->intx.route.irq,
        .flags = KVM_IRQFD_FLAG_RESAMPLE,
    };
    struct vfio_irq_set *irq_set;
    int ret, argsz;
    int32_t *pfd;

    if (!VFIO_ALLOW_KVM_INTX || !kvm_irqfds_enabled() ||
        vdev->intx.route.mode != PCI_INTX_ENABLED ||
        !kvm_check_extension(kvm_state, KVM_CAP_IRQFD_RESAMPLE)) {
        return;
    }

    /* Get to a known interrupt state */
    qemu_set_fd_handler(irqfd.fd, NULL, NULL, vdev);
    vfio_mask_intx(vdev);
    vdev->intx.pending = false;
    pci_irq_deassert(&vdev->pdev);

    /* Get an eventfd for resample/unmask */
    if (event_notifier_init(&vdev->intx.unmask, 0)) {
        error_report("vfio: Error: event_notifier_init failed eoi");
        goto fail;
    }

    /* KVM triggers it, VFIO listens for it */
    irqfd.resamplefd = event_notifier_get_fd(&vdev->intx.unmask);

    if (kvm_vm_ioctl(kvm_state, KVM_IRQFD, &irqfd)) {
        error_report("vfio: Error: Failed to setup resample irqfd: %m");
        goto fail_irqfd;
    }

    argsz = sizeof(*irq_set) + sizeof(*pfd);

    irq_set = g_malloc0(argsz);
    irq_set->argsz = argsz;
    irq_set->flags = VFIO_IRQ_SET_DATA_EVENTFD | VFIO_IRQ_SET_ACTION_UNMASK;
    irq_set->index = VFIO_PCI_INTX_IRQ_INDEX;
    irq_set->start = 0;
    irq_set->count = 1;
    pfd = (int32_t *)&irq_set->data;

    *pfd = irqfd.resamplefd;

    ret = ioctl(vdev->fd, VFIO_DEVICE_SET_IRQS, irq_set);
    g_free(irq_set);
    if (ret) {
        error_report("vfio: Error: Failed to setup INTx unmask fd: %m");
        goto fail_vfio;
    }

    /* Let'em rip */
    vfio_unmask_intx(vdev);

    vdev->intx.kvm_accel = true;

    DPRINTF("%s(%04x:%02x:%02x.%x) KVM INTx accel enabled\n",
            __func__, vdev->host.domain, vdev->host.bus,
            vdev->host.slot, vdev->host.function);

    return;

fail_vfio:
    irqfd.flags = KVM_IRQFD_FLAG_DEASSIGN;
    kvm_vm_ioctl(kvm_state, KVM_IRQFD, &irqfd);
fail_irqfd:
    event_notifier_cleanup(&vdev->intx.unmask);
fail:
    qemu_set_fd_handler(irqfd.fd, vfio_intx_interrupt, NULL, vdev);
    vfio_unmask_intx(vdev);
#endif
}

static void vfio_disable_intx_kvm(VFIODevice *vdev)
{
#ifdef CONFIG_KVM
    struct kvm_irqfd irqfd = {
        .fd = event_notifier_get_fd(&vdev->intx.interrupt),
        .gsi = vdev->intx.route.irq,
        .flags = KVM_IRQFD_FLAG_DEASSIGN,
    };

    if (!vdev->intx.kvm_accel) {
        return;
    }

    /*
     * Get to a known state, hardware masked, QEMU ready to accept new
     * interrupts, QEMU IRQ de-asserted.
     */
    vfio_mask_intx(vdev);
    vdev->intx.pending = false;
    pci_irq_deassert(&vdev->pdev);

    /* Tell KVM to stop listening for an INTx irqfd */
    if (kvm_vm_ioctl(kvm_state, KVM_IRQFD, &irqfd)) {
        error_report("vfio: Error: Failed to disable INTx irqfd: %m");
    }

    /* We only need to close the eventfd for VFIO to cleanup the kernel side */
    event_notifier_cleanup(&vdev->intx.unmask);

    /* QEMU starts listening for interrupt events. */
    qemu_set_fd_handler(irqfd.fd, vfio_intx_interrupt, NULL, vdev);

    vdev->intx.kvm_accel = false;

    /* If we've missed an event, let it re-fire through QEMU */
    vfio_unmask_intx(vdev);

    DPRINTF("%s(%04x:%02x:%02x.%x) KVM INTx accel disabled\n",
            __func__, vdev->host.domain, vdev->host.bus,
            vdev->host.slot, vdev->host.function);
#endif
}

static void vfio_update_irq(PCIDevice *pdev)
{
    VFIODevice *vdev = DO_UPCAST(VFIODevice, pdev, pdev);
    PCIINTxRoute route;

    if (vdev->interrupt != VFIO_INT_INTx) {
        return;
    }

    route = pci_device_route_intx_to_irq(&vdev->pdev, vdev->intx.pin);

    if (!pci_intx_route_changed(&vdev->intx.route, &route)) {
        return; /* Nothing changed */
    }

    DPRINTF("%s(%04x:%02x:%02x.%x) IRQ moved %d -> %d\n", __func__,
            vdev->host.domain, vdev->host.bus, vdev->host.slot,
            vdev->host.function, vdev->intx.route.irq, route.irq);

    vfio_disable_intx_kvm(vdev);

    vdev->intx.route = route;

    if (route.mode != PCI_INTX_ENABLED) {
        return;
    }

    vfio_enable_intx_kvm(vdev);

    /* Re-enable the interrupt in cased we missed an EOI */
    vfio_eoi(vdev);
}

static int vfio_enable_intx(VFIODevice *vdev)
{
    uint8_t pin = vfio_pci_read_config(&vdev->pdev, PCI_INTERRUPT_PIN, 1);
    int ret, argsz;
    struct vfio_irq_set *irq_set;
    int32_t *pfd;

    if (!pin) {
        return 0;
    }

    vfio_disable_interrupts(vdev);

    vdev->intx.pin = pin - 1; /* Pin A (1) -> irq[0] */
    pci_config_set_interrupt_pin(vdev->pdev.config, pin);

#ifdef CONFIG_KVM
    /*
     * Only conditional to avoid generating error messages on platforms
     * where we won't actually use the result anyway.
     */
    if (kvm_irqfds_enabled() &&
        kvm_check_extension(kvm_state, KVM_CAP_IRQFD_RESAMPLE)) {
        vdev->intx.route = pci_device_route_intx_to_irq(&vdev->pdev,
                                                        vdev->intx.pin);
    }
#endif

    ret = event_notifier_init(&vdev->intx.interrupt, 0);
    if (ret) {
        error_report("vfio: Error: event_notifier_init failed");
        return ret;
    }

    argsz = sizeof(*irq_set) + sizeof(*pfd);

    irq_set = g_malloc0(argsz);
    irq_set->argsz = argsz;
    irq_set->flags = VFIO_IRQ_SET_DATA_EVENTFD | VFIO_IRQ_SET_ACTION_TRIGGER;
    irq_set->index = VFIO_PCI_INTX_IRQ_INDEX;
    irq_set->start = 0;
    irq_set->count = 1;
    pfd = (int32_t *)&irq_set->data;

    *pfd = event_notifier_get_fd(&vdev->intx.interrupt);
    qemu_set_fd_handler(*pfd, vfio_intx_interrupt, NULL, vdev);

    ret = ioctl(vdev->fd, VFIO_DEVICE_SET_IRQS, irq_set);
    g_free(irq_set);
    if (ret) {
        error_report("vfio: Error: Failed to setup INTx fd: %m");
        qemu_set_fd_handler(*pfd, NULL, NULL, vdev);
        event_notifier_cleanup(&vdev->intx.interrupt);
        return -errno;
    }

    vfio_enable_intx_kvm(vdev);

    vdev->interrupt = VFIO_INT_INTx;

    DPRINTF("%s(%04x:%02x:%02x.%x)\n", __func__, vdev->host.domain,
            vdev->host.bus, vdev->host.slot, vdev->host.function);

    return 0;
}

static void vfio_disable_intx(VFIODevice *vdev)
{
    int fd;

    timer_del(vdev->intx.mmap_timer);
    vfio_disable_intx_kvm(vdev);
    vfio_disable_irqindex(vdev, VFIO_PCI_INTX_IRQ_INDEX);
    vdev->intx.pending = false;
    pci_irq_deassert(&vdev->pdev);
    vfio_mmap_set_enabled(vdev, true);

    fd = event_notifier_get_fd(&vdev->intx.interrupt);
    qemu_set_fd_handler(fd, NULL, NULL, vdev);
    event_notifier_cleanup(&vdev->intx.interrupt);

    vdev->interrupt = VFIO_INT_NONE;

    DPRINTF("%s(%04x:%02x:%02x.%x)\n", __func__, vdev->host.domain,
            vdev->host.bus, vdev->host.slot, vdev->host.function);
}

/*
 * MSI/X
 */
static void vfio_msi_interrupt(void *opaque)
{
    VFIOMSIVector *vector = opaque;
    VFIODevice *vdev = vector->vdev;
    int nr = vector - vdev->msi_vectors;

    if (!event_notifier_test_and_clear(&vector->interrupt)) {
        return;
    }

#ifdef DEBUG_VFIO
    MSIMessage msg;

    if (vdev->interrupt == VFIO_INT_MSIX) {
        msg = msix_get_message(&vdev->pdev, nr);
    } else if (vdev->interrupt == VFIO_INT_MSI) {
        msg = msi_get_message(&vdev->pdev, nr);
    } else {
        abort();
    }

    DPRINTF("%s(%04x:%02x:%02x.%x) vector %d 0x%"PRIx64"/0x%x\n", __func__,
            vdev->host.domain, vdev->host.bus, vdev->host.slot,
            vdev->host.function, nr, msg.address, msg.data);
#endif

    if (vdev->interrupt == VFIO_INT_MSIX) {
        msix_notify(&vdev->pdev, nr);
    } else if (vdev->interrupt == VFIO_INT_MSI) {
        msi_notify(&vdev->pdev, nr);
    } else {
        error_report("vfio: MSI interrupt receieved, but not enabled?");
    }
}

static int vfio_enable_vectors(VFIODevice *vdev, bool msix)
{
    struct vfio_irq_set *irq_set;
    int ret = 0, i, argsz;
    int32_t *fds;

    argsz = sizeof(*irq_set) + (vdev->nr_vectors * sizeof(*fds));

    irq_set = g_malloc0(argsz);
    irq_set->argsz = argsz;
    irq_set->flags = VFIO_IRQ_SET_DATA_EVENTFD | VFIO_IRQ_SET_ACTION_TRIGGER;
    irq_set->index = msix ? VFIO_PCI_MSIX_IRQ_INDEX : VFIO_PCI_MSI_IRQ_INDEX;
    irq_set->start = 0;
    irq_set->count = vdev->nr_vectors;
    fds = (int32_t *)&irq_set->data;

    for (i = 0; i < vdev->nr_vectors; i++) {
        if (!vdev->msi_vectors[i].use) {
            fds[i] = -1;
            continue;
        }

        fds[i] = event_notifier_get_fd(&vdev->msi_vectors[i].interrupt);
    }

    ret = ioctl(vdev->fd, VFIO_DEVICE_SET_IRQS, irq_set);

    g_free(irq_set);

    return ret;
}

static int vfio_msix_vector_do_use(PCIDevice *pdev, unsigned int nr,
                                   MSIMessage *msg, IOHandler *handler)
{
    VFIODevice *vdev = DO_UPCAST(VFIODevice, pdev, pdev);
    VFIOMSIVector *vector;
    int ret;

    DPRINTF("%s(%04x:%02x:%02x.%x) vector %d used\n", __func__,
            vdev->host.domain, vdev->host.bus, vdev->host.slot,
            vdev->host.function, nr);

    vector = &vdev->msi_vectors[nr];
    vector->vdev = vdev;
    vector->use = true;

    msix_vector_use(pdev, nr);

    if (event_notifier_init(&vector->interrupt, 0)) {
        error_report("vfio: Error: event_notifier_init failed");
    }

    /*
     * Attempt to enable route through KVM irqchip,
     * default to userspace handling if unavailable.
     */
    vector->virq = msg && VFIO_ALLOW_KVM_MSIX ?
                   kvm_irqchip_add_msi_route(kvm_state, *msg) : -1;
    if (vector->virq < 0 ||
        kvm_irqchip_add_irqfd_notifier(kvm_state, &vector->interrupt,
                                       NULL, vector->virq) < 0) {
        if (vector->virq >= 0) {
            kvm_irqchip_release_virq(kvm_state, vector->virq);
            vector->virq = -1;
        }
        qemu_set_fd_handler(event_notifier_get_fd(&vector->interrupt),
                            handler, NULL, vector);
    }

    /*
     * We don't want to have the host allocate all possible MSI vectors
     * for a device if they're not in use, so we shutdown and incrementally
     * increase them as needed.
     */
    if (vdev->nr_vectors < nr + 1) {
        vfio_disable_irqindex(vdev, VFIO_PCI_MSIX_IRQ_INDEX);
        vdev->nr_vectors = nr + 1;
        ret = vfio_enable_vectors(vdev, true);
        if (ret) {
            error_report("vfio: failed to enable vectors, %d", ret);
        }
    } else {
        int argsz;
        struct vfio_irq_set *irq_set;
        int32_t *pfd;

        argsz = sizeof(*irq_set) + sizeof(*pfd);

        irq_set = g_malloc0(argsz);
        irq_set->argsz = argsz;
        irq_set->flags = VFIO_IRQ_SET_DATA_EVENTFD |
                         VFIO_IRQ_SET_ACTION_TRIGGER;
        irq_set->index = VFIO_PCI_MSIX_IRQ_INDEX;
        irq_set->start = nr;
        irq_set->count = 1;
        pfd = (int32_t *)&irq_set->data;

        *pfd = event_notifier_get_fd(&vector->interrupt);

        ret = ioctl(vdev->fd, VFIO_DEVICE_SET_IRQS, irq_set);
        g_free(irq_set);
        if (ret) {
            error_report("vfio: failed to modify vector, %d", ret);
        }
    }

    return 0;
}

static int vfio_msix_vector_use(PCIDevice *pdev,
                                unsigned int nr, MSIMessage msg)
{
    return vfio_msix_vector_do_use(pdev, nr, &msg, vfio_msi_interrupt);
}

static void vfio_msix_vector_release(PCIDevice *pdev, unsigned int nr)
{
    VFIODevice *vdev = DO_UPCAST(VFIODevice, pdev, pdev);
    VFIOMSIVector *vector = &vdev->msi_vectors[nr];
    int argsz;
    struct vfio_irq_set *irq_set;
    int32_t *pfd;

    DPRINTF("%s(%04x:%02x:%02x.%x) vector %d released\n", __func__,
            vdev->host.domain, vdev->host.bus, vdev->host.slot,
            vdev->host.function, nr);

    /*
     * XXX What's the right thing to do here?  This turns off the interrupt
     * completely, but do we really just want to switch the interrupt to
     * bouncing through userspace and let msix.c drop it?  Not sure.
     */
    msix_vector_unuse(pdev, nr);

    argsz = sizeof(*irq_set) + sizeof(*pfd);

    irq_set = g_malloc0(argsz);
    irq_set->argsz = argsz;
    irq_set->flags = VFIO_IRQ_SET_DATA_EVENTFD |
                     VFIO_IRQ_SET_ACTION_TRIGGER;
    irq_set->index = VFIO_PCI_MSIX_IRQ_INDEX;
    irq_set->start = nr;
    irq_set->count = 1;
    pfd = (int32_t *)&irq_set->data;

    *pfd = -1;

    ioctl(vdev->fd, VFIO_DEVICE_SET_IRQS, irq_set);

    g_free(irq_set);

    if (vector->virq < 0) {
        qemu_set_fd_handler(event_notifier_get_fd(&vector->interrupt),
                            NULL, NULL, NULL);
    } else {
        kvm_irqchip_remove_irqfd_notifier(kvm_state, &vector->interrupt,
                                          vector->virq);
        kvm_irqchip_release_virq(kvm_state, vector->virq);
        vector->virq = -1;
    }

    event_notifier_cleanup(&vector->interrupt);
    vector->use = false;
}

static void vfio_enable_msix(VFIODevice *vdev)
{
    vfio_disable_interrupts(vdev);

    vdev->msi_vectors = g_malloc0(vdev->msix->entries * sizeof(VFIOMSIVector));

    vdev->interrupt = VFIO_INT_MSIX;

    /*
     * Some communication channels between VF & PF or PF & fw rely on the
     * physical state of the device and expect that enabling MSI-X from the
     * guest enables the same on the host.  When our guest is Linux, the
     * guest driver call to pci_enable_msix() sets the enabling bit in the
     * MSI-X capability, but leaves the vector table masked.  We therefore
     * can't rely on a vector_use callback (from request_irq() in the guest)
     * to switch the physical device into MSI-X mode because that may come a
     * long time after pci_enable_msix().  This code enables vector 0 with
     * triggering to userspace, then immediately release the vector, leaving
     * the physical device with no vectors enabled, but MSI-X enabled, just
     * like the guest view.
     */
    vfio_msix_vector_do_use(&vdev->pdev, 0, NULL, NULL);
    vfio_msix_vector_release(&vdev->pdev, 0);

    if (msix_set_vector_notifiers(&vdev->pdev, vfio_msix_vector_use,
                                  vfio_msix_vector_release, NULL)) {
        error_report("vfio: msix_set_vector_notifiers failed");
    }

    DPRINTF("%s(%04x:%02x:%02x.%x)\n", __func__, vdev->host.domain,
            vdev->host.bus, vdev->host.slot, vdev->host.function);
}


	
static void vfio_enable_msi(VFIODevice *vdev)
{
    int ret, i;

    vfio_disable_interrupts(vdev);

    vdev->nr_vectors = msi_nr_vectors_allocated(&vdev->pdev);
	
retry:
    vdev->msi_vectors = g_malloc0(vdev->nr_vectors * sizeof(VFIOMSIVector));

    for (i = 0; i < vdev->nr_vectors; i++) {
        VFIOMSIVector *vector = &vdev->msi_vectors[i];

        vector->vdev = vdev;
        vector->use = true;

        if (event_notifier_init(&vector->interrupt, 0)) {
            error_report("vfio: Error: event_notifier_init failed");
        }

        vector->msg = msi_get_message(&vdev->pdev, i);

        /*
         * Attempt to enable route through KVM irqchip,
         * default to userspace handling if unavailable.
         */
        vector->virq = VFIO_ALLOW_KVM_MSI ?
                       kvm_irqchip_add_msi_route(kvm_state, vector->msg) : -1;
        if (vector->virq < 0 ||
            kvm_irqchip_add_irqfd_notifier(kvm_state, &vector->interrupt,
                                           NULL, vector->virq) < 0) {
            DPRINTF("%s(%04x:%02x:%02x.%x) set irq handler to userspace handler\n", 
				__func__,
            	vdev->host.domain, vdev->host.bus, vdev->host.slot,
            	vdev->host.function);
            qemu_set_fd_handler(event_notifier_get_fd(&vector->interrupt),
                                vfio_msi_interrupt, NULL, vector);
        }
    }

    ret = vfio_enable_vectors(vdev, false);
    if (ret) {
        if (ret < 0) {
            error_report("vfio: Error: Failed to setup MSI fds: %m");
        } else if (ret != vdev->nr_vectors) {
            error_report("vfio: Error: Failed to enable %d "
                         "MSI vectors, retry with %d", vdev->nr_vectors, ret);
        }

        for (i = 0; i < vdev->nr_vectors; i++) {
            VFIOMSIVector *vector = &vdev->msi_vectors[i];
            if (vector->virq >= 0) {
                kvm_irqchip_remove_irqfd_notifier(kvm_state, &vector->interrupt,
                                                  vector->virq);
                kvm_irqchip_release_virq(kvm_state, vector->virq);
                vector->virq = -1;
            } else {
                qemu_set_fd_handler(event_notifier_get_fd(&vector->interrupt),
                                    NULL, NULL, NULL);
            }
            event_notifier_cleanup(&vector->interrupt);
        }

        g_free(vdev->msi_vectors);

        if (ret > 0 && ret != vdev->nr_vectors) {
            vdev->nr_vectors = ret;
            goto retry;
        }
        vdev->nr_vectors = 0;

        return;
    }

    vdev->interrupt = VFIO_INT_MSI;

    DPRINTF("%s(%04x:%02x:%02x.%x) Enabled %d MSI vectors\n", __func__,
            vdev->host.domain, vdev->host.bus, vdev->host.slot,
            vdev->host.function, vdev->nr_vectors);
}

static void vfio_disable_msi_common(VFIODevice *vdev)
{
    g_free(vdev->msi_vectors);
    vdev->msi_vectors = NULL;
    vdev->nr_vectors = 0;
    vdev->interrupt = VFIO_INT_NONE;

    vfio_enable_intx(vdev);
}

static void vfio_disable_msix(VFIODevice *vdev)
{
    int i;

    msix_unset_vector_notifiers(&vdev->pdev);

    /*
     * MSI-X will only release vectors if MSI-X is still enabled on the
     * device, check through the rest and release it ourselves if necessary.
     */
    for (i = 0; i < vdev->nr_vectors; i++) {
        if (vdev->msi_vectors[i].use) {
            vfio_msix_vector_release(&vdev->pdev, i);
        }
    }

    if (vdev->nr_vectors) {
        vfio_disable_irqindex(vdev, VFIO_PCI_MSIX_IRQ_INDEX);
    }

    vfio_disable_msi_common(vdev);

    DPRINTF("%s(%04x:%02x:%02x.%x)\n", __func__, vdev->host.domain,
            vdev->host.bus, vdev->host.slot, vdev->host.function);
}

static void vfio_disable_msi(VFIODevice *vdev)
{
    int i;

    vfio_disable_irqindex(vdev, VFIO_PCI_MSI_IRQ_INDEX);

    for (i = 0; i < vdev->nr_vectors; i++) {
        VFIOMSIVector *vector = &vdev->msi_vectors[i];

        if (!vector->use) {
            continue;
        }

        if (vector->virq >= 0) {
            kvm_irqchip_remove_irqfd_notifier(kvm_state,
                                              &vector->interrupt, vector->virq);
            kvm_irqchip_release_virq(kvm_state, vector->virq);
            vector->virq = -1;
        } else {
            qemu_set_fd_handler(event_notifier_get_fd(&vector->interrupt),
                                NULL, NULL, NULL);
        }

        event_notifier_cleanup(&vector->interrupt);
    }

    vfio_disable_msi_common(vdev);

    DPRINTF("%s(%04x:%02x:%02x.%x)\n", __func__, vdev->host.domain,
            vdev->host.bus, vdev->host.slot, vdev->host.function);
}

static void vfio_update_msi(VFIODevice *vdev)
{
    int i;

    for (i = 0; i < vdev->nr_vectors; i++) {
        VFIOMSIVector *vector = &vdev->msi_vectors[i];
        MSIMessage msg;

        if (!vector->use || vector->virq < 0) {
            continue;
        }

        msg = msi_get_message(&vdev->pdev, i);

        if (msg.address != vector->msg.address ||
            msg.data != vector->msg.data) {

            DPRINTF("%s(%04x:%02x:%02x.%x) MSI vector %d changed\n",
                    __func__, vdev->host.domain, vdev->host.bus,
                    vdev->host.slot, vdev->host.function, i);

            kvm_irqchip_update_msi_route(kvm_state, vector->virq, msg);
            vector->msg = msg;
        }
    }
}

#include "debug_mmio.h"


/*
 * IO Port/MMIO - Beware of the endians, VFIO is always little endian
 */
static void vfio_bar_write(void *opaque, hwaddr addr,
                           uint64_t data, unsigned size)
{
    VFIOBAR *bar = opaque;
    union {
        uint8_t byte;
        uint16_t word;
        uint32_t dword;
        uint64_t qword;
    } buf;

    switch (size) {
    case 1:
        buf.byte = data;
        break;
    case 2:
        buf.word = cpu_to_le16(data);
        break;
    case 4:
        buf.dword = cpu_to_le32(data);
        break;
    default:
        hw_error("vfio: unsupported write size, %d bytes", size);
        break;
    }

    if (pwrite(bar->fd, &buf, size, bar->fd_offset + addr) != size) {
        error_report("%s(,0x%"HWADDR_PRIx", 0x%"PRIx64", %d) failed: %m",
                     __func__, addr, data, size);
    }
	
#if 0

//#ifdef DEBUG_VFIO
    {
        VFIODevice *vdev = container_of(bar, VFIODevice, bars[bar->nr]);

        DPRINTF("%s(%04x:%02x:%02x.%x:BAR%d+0x%"HWADDR_PRIx", 0x%"PRIx64
                ", %d)\n", __func__, vdev->host.domain, vdev->host.bus,
                vdev->host.slot, vdev->host.function, bar->nr, addr,
                data, size);
    }
//#ifdef DEBUG_IGD_MMIO
//if(bar->fd_offset == 0 )
//	debug_igd_mmio(addr, size, data, 1);

#endif

    /*
     * A read or write to a BAR always signals an INTx EOI.  This will
     * do nothing if not pending (including not in INTx mode).  We assume
     * that a BAR access is in response to an interrupt and that BAR
     * accesses will service the interrupt.  Unfortunately, we don't know
     * which access will service the interrupt, so we're potentially
     * getting quite a few host interrupts per guest interrupt.
     */
    vfio_eoi(container_of(bar, VFIODevice, bars[bar->nr]));
}

static uint64_t vfio_bar_read(void *opaque,
                              hwaddr addr, unsigned size)
{
    VFIOBAR *bar = opaque;
    union {
        uint8_t byte;
        uint16_t word;
        uint32_t dword;
        uint64_t qword;
    } buf;
    uint64_t data = 0;

    if (pread(bar->fd, &buf, size, bar->fd_offset + addr) != size) {
        error_report("%s(,0x%"HWADDR_PRIx", %d) failed: %m",
                     __func__, addr, size);
        return (uint64_t)-1;
    }

    switch (size) {
    case 1:
        data = buf.byte;
        break;
    case 2:
        data = le16_to_cpu(buf.word);
        break;
    case 4:
        data = le32_to_cpu(buf.dword);
        break;
    default:
        hw_error("vfio: unsupported read size, %d bytes", size);
        break;
    }
	
#if 0
//#ifdef DEBUG_VFIO
    {
        VFIODevice *vdev = container_of(bar, VFIODevice, bars[bar->nr]);

        DPRINTF("%s(%04x:%02x:%02x.%x:BAR%d+0x%"HWADDR_PRIx
                ", %d) = 0x%"PRIx64"\n", __func__, vdev->host.domain,
                vdev->host.bus, vdev->host.slot, vdev->host.function,
                bar->nr, addr, size, data);
    }
//#ifdef DEBUG_IGD_MMIO
//if(bar->fd_offset == 0 )
//	debug_igd_mmio(addr, size, data,0);

#endif


    /* Same as write above */
    vfio_eoi(container_of(bar, VFIODevice, bars[bar->nr]));

    return data;
}

static const MemoryRegionOps vfio_bar_ops = {
    .read = vfio_bar_read,
    .write = vfio_bar_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};


static void vfio_vga_write(void *opaque, hwaddr addr,
                           uint64_t data, unsigned size)
{
    VFIOVGARegion *region = opaque;
    VFIOVGA *vga = container_of(region, VFIOVGA, region[region->nr]);
    union {
        uint8_t byte;
        uint16_t word;
        uint32_t dword;
        uint64_t qword;
    } buf;
    off_t offset = vga->fd_offset + region->offset + addr;

    switch (size) {
    case 1:
        buf.byte = data;
        break;
    case 2:
        buf.word = cpu_to_le16(data);
        break;
    case 4:
        buf.dword = cpu_to_le32(data);
        break;
    default:
        hw_error("vfio: unsupported write size, %d bytes\n", size);
        break;
    }

    if (pwrite(vga->fd, &buf, size, offset) != size) {
        error_report("%s(,0x%"HWADDR_PRIx", 0x%"PRIx64", %d) failed: %m",
                     __func__, region->offset + addr, data, size);
    }
	
#if 0
    //DPRINTF("%s(0x%"HWADDR_PRIx", 0x%"PRIx64", %d)\n",
    //       __func__, region->offset + addr, data, size);
	debug_vga(addr, size, data, 1);
#endif

}

static uint64_t vfio_vga_read(void *opaque, hwaddr addr, unsigned size)
{
    VFIOVGARegion *region = opaque;
    VFIOVGA *vga = container_of(region, VFIOVGA, region[region->nr]);
    union {
        uint8_t byte;
        uint16_t word;
        uint32_t dword;
        uint64_t qword;
    } buf;
    uint64_t data = 0;
    off_t offset = vga->fd_offset + region->offset + addr;

    if (pread(vga->fd, &buf, size, offset) != size) {
        error_report("%s(,0x%"HWADDR_PRIx", %d) failed: %m",
                     __func__, region->offset + addr, size);
        return (uint64_t)-1;
    }

    switch (size) {
    case 1:
        data = buf.byte;
        break;
    case 2:
        data = le16_to_cpu(buf.word);
        break;
    case 4:
        data = le32_to_cpu(buf.dword);
        break;
    default:
        hw_error("vfio: unsupported read size, %d bytes\n", size);
        break;
    }
#if 0
	debug_vga(addr, size, data, 0);

   // DPRINTF("%s(0x%"HWADDR_PRIx", %d) = 0x%"PRIx64"\n",
     //       __func__, region->offset + addr, size, data);
#endif
    return data;
}



/*
 * Device specific quirks
 */

/* Is range1 fully contained within range2?  */
static bool vfio_range_contained(uint64_t first1, uint64_t len1,
                                 uint64_t first2, uint64_t len2) {
    return (first1 >= first2 && first1 + len1 <= first2 + len2);
}

static bool vfio_flags_enabled(uint8_t flags, uint8_t mask)
{
    return (mask && (flags & mask) == mask);
}


static void vfio_vga_quirk_teardown(VFIODevice *vdev)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(vdev->vga.region); i++) {
        while (!QLIST_EMPTY(&vdev->vga.region[i].quirks)) {
            VFIOQuirk *quirk = QLIST_FIRST(&vdev->vga.region[i].quirks);
            memory_region_del_subregion(&vdev->vga.region[i].mem, &quirk->mem);
            memory_region_destroy(&quirk->mem);
            QLIST_REMOVE(quirk, next);
            g_free(quirk);
        }
    }
}

/*
 * PCI config space
 */
static uint32_t vfio_pci_read_config(PCIDevice *pdev, uint32_t addr, int len)
{
    VFIODevice *vdev = DO_UPCAST(VFIODevice, pdev, pdev);
    uint32_t emu_bits = 0, emu_val = 0, phys_val = 0, val;

    memcpy(&emu_bits, vdev->emulated_config_bits + addr, len);
    emu_bits = le32_to_cpu(emu_bits);

    if (emu_bits) {
        emu_val = pci_default_read_config(pdev, addr, len);
    }

    if (~emu_bits & (0xffffffffU >> (32 - len * 8))) {
        ssize_t ret;
		
        ret = pread(vdev->fd, &phys_val, len, vdev->config_offset + addr);
        if (ret != len) {
            error_report("%s(%04x:%02x:%02x.%x, 0x%x, 0x%x) failed: %m",
                         __func__, 0, 0, 2, 0, addr, len);
            return -errno;
        }
        phys_val = le32_to_cpu(phys_val);
    }

    val = (emu_val & emu_bits) | (phys_val & ~emu_bits);

	DPRINTF("%s(%04x:%02x:%02x.%x, @0x%x, len=0x%x) %x\n", __func__,
            0, 0, 2, 0, addr, len, val);
  

    return val;
}

static void vfio_pci_write_config(PCIDevice *pdev, uint32_t addr,
                                  uint32_t val, int len)
{
    VFIODevice *vdev = DO_UPCAST(VFIODevice, pdev, pdev);
    uint32_t val_le = cpu_to_le32(val);
	DPRINTF("%s(%04x:%02x:%02x.%x, @0x%x, 0x%x, len=0x%x)\n", __func__,
		0, 0, 2, 0, addr, val, len);

    /* Write everything to VFIO, let it filter out what we can't write */
    if (pwrite(vdev->fd, &val_le, len, vdev->config_offset + addr) != len) {
        error_report("%s(%04x:%02x:%02x.%x, 0x%x, 0x%x, 0x%x) failed: %m",
                     __func__, 0, 0, 2, 0, addr, val, len);
    }

    /* MSI/MSI-X Enabling/Disabling */
    if (pdev->cap_present & QEMU_PCI_CAP_MSI &&
        ranges_overlap(addr, len, pdev->msi_cap, vdev->msi_cap_size)) {
        int is_enabled, was_enabled = msi_enabled(pdev);

        pci_default_write_config(pdev, addr, val, len);

        is_enabled = msi_enabled(pdev);

        if (!was_enabled) {
            if (is_enabled) {
                vfio_enable_msi(vdev);
            }
        } else {
            if (!is_enabled) {
                vfio_disable_msi(vdev);
            } else {
                vfio_update_msi(vdev);
            }
        }
    } else if (pdev->cap_present & QEMU_PCI_CAP_MSIX &&
        ranges_overlap(addr, len, pdev->msix_cap, MSIX_CAP_LENGTH)) {
        int is_enabled, was_enabled = msix_enabled(pdev);

        pci_default_write_config(pdev, addr, val, len);

        is_enabled = msix_enabled(pdev);

        if (!was_enabled && is_enabled) {
            vfio_enable_msix(vdev);
        } else if (was_enabled && !is_enabled) {
            vfio_disable_msix(vdev);
        }
    } else {
        /* Write everything to QEMU to keep emulated bits correct */
        pci_default_write_config(pdev, addr, val, len);
    }
}

/*
 * DMA - Mapping and unmapping for the "type1" IOMMU interface used on x86
 */
static int vfio_dma_unmap(VFIOContainer *container,
                          hwaddr iova, ram_addr_t size)
{
    struct vfio_iommu_type1_dma_unmap unmap = {
        .argsz = sizeof(unmap),
        .flags = 0,
        .iova = iova,
        .size = size,
    };

    if (ioctl(container->fd, VFIO_IOMMU_UNMAP_DMA, &unmap)) {
        DPRINTF("VFIO_UNMAP_DMA: %d\n", -errno);
        return -errno;
    }
	DPRINTF("%s: iova:%08X, size:%08X\n",__func__, iova, size);
    return 0;
}

static int vfio_dma_map(VFIOContainer *container, hwaddr iova,
                        ram_addr_t size, void *vaddr, bool readonly)
{
    struct vfio_iommu_type1_dma_map map = {
        .argsz = sizeof(map),
        .flags = VFIO_DMA_MAP_FLAG_READ,
        .vaddr = (__u64)(uintptr_t)vaddr,
        .iova = iova,
        .size = size,
    };
	
	DPRINTF("%s:iova:%x, size:%x, vaddr:%p, ro:%d \n", __func__, iova, size, vaddr, readonly);

    if (!readonly) {
        map.flags |= VFIO_DMA_MAP_FLAG_WRITE;
    }

    /*
     * Try the mapping, if it fails with EBUSY, unmap the region and try
     * again.  This shouldn't be necessary, but we sometimes see it in
     * the the VGA ROM space.
     */
    if (ioctl(container->fd, VFIO_IOMMU_MAP_DMA, &map) == 0 ||
        (errno == EBUSY && vfio_dma_unmap(container, iova, size) == 0 &&
         ioctl(container->fd, VFIO_IOMMU_MAP_DMA, &map) == 0)) {
        return 0;
    }

    DPRINTF("VFIO_MAP_DMA: %d\n", -errno);
    return -errno;
}

static bool vfio_listener_skipped_section(MemoryRegionSection *section)
{
	Int128 legacy={
		.hi=0,
		.lo=0xFFFFF
	};
	
    //return !memory_region_is_ram(section->mr) ||
           /*
            * Sizing an enabled 64-bit BAR can cause spurious mappings to
            * addresses in the upper part of the 64-bit address space.  These
            * are never accessed by the CPU and beyond the address width of
            * some IOMMU hardware.  TODO: VFIO should tell us the IOMMU width.q
            */
    //       section->offset_within_address_space & (1ULL << 63)||
     //      (section->offset_within_address_space <= 0xFFFF && 
     //      int128_le(section->size, legacy)) ||
     //      section->offset_within_address_space == 0x0;
	
    return !memory_region_is_ram(section->mr) ||
    		section->offset_within_address_space & (1ULL << 63);
	
}

static void vfio_listener_region_add(MemoryListener *listener,
                                     MemoryRegionSection *section)
{
    VFIOContainer *container = container_of(listener, VFIOContainer,
                                            iommu_data.type1.listener);
    hwaddr iova, end;
    void *vaddr;
    int ret;

    assert(!memory_region_is_iommu(section->mr));

    if (vfio_listener_skipped_section(section)) {
        DPRINTF("SKIPPING region_add %"HWADDR_PRIx" - %"PRIx64"\n",
                section->offset_within_address_space,
                section->offset_within_address_space +
                int128_get64(int128_sub(section->size, int128_one())));
        return;
    }

    if (unlikely((section->offset_within_address_space & ~TARGET_PAGE_MASK) !=
                 (section->offset_within_region & ~TARGET_PAGE_MASK))) {
        error_report("%s received unaligned region", __func__);
        return;
    }

    iova = section->offset_within_address_space & ~( (1 << 12) - 1);
	DPRINTF("%s ofas: %"HWADDR_PRIx", size:%08llx, iova:%"HWADDR_PRIx"\n",
		section->mr->name, section->offset_within_address_space, int128_get64(section->size), iova);
    end = (section->offset_within_address_space + int128_get64(section->size)) &
          TARGET_PAGE_MASK;

    if (iova >= end) {
        return;
    }

    vaddr = memory_region_get_ram_ptr(section->mr) +
            section->offset_within_region +
            (iova - section->offset_within_address_space);

    DPRINTF("region_add %"HWADDR_PRIx" - %"HWADDR_PRIx" [%p]\n",
            iova, end - 1, vaddr);

    memory_region_ref(section->mr);
    ret = vfio_dma_map(container, iova, end - iova, vaddr, section->readonly);
    if (ret) {
        error_report("vfio_dma_map(%p, 0x%"HWADDR_PRIx", "
                     "0x%"HWADDR_PRIx", %p) = %d (%m)",
                     container, iova, end - iova, vaddr, ret);

        /*
         * On the initfn path, store the first error in the container so we
         * can gracefully fail.  Runtime, there's not much we can do other
         * than throw a hardware error.
         */
        if (!container->iommu_data.type1.initialized) {
            if (!container->iommu_data.type1.error) {
                container->iommu_data.type1.error = ret;
            }
        } else {
            hw_error("vfio: DMA mapping failed, unable to continue\n");
        }
    }
}

static void vfio_listener_region_del(MemoryListener *listener,
                                     MemoryRegionSection *section)
{
    VFIOContainer *container = container_of(listener, VFIOContainer,
                                            iommu_data.type1.listener);
    hwaddr iova, end;
    int ret;

    if (vfio_listener_skipped_section(section)) {
        DPRINTF("SKIPPING region_del %"HWADDR_PRIx" - %"PRIx64"\n",
                section->offset_within_address_space,
                section->offset_within_address_space +
                int128_get64(int128_sub(section->size, int128_one())));
        return;
    }

    if (unlikely((section->offset_within_address_space & ~TARGET_PAGE_MASK) !=
                 (section->offset_within_region & ~TARGET_PAGE_MASK))) {
        error_report("%s received unaligned region", __func__);
        return;
    }

    //iova = TARGET_PAGE_ALIGN(section->offset_within_address_space);
    iova = section->offset_within_address_space & ~( (1 << 12) - 1);
    end = (section->offset_within_address_space + int128_get64(section->size)) &
          TARGET_PAGE_MASK;

    if (iova >= end) {
        return;
    }

    DPRINTF("region_del %"HWADDR_PRIx" - %"HWADDR_PRIx"\n",
            iova, end - 1);

    ret = vfio_dma_unmap(container, iova, end - iova);
    memory_region_unref(section->mr);
    if (ret) {
        error_report("vfio_dma_unmap(%p, 0x%"HWADDR_PRIx", "
                     "0x%"HWADDR_PRIx") = %d (%m)",
                     container, iova, end - iova, ret);
    }
}

static MemoryListener vfio_memory_listener = {
    .region_add = vfio_listener_region_add,
    .region_del = vfio_listener_region_del,
};

static void vfio_listener_release(VFIOContainer *container)
{
    memory_listener_unregister(&container->iommu_data.type1.listener);
}

/*
 * Interrupt setup
 */
static void vfio_disable_interrupts(VFIODevice *vdev)
{
    switch (vdev->interrupt) {
    case VFIO_INT_INTx:
        vfio_disable_intx(vdev);
        break;
    case VFIO_INT_MSI:
        vfio_disable_msi(vdev);
        break;
    case VFIO_INT_MSIX:
        vfio_disable_msix(vdev);
        break;
    }
}

static int vfio_setup_msi(VFIODevice *vdev, int pos)
{
    uint16_t ctrl;
    bool msi_64bit, msi_maskbit;
    int ret, entries;

    if (pread(vdev->fd, &ctrl, sizeof(ctrl),
              vdev->config_offset + pos + PCI_CAP_FLAGS) != sizeof(ctrl)) {
        return -errno;
    }
    ctrl = le16_to_cpu(ctrl);

    msi_64bit = !!(ctrl & PCI_MSI_FLAGS_64BIT);
    msi_maskbit = !!(ctrl & PCI_MSI_FLAGS_MASKBIT);
    entries = 1 << ((ctrl & PCI_MSI_FLAGS_QMASK) >> 1);

    DPRINTF("%04x:%02x:%02x.%x PCI MSI CAP @0x%x\n", vdev->host.domain,
            vdev->host.bus, vdev->host.slot, vdev->host.function, pos);

    ret = msi_init(&vdev->pdev, pos, entries, msi_64bit, msi_maskbit);
    if (ret < 0) {
        if (ret == -ENOTSUP) {
            return 0;
        }
        error_report("vfio: msi_init failed");
        return ret;
    }
    vdev->msi_cap_size = 0xa + (msi_maskbit ? 0xa : 0) + (msi_64bit ? 0x4 : 0);

    return 0;
}

/*
 * We don't have any control over how pci_add_capability() inserts
 * capabilities into the chain.  In order to setup MSI-X we need a
 * MemoryRegion for the BAR.  In order to setup the BAR and not
 * attempt to mmap the MSI-X table area, which VFIO won't allow, we
 * need to first look for where the MSI-X table lives.  So we
 * unfortunately split MSI-X setup across two functions.
 */
static int vfio_early_setup_msix(VFIODevice *vdev)
{
    uint8_t pos;
    uint16_t ctrl;
    uint32_t table, pba;

    pos = pci_find_capability(&vdev->pdev, PCI_CAP_ID_MSIX);
    if (!pos) {
        return 0;
    }

    if (pread(vdev->fd, &ctrl, sizeof(ctrl),
              vdev->config_offset + pos + PCI_CAP_FLAGS) != sizeof(ctrl)) {
        return -errno;
    }

    if (pread(vdev->fd, &table, sizeof(table),
              vdev->config_offset + pos + PCI_MSIX_TABLE) != sizeof(table)) {
        return -errno;
    }

    if (pread(vdev->fd, &pba, sizeof(pba),
              vdev->config_offset + pos + PCI_MSIX_PBA) != sizeof(pba)) {
        return -errno;
    }

    ctrl = le16_to_cpu(ctrl);
    table = le32_to_cpu(table);
    pba = le32_to_cpu(pba);

    vdev->msix = g_malloc0(sizeof(*(vdev->msix)));
    vdev->msix->table_bar = table & PCI_MSIX_FLAGS_BIRMASK;
    vdev->msix->table_offset = table & ~PCI_MSIX_FLAGS_BIRMASK;
    vdev->msix->pba_bar = pba & PCI_MSIX_FLAGS_BIRMASK;
    vdev->msix->pba_offset = pba & ~PCI_MSIX_FLAGS_BIRMASK;
    vdev->msix->entries = (ctrl & PCI_MSIX_FLAGS_QSIZE) + 1;

    DPRINTF("%04x:%02x:%02x.%x "
            "PCI MSI-X CAP @0x%x, BAR %d, offset 0x%x, entries %d\n",
            vdev->host.domain, vdev->host.bus, vdev->host.slot,
            vdev->host.function, pos, vdev->msix->table_bar,
            vdev->msix->table_offset, vdev->msix->entries);

    return 0;
}

static int vfio_setup_msix(VFIODevice *vdev, int pos)
{
    int ret;
    DPRINTF("%04x:%02x:%02x.%x "
		"%s\n",
        vdev->host.domain, vdev->host.bus, vdev->host.slot,
        vdev->host.function, __func__);

    ret = msix_init(&vdev->pdev, vdev->msix->entries,
                    &vdev->bars[vdev->msix->table_bar].mem,
                    vdev->msix->table_bar, vdev->msix->table_offset,
                    &vdev->bars[vdev->msix->pba_bar].mem,
                    vdev->msix->pba_bar, vdev->msix->pba_offset, pos);
    if (ret < 0) {
        if (ret == -ENOTSUP) {
            return 0;
        }
        error_report("vfio: msix_init failed");
        return ret;
    }

    return 0;
}

static void vfio_teardown_msi(VFIODevice *vdev)
{
    msi_uninit(&vdev->pdev);

    if (vdev->msix) {
        msix_uninit(&vdev->pdev, &vdev->bars[vdev->msix->table_bar].mem,
                    &vdev->bars[vdev->msix->pba_bar].mem);
    }
}

/*
 * Resource setup
 */
static void vfio_mmap_set_enabled(VFIODevice *vdev, bool enabled)
{
    int i;

    for (i = 0; i < PCI_ROM_SLOT; i++) {
        VFIOBAR *bar = &vdev->bars[i];

        if (!bar->size) {
            continue;
        }

        memory_region_set_enabled(&bar->mmap_mem, enabled);
        if (vdev->msix && vdev->msix->table_bar == i) {
            memory_region_set_enabled(&vdev->msix->mmap_mem, enabled);
        }
    }
}

static void vfio_unmap_bar(VFIODevice *vdev, int nr)
{
    VFIOBAR *bar = &vdev->bars[nr];

    if (!bar->size) {
        return;
    }

    memory_region_del_subregion(&bar->mem, &bar->mmap_mem);
    munmap(bar->mmap, memory_region_size(&bar->mmap_mem));
    memory_region_destroy(&bar->mmap_mem);

    if (vdev->msix && vdev->msix->table_bar == nr) {
        memory_region_del_subregion(&bar->mem, &vdev->msix->mmap_mem);
        munmap(vdev->msix->mmap, memory_region_size(&vdev->msix->mmap_mem));
        memory_region_destroy(&vdev->msix->mmap_mem);
    }

    memory_region_destroy(&bar->mem);
}

static int vfio_mmap_bar(VFIODevice *vdev, VFIOBAR *bar,
                         MemoryRegion *mem, MemoryRegion *submem,
                         void **map, size_t size, off_t offset,
                         const char *name)
{
    int ret = 0;
	

	
    if (VFIO_ALLOW_MMAP && size && bar->flags & VFIO_REGION_INFO_FLAG_MMAP) {
#ifdef DEBUG_IGD_MMIO
		if(bar->fd_offset  == 0) {
			/*debug BAR0 mmio*/
			*map = NULL;
			ret = -errno;
			goto empty_region;
		}
#endif

        int prot = 0;

        if (bar->flags & VFIO_REGION_INFO_FLAG_READ) {
            prot |= PROT_READ;
        }

        if (bar->flags & VFIO_REGION_INFO_FLAG_WRITE) {
            prot |= PROT_WRITE;
        }

        *map = mmap(NULL, size, prot, MAP_SHARED,
                    bar->fd, bar->fd_offset + offset);
        if (*map == MAP_FAILED) {
            *map = NULL;
            ret = -errno;
            goto empty_region;
        }

        memory_region_init_ram_ptr(submem, OBJECT(vdev), name, size, *map);
    } else {
empty_region:
        /* Create a zero sized sub-region to make cleanup easy. */
        memory_region_init(submem, OBJECT(vdev), name, 0);
    }

    memory_region_add_subregion(mem, offset, submem);

    return ret;
}

static void vfio_map_bar(VFIODevice *vdev, int nr)
{
    VFIOBAR *bar = &vdev->bars[nr];
    unsigned size = bar->size;
    char name[64];
    uint32_t pci_bar;
    uint8_t type;
    int ret;

    /* Skip both unimplemented BARs and the upper half of 64bit BARS. */
    if (!size) {
        return;
    }

	
	
    snprintf(name, sizeof(name), "VFIO %04x:%02x:%02x.%x BAR %d",
             0, 0, 2, 0, nr);

    /* Determine what type of BAR this is for registration */
    ret = pread(vdev->fd, &pci_bar, sizeof(pci_bar),
                vdev->config_offset + PCI_BASE_ADDRESS_0 + (4 * nr));
    if (ret != sizeof(pci_bar)) {
        error_report("vfio: Failed to read BAR %d (%m)", nr);
        return;
    }

    pci_bar = le32_to_cpu(pci_bar);
    bar->ioport = (pci_bar & PCI_BASE_ADDRESS_SPACE_IO);
    bar->mem64 = bar->ioport ? 0 : (pci_bar & PCI_BASE_ADDRESS_MEM_TYPE_64);
    type = pci_bar & (bar->ioport ? ~PCI_BASE_ADDRESS_IO_MASK :
                                    ~PCI_BASE_ADDRESS_MEM_MASK);

    /* A "slow" read/write mapping underlies all BARs */
    memory_region_init_io(&bar->mem, OBJECT(vdev), &vfio_bar_ops,
                          bar, name, size);
	
	DPRINTF("%s: bar: %x, iop: %d, 64: %d, size:%x\n",__func__, pci_bar, bar->ioport, bar->mem64, size);

	pci_register_bar(&vdev->pdev, nr, type, &bar->mem);

    /*
     * We can't mmap areas overlapping the MSIX vector table, so we
     * potentially insert a direct-mapped subregion before and after it.
     */
    if (vdev->msix && vdev->msix->table_bar == nr) {
        size = vdev->msix->table_offset & qemu_host_page_mask;
    }

    
    if (vfio_mmap_bar(vdev, bar, &bar->mem,
                      &bar->mmap_mem, &bar->mmap, size, 0, name)) {
        error_report("%s unsupported. Performance may be slow", name);
    }else
    	strncat(name, " mmap", sizeof(name) - strlen(name) - 1);

    if (vdev->msix && vdev->msix->table_bar == nr) {
        unsigned start;

        start = HOST_PAGE_ALIGN(vdev->msix->table_offset +
                                (vdev->msix->entries * PCI_MSIX_ENTRY_SIZE));

        size = start < bar->size ? bar->size - start : 0;
        strncat(name, " msix-hi", sizeof(name) - strlen(name) - 1);
        /* VFIOMSIXInfo contains another MemoryRegion for this mapping */
        if (vfio_mmap_bar(vdev, bar, &bar->mem, &vdev->msix->mmap_mem,
                          &vdev->msix->mmap, size, start, name)) {
            error_report("%s unsupported. Performance may be slow", name);
        }
    }

}


static const MemoryRegionOps vfio_vga_ops = {
    .read = vfio_vga_read,
    .write = vfio_vga_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void vfio_map_bars(VFIODevice *vdev)
{
    int i;

    for (i = 0; i < PCI_ROM_SLOT; i++) {
        vfio_map_bar(vdev, i);
    }

    if (vdev->has_vga) {
		
		int vram_fd = open("/dev/mem", O_RDWR);
		void *map = mmap(NULL, 0x20000, PROT_READ|PROT_WRITE, MAP_SHARED,
                    vram_fd, 0xa0000);
        if (map == MAP_FAILED) {
            error_report("VRAM mmap failed\n");
			close(vram_fd);
			memory_region_init_io(&vdev->vga.region[QEMU_PCI_VGA_MEM].mem,
									 OBJECT(vdev), &vfio_vga_ops,
									 &vdev->vga.region[QEMU_PCI_VGA_MEM],
									 "vfio-vga-mmio@0xa0000",
									 QEMU_PCI_VGA_MEM_SIZE);
			DPRINTF("mg init io: vfio-vga-mmio@0xa0000\n");

        } else {

        	memory_region_init_ram_ptr(&vdev->vga.region[QEMU_PCI_VGA_MEM].mem,
							OBJECT(vdev), "vfio-vga-vram", 0x20000, map);
        }
       //memory_region_init_io(&vdev->vga.region[QEMU_PCI_VGA_MEM].mem,
       //                       OBJECT(vdev), &vfio_vga_ops,
       //                       &vdev->vga.region[QEMU_PCI_VGA_MEM],
       //                       "vfio-vga-mmio@0xa0000",
       //                       QEMU_PCI_VGA_MEM_SIZE);
		//DPRINTF("mg init io: vfio-vga-mmio@0xa0000\n");
		
		
        memory_region_init_io(&vdev->vga.region[QEMU_PCI_VGA_IO_LO].mem,
                              OBJECT(vdev), &vfio_vga_ops,
                              &vdev->vga.region[QEMU_PCI_VGA_IO_LO],
                              "vfio-vga-io@0x3b0",
                              QEMU_PCI_VGA_IO_LO_SIZE);
		DPRINTF("mg init io: vfio-vga-io@0x3b0\n");
		
        memory_region_init_io(&vdev->vga.region[QEMU_PCI_VGA_IO_HI].mem,
                              OBJECT(vdev), &vfio_vga_ops,
                              &vdev->vga.region[QEMU_PCI_VGA_IO_HI],
                              "vfio-vga-io@0x3c0",
                              QEMU_PCI_VGA_IO_HI_SIZE);
		DPRINTF("mg init io: vfio-vga-io@0x3c0\n");

        pci_register_vga(&vdev->pdev, &vdev->vga.region[QEMU_PCI_VGA_MEM].mem,
                         &vdev->vga.region[QEMU_PCI_VGA_IO_LO].mem,
                        &vdev->vga.region[QEMU_PCI_VGA_IO_HI].mem);
		DPRINTF("pci_register_vga\n");

    }


}

static void vfio_unmap_bars(VFIODevice *vdev)
{
    int i;

    for (i = 0; i < PCI_ROM_SLOT; i++) {
        vfio_unmap_bar(vdev, i);
    }

    if (vdev->has_vga) {
        vfio_vga_quirk_teardown(vdev);
        pci_unregister_vga(&vdev->pdev);
        memory_region_destroy(&vdev->vga.region[QEMU_PCI_VGA_MEM].mem);
        memory_region_destroy(&vdev->vga.region[QEMU_PCI_VGA_IO_LO].mem);
        memory_region_destroy(&vdev->vga.region[QEMU_PCI_VGA_IO_HI].mem);
    }
}
struct dev_interface {
	volatile char bar0_addr;
	volatile char bar2_addr;
	void *mmio;
	void *gtt_aperture;
};

struct dev_interface interface;
pthread_t draw_thread;
volatile uint64_t fence_regs[32];
void *vnc_draw(void *param)
{
	printf("bar0:%x, bar2:%x, mmio:%x, gttap:%x\n", 
		((struct dev_interface *)param)->bar0_addr,
		((struct dev_interface *)param)->bar2_addr,
		((struct dev_interface *)param)->mmio,
		((struct dev_interface *)param)->gtt_aperture);
	while(true) {
		sleep(10);
		//do not use memcpy to avoid optimization
		int i;
		for(i = 0; i < 32; i++) {
			fence_regs[i] = *(volatile uint64_t *)(((struct dev_interface *)param)->mmio + 0x100000);
		}
		
		for(i = 0; i < 16; i++) {
			printf("value of %dth fence 0x%0llx\n",
				i, fence_regs[i]);
			
		}
		printf("value of DE_IER 0x%0llx\n",
				i, 
				*(uint32_t *)((struct dev_interface *)param)->mmio + 0x44000);
	}
}

void vnc_drawing_init(PCIDevice *pdev)
{
	pthread_attr_t attr;
 	pthread_attr_init(&attr);
	int devmem_fd = open("/dev/mem", O_RDWR); 
	interface.bar0_addr = __host_pci_read_config(0, 2, 0, 0x10, 4) & ~0xF;
	interface.bar2_addr = __host_pci_read_config(0, 2, 0, 0x18, 4) & ~0xF;
	interface.mmio = mmap(NULL, 0x14000, PROT_READ|PROT_WRITE, MAP_SHARED,
                    devmem_fd, interface.bar0_addr);
	interface.gtt_aperture = mmap(NULL, GFX_STOLEN_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED,
                    devmem_fd, interface.bar2_addr);
	
    if (interface.mmio == MAP_FAILED || interface.gtt_aperture == MAP_FAILED) {
		error_report("buffer mmap failed\n");
		close(devmem_fd);
		return;
    }
	
	pthread_create(&draw_thread, &attr, vnc_draw, &interface);
}

/*
 * General setup
 */
static uint8_t vfio_std_cap_max_size(PCIDevice *pdev, uint8_t pos)
{
    uint8_t tmp, next = 0xff;

    for (tmp = pdev->config[PCI_CAPABILITY_LIST]; tmp;
         tmp = pdev->config[tmp + 1]) {
        if (tmp > pos && tmp < next) {
            next = tmp;
        }
    }

    return next - pos;
}

static void vfio_set_word_bits(uint8_t *buf, uint16_t val, uint16_t mask)
{
    pci_set_word(buf, (pci_get_word(buf) & ~mask) | val);
}

static void vfio_add_emulated_word(VFIODevice *vdev, int pos,
                                   uint16_t val, uint16_t mask)
{
    vfio_set_word_bits(vdev->pdev.config + pos, val, mask);
    vfio_set_word_bits(vdev->pdev.wmask + pos, ~mask, mask);
    vfio_set_word_bits(vdev->emulated_config_bits + pos, mask, mask);
}

static void vfio_set_long_bits(uint8_t *buf, uint32_t val, uint32_t mask)
{
    pci_set_long(buf, (pci_get_long(buf) & ~mask) | val);
}

static void vfio_add_emulated_long(VFIODevice *vdev, int pos,
                                   uint32_t val, uint32_t mask)
{
    vfio_set_long_bits(vdev->pdev.config + pos, val, mask);
    vfio_set_long_bits(vdev->pdev.wmask + pos, ~mask, mask);
    vfio_set_long_bits(vdev->emulated_config_bits + pos, mask, mask);
}

static int vfio_setup_pcie_cap(VFIODevice *vdev, int pos, uint8_t size)
{
    uint16_t flags;
    uint8_t type;

    flags = pci_get_word(vdev->pdev.config + pos + PCI_CAP_FLAGS);
    type = (flags & PCI_EXP_FLAGS_TYPE) >> 4;

    if (type != PCI_EXP_TYPE_ENDPOINT &&
        type != PCI_EXP_TYPE_LEG_END &&
        type != PCI_EXP_TYPE_RC_END) {

        error_report("vfio: Assignment of PCIe type 0x%x "
                     "devices is not currently supported", type);
        return -EINVAL;
    }

    if (!pci_bus_is_express(vdev->pdev.bus)) {
        /*
         * Use express capability as-is on PCI bus.  It doesn't make much
         * sense to even expose, but some drivers (ex. tg3) depend on it
         * and guests don't seem to be particular about it.  We'll need
         * to revist this or force express devices to express buses if we
         * ever expose an IOMMU to the guest.
         */
    } else if (pci_bus_is_root(vdev->pdev.bus)) {
        /*
         * On a Root Complex bus Endpoints become Root Complex Integrated
         * Endpoints, which changes the type and clears the LNK & LNK2 fields.
         */
        if (type == PCI_EXP_TYPE_ENDPOINT) {
            vfio_add_emulated_word(vdev, pos + PCI_CAP_FLAGS,
                                   PCI_EXP_TYPE_RC_END << 4,
                                   PCI_EXP_FLAGS_TYPE);

            /* Link Capabilities, Status, and Control goes away */
            if (size > PCI_EXP_LNKCTL) {
                vfio_add_emulated_long(vdev, pos + PCI_EXP_LNKCAP, 0, ~0);
                vfio_add_emulated_word(vdev, pos + PCI_EXP_LNKCTL, 0, ~0);
                vfio_add_emulated_word(vdev, pos + PCI_EXP_LNKSTA, 0, ~0);

#ifndef PCI_EXP_LNKCAP2
#define PCI_EXP_LNKCAP2 44
#endif
#ifndef PCI_EXP_LNKSTA2
#define PCI_EXP_LNKSTA2 50
#endif
                /* Link 2 Capabilities, Status, and Control goes away */
                if (size > PCI_EXP_LNKCAP2) {
                    vfio_add_emulated_long(vdev, pos + PCI_EXP_LNKCAP2, 0, ~0);
                    vfio_add_emulated_word(vdev, pos + PCI_EXP_LNKCTL2, 0, ~0);
                    vfio_add_emulated_word(vdev, pos + PCI_EXP_LNKSTA2, 0, ~0);
                }
            }

        } else if (type == PCI_EXP_TYPE_LEG_END) {
            /*
             * Legacy endpoints don't belong on the root complex.  Windows
             * seems to be happier with devices if we skip the capability.
             */
            return 0;
        }

    } else {
        /*
         * Convert Root Complex Integrated Endpoints to regular endpoints.
         * These devices don't support LNK/LNK2 capabilities, so make them up.
         */
        if (type == PCI_EXP_TYPE_RC_END) {
            vfio_add_emulated_word(vdev, pos + PCI_CAP_FLAGS,
                                   PCI_EXP_TYPE_ENDPOINT << 4,
                                   PCI_EXP_FLAGS_TYPE);
            vfio_add_emulated_long(vdev, pos + PCI_EXP_LNKCAP,
                                   PCI_EXP_LNK_MLW_1 | PCI_EXP_LNK_LS_25, ~0);
            vfio_add_emulated_word(vdev, pos + PCI_EXP_LNKCTL, 0, ~0);
        }

        /* Mark the Link Status bits as emulated to allow virtual negotiation */
        vfio_add_emulated_word(vdev, pos + PCI_EXP_LNKSTA,
                               pci_get_word(vdev->pdev.config + pos +
                                            PCI_EXP_LNKSTA),
                               PCI_EXP_LNKCAP_MLW | PCI_EXP_LNKCAP_SLS);
    }

    pos = pci_add_capability(&vdev->pdev, PCI_CAP_ID_EXP, pos, size);
    if (pos >= 0) {
        vdev->pdev.exp.exp_cap = pos;
    }

    return pos;
}

static void vfio_check_pcie_flr(VFIODevice *vdev, uint8_t pos)
{
    uint32_t cap = pci_get_long(vdev->pdev.config + pos + PCI_EXP_DEVCAP);

    if (cap & PCI_EXP_DEVCAP_FLR) {
        DPRINTF("%04x:%02x:%02x.%x Supports FLR via PCIe cap\n",
                vdev->host.domain, vdev->host.bus, vdev->host.slot,
                vdev->host.function);
        vdev->has_flr = true;
    }
}

static void vfio_check_pm_reset(VFIODevice *vdev, uint8_t pos)
{
    uint16_t csr = pci_get_word(vdev->pdev.config + pos + PCI_PM_CTRL);

    if (!(csr & PCI_PM_CTRL_NO_SOFT_RESET)) {
        DPRINTF("%04x:%02x:%02x.%x Supports PM reset\n", 0,0,2,0);
        vdev->has_pm_reset = true;
    }
}

static void vfio_check_af_flr(VFIODevice *vdev, uint8_t pos)
{
    uint8_t cap = pci_get_byte(vdev->pdev.config + pos + PCI_AF_CAP);

    if ((cap & PCI_AF_CAP_TP) && (cap & PCI_AF_CAP_FLR)) {
        DPRINTF("%04x:%02x:%02x.%x Supports FLR via AF cap\n", 0,0,2,0);
        vdev->has_flr = true;
    }
}

static int vfio_add_std_cap(VFIODevice *vdev, uint8_t pos)
{
    PCIDevice *pdev = &vdev->pdev;
    uint8_t cap_id, next, size;
    int ret;

    cap_id = pdev->config[pos];
    next = pdev->config[pos + 1];

    /*
     * If it becomes important to configure capabilities to their actual
     * size, use this as the default when it's something we don't recognize.
     * Since QEMU doesn't actually handle many of the config accesses,
     * exact size doesn't seem worthwhile.
     */
    size = vfio_std_cap_max_size(pdev, pos);

    /*
     * pci_add_capability always inserts the new capability at the head
     * of the chain.  Therefore to end up with a chain that matches the
     * physical device, we insert from the end by making this recursive.
     * This is also why we pre-caclulate size above as cached config space
     * will be changed as we unwind the stack.
     */
    if (next) {
        ret = vfio_add_std_cap(vdev, next);
        if (ret) {
            return ret;
        }
    } else {
        /* Begin the rebuild, use QEMU emulated list bits */
        pdev->config[PCI_CAPABILITY_LIST] = 0;
        vdev->emulated_config_bits[PCI_CAPABILITY_LIST] = 0xff;
        vdev->emulated_config_bits[PCI_STATUS] |= PCI_STATUS_CAP_LIST;
    }

    /* Use emulated next pointer to allow dropping caps */
    pci_set_byte(vdev->emulated_config_bits + pos + 1, 0xff);

    switch (cap_id) {
    case PCI_CAP_ID_MSI:
        ret = vfio_setup_msi(vdev, pos);
        break;
    case PCI_CAP_ID_EXP:
        vfio_check_pcie_flr(vdev, pos);
        ret = vfio_setup_pcie_cap(vdev, pos, size);
        break;
    case PCI_CAP_ID_MSIX:
        ret = vfio_setup_msix(vdev, pos);
        break;
    case PCI_CAP_ID_PM:
        vfio_check_pm_reset(vdev, pos);
        vdev->pm_cap = pos;
        ret = pci_add_capability(pdev, cap_id, pos, size);
        break;
    case PCI_CAP_ID_AF:
        vfio_check_af_flr(vdev, pos);
        ret = pci_add_capability(pdev, cap_id, pos, size);
        break;
    default:
        ret = pci_add_capability(pdev, cap_id, pos, size);
        break;
    }

    if (ret < 0) {
        error_report("vfio: %04x:%02x:%02x.%x Error adding PCI capability "
                     "0x%x[0x%x]@0x%x: %d", vdev->host.domain,
                     vdev->host.bus, vdev->host.slot, vdev->host.function,
                     cap_id, size, pos, ret);
        return ret;
    }

    return 0;
}

static int vfio_add_capabilities(VFIODevice *vdev)
{
    PCIDevice *pdev = &vdev->pdev;

    if (!(pdev->config[PCI_STATUS] & PCI_STATUS_CAP_LIST) ||
        !pdev->config[PCI_CAPABILITY_LIST]) {
        return 0; /* Nothing to add */
    }

    return vfio_add_std_cap(vdev, pdev->config[PCI_CAPABILITY_LIST]);
}

static void vfio_pci_pre_reset(VFIODevice *vdev)
{
    PCIDevice *pdev = &vdev->pdev;
    uint16_t cmd;

    vfio_disable_interrupts(vdev);

    /* Make sure the device is in D0 */
    if (vdev->pm_cap) {
        uint16_t pmcsr;
        uint8_t state;

        pmcsr = vfio_pci_read_config(pdev, vdev->pm_cap + PCI_PM_CTRL, 2);
        state = pmcsr & PCI_PM_CTRL_STATE_MASK;
        if (state) {
			DPRINTF("PM STATE: %u\n", (unsigned int)state);
            pmcsr &= ~PCI_PM_CTRL_STATE_MASK;
            vfio_pci_write_config(pdev, vdev->pm_cap + PCI_PM_CTRL, pmcsr, 2);
            /* vfio handles the necessary delay here */
            pmcsr = vfio_pci_read_config(pdev, vdev->pm_cap + PCI_PM_CTRL, 2);
            state = pmcsr & PCI_PM_CTRL_STATE_MASK;
            if (state) {
                error_report("vfio: Unable to power on device, stuck in D%d\n",
                             state);
            }
        }
    }

    /*
     * Stop any ongoing DMA by disconecting I/O, MMIO, and bus master.
     * Also put INTx Disable in known state.
     */
    cmd = vfio_pci_read_config(pdev, PCI_COMMAND, 2);
    cmd &= ~(PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER |
             PCI_COMMAND_INTX_DISABLE);
    vfio_pci_write_config(pdev, PCI_COMMAND, cmd, 2);
}

static void vfio_pci_post_reset(VFIODevice *vdev)
{
    vfio_enable_intx(vdev);
}

static bool vfio_pci_host_match(PCIHostDeviceAddress *host1,
                                PCIHostDeviceAddress *host2)
{
    return (host1->domain == host2->domain && host1->bus == host2->bus &&
            host1->slot == host2->slot && host1->function == host2->function);
}

static int vfio_pci_hot_reset(VFIODevice *vdev, bool single)
{
    VFIOGroup *group;
    struct vfio_pci_hot_reset_info *info;
    struct vfio_pci_dependent_device *devices;
    struct vfio_pci_hot_reset *reset;
    int32_t *fds;
    int ret, i, count;
    bool multi = false;

    DPRINTF("%s(%04x:%02x:%02x.%x) %s\n", __func__, 0, 0, 2, 0,
            single ? "one" : "multi");

    vfio_pci_pre_reset(vdev);
    vdev->needs_reset = false;

    info = g_malloc0(sizeof(*info));
    info->argsz = sizeof(*info);

    ret = ioctl(vdev->fd, VFIO_DEVICE_GET_PCI_HOT_RESET_INFO, info);
    if (ret && errno != ENOSPC) {
        ret = -errno;
        if (!vdev->has_pm_reset) {
            error_report("vfio: Cannot reset device %04x:%02x:%02x.%x, "
                         "no available reset mechanism.", 0,0,2,0);
        }
        goto out_single;
    }

    count = info->count;
    info = g_realloc(info, sizeof(*info) + (count * sizeof(*devices)));
    info->argsz = sizeof(*info) + (count * sizeof(*devices));
    devices = &info->devices[0];

    ret = ioctl(vdev->fd, VFIO_DEVICE_GET_PCI_HOT_RESET_INFO, info);
    if (ret) {
        ret = -errno;
        error_report("vfio: hot reset info failed: %m");
        goto out_single;
    }

    DPRINTF("%04x:%02x:%02x.%x: hot reset dependent devices:\n",
            vdev->host.domain, vdev->host.bus, vdev->host.slot,
            vdev->host.function);

    /* Verify that we have all the groups required */
    for (i = 0; i < info->count; i++) {
        PCIHostDeviceAddress host;
        VFIODevice *tmp;

        host.domain = devices[i].segment;
        host.bus = devices[i].bus;
        host.slot = PCI_SLOT(devices[i].devfn);
        host.function = PCI_FUNC(devices[i].devfn);

        DPRINTF("\t%04x:%02x:%02x.%x group %d\n", host.domain,
                host.bus, host.slot, host.function, devices[i].group_id);

        if (vfio_pci_host_match(&host, &vdev->host)) {
            continue;
        }

        QLIST_FOREACH(group, &group_list, next) {
            if (group->groupid == devices[i].group_id) {
                break;
            }
        }

        if (!group) {
            if (!vdev->has_pm_reset) {
                error_report("vfio: Cannot reset device %04x:%02x:%02x.%x, "
                             "depends on group %d which is not owned.",
                             vdev->host.domain, vdev->host.bus, vdev->host.slot,
                             vdev->host.function, devices[i].group_id);
            }
            ret = -EPERM;
            goto out;
        }

        /* Prep dependent devices for reset and clear our marker. */
        QLIST_FOREACH(tmp, &group->device_list, next) {
            if (vfio_pci_host_match(&host, &tmp->host)) {
                if (single) {
                    DPRINTF("vfio: found another in-use device "
                            "%04x:%02x:%02x.%x\n", host.domain, host.bus,
                            host.slot, host.function);
                    ret = -EINVAL;
                    goto out_single;
                }
                vfio_pci_pre_reset(tmp);
                tmp->needs_reset = false;
                multi = true;
                break;
            }
        }
    }

    if (!single && !multi) {
        DPRINTF("vfio: No other in-use devices for multi hot reset\n");
        ret = -EINVAL;
        goto out_single;
    }

    /* Determine how many group fds need to be passed */
    count = 0;
    QLIST_FOREACH(group, &group_list, next) {
        for (i = 0; i < info->count; i++) {
            if (group->groupid == devices[i].group_id) {
                count++;
                break;
            }
        }
    }

    reset = g_malloc0(sizeof(*reset) + (count * sizeof(*fds)));
    reset->argsz = sizeof(*reset) + (count * sizeof(*fds));
    fds = &reset->group_fds[0];

    /* Fill in group fds */
    QLIST_FOREACH(group, &group_list, next) {
        for (i = 0; i < info->count; i++) {
            if (group->groupid == devices[i].group_id) {
                fds[reset->count++] = group->fd;
                break;
            }
        }
    }

    /* Bus reset! */
    ret = ioctl(vdev->fd, VFIO_DEVICE_PCI_HOT_RESET, reset);
    g_free(reset);

    DPRINTF("%04x:%02x:%02x.%x hot reset: %s\n", vdev->host.domain,
            vdev->host.bus, vdev->host.slot, vdev->host.function,
            ret ? "%m" : "Success");

out:
    /* Re-enable INTx on affected devices */
    for (i = 0; i < info->count; i++) {
        PCIHostDeviceAddress host;
        VFIODevice *tmp;

        host.domain = devices[i].segment;
        host.bus = devices[i].bus;
        host.slot = PCI_SLOT(devices[i].devfn);
        host.function = PCI_FUNC(devices[i].devfn);

        if (vfio_pci_host_match(&host, &vdev->host)) {
            continue;
        }

        QLIST_FOREACH(group, &group_list, next) {
            if (group->groupid == devices[i].group_id) {
                break;
            }
        }

        if (!group) {
            break;
        }

        QLIST_FOREACH(tmp, &group->device_list, next) {
            if (vfio_pci_host_match(&host, &tmp->host)) {
                vfio_pci_post_reset(tmp);
                break;
            }
        }
    }
out_single:
    vfio_pci_post_reset(vdev);
    g_free(info);

    return ret;
}

/*
 * We want to differentiate hot reset of mulitple in-use devices vs hot reset
 * of a single in-use device.  VFIO_DEVICE_RESET will already handle the case
 * of doing hot resets when there is only a single device per bus.  The in-use
 * here refers to how many VFIODevices are affected.  A hot reset that affects
 * multiple devices, but only a single in-use device, means that we can call
 * it from our bus ->reset() callback since the extent is effectively a single
 * device.  This allows us to make use of it in the hotplug path.  When there
 * are multiple in-use devices, we can only trigger the hot reset during a
 * system reset and thus from our reset handler.  We separate _one vs _multi
 * here so that we don't overlap and do a double reset on the system reset
 * path where both our reset handler and ->reset() callback are used.  Calling
 * _one() will only do a hot reset for the one in-use devices case, calling
 * _multi() will do nothing if a _one() would have been sufficient.
 */
static int vfio_pci_hot_reset_one(VFIODevice *vdev)
{
    return vfio_pci_hot_reset(vdev, true);
}

static int vfio_pci_hot_reset_multi(VFIODevice *vdev)
{
    return vfio_pci_hot_reset(vdev, false);
}

static void vfio_pci_reset_handler(void *opaque)
{
    VFIOGroup *group;
    VFIODevice *vdev;

    QLIST_FOREACH(group, &group_list, next) {
        QLIST_FOREACH(vdev, &group->device_list, next) {
            if (!vdev->reset_works || (!vdev->has_flr && vdev->has_pm_reset)) {
                vdev->needs_reset = true;
            }
        }
    }

    QLIST_FOREACH(group, &group_list, next) {
        QLIST_FOREACH(vdev, &group->device_list, next) {
            if (vdev->needs_reset) {
                vfio_pci_hot_reset_multi(vdev);
            }
        }
    }
}

static void vfio_kvm_device_add_group(VFIOGroup *group)
{
#ifdef CONFIG_KVM
    struct kvm_device_attr attr = {
        .group = KVM_DEV_VFIO_GROUP,
        .attr = KVM_DEV_VFIO_GROUP_ADD,
        .addr = (uint64_t)(unsigned long)&group->fd,
    };

    if (!kvm_enabled()) {
        return;
    }

    if (vfio_kvm_device_fd < 0) {
        struct kvm_create_device cd = {
            .type = KVM_DEV_TYPE_VFIO,
        };

        if (kvm_vm_ioctl(kvm_state, KVM_CREATE_DEVICE, &cd)) {
            DPRINTF("KVM_CREATE_DEVICE: %m\n");
            return;
        }

        vfio_kvm_device_fd = cd.fd;
    }

    if (ioctl(vfio_kvm_device_fd, KVM_SET_DEVICE_ATTR, &attr)) {
        error_report("Failed to add group %d to KVM VFIO device: %m",
                     group->groupid);
    }
#endif
}

static void vfio_kvm_device_del_group(VFIOGroup *group)
{
#ifdef CONFIG_KVM
    struct kvm_device_attr attr = {
        .group = KVM_DEV_VFIO_GROUP,
        .attr = KVM_DEV_VFIO_GROUP_DEL,
        .addr = (uint64_t)(unsigned long)&group->fd,
    };

    if (vfio_kvm_device_fd < 0) {
        return;
    }

    if (ioctl(vfio_kvm_device_fd, KVM_SET_DEVICE_ATTR, &attr)) {
        error_report("Failed to remove group %d to KVM VFIO device: %m",
                     group->groupid);
    }
#endif
}

static int vfio_connect_container(VFIOGroup *group)
{
    VFIOContainer *container;
    int ret, fd;

    if (group->container) {
        return 0;
    }

    QLIST_FOREACH(container, &container_list, next) {
        if (!ioctl(group->fd, VFIO_GROUP_SET_CONTAINER, &container->fd)) {
            group->container = container;
            QLIST_INSERT_HEAD(&container->group_list, group, container_next);
            return 0;
        }
    }

    fd = qemu_open("/dev/vfio/vfio", O_RDWR);
    if (fd < 0) {
        error_report("vfio: failed to open /dev/vfio/vfio: %m");
        return -errno;
    }

    ret = ioctl(fd, VFIO_GET_API_VERSION);
    if (ret != VFIO_API_VERSION) {
        error_report("vfio: supported vfio version: %d, "
                     "reported version: %d", VFIO_API_VERSION, ret);
        close(fd);
        return -EINVAL;
    }

    container = g_malloc0(sizeof(*container));
    container->fd = fd;

    if (ioctl(fd, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU)) {
        ret = ioctl(group->fd, VFIO_GROUP_SET_CONTAINER, &fd);
        if (ret) {
            error_report("vfio: failed to set group container: %m");
            g_free(container);
            close(fd);
            return -errno;
        }

        ret = ioctl(fd, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU);
        if (ret) {
            error_report("vfio: failed to set iommu for container: %m");
            g_free(container);
            close(fd);
            return -errno;
        }

        container->iommu_data.type1.listener = vfio_memory_listener;
        container->iommu_data.release = vfio_listener_release;

        memory_listener_register(&container->iommu_data.type1.listener,
                                 &address_space_memory);

        if (container->iommu_data.type1.error) {
            ret = container->iommu_data.type1.error;
            vfio_listener_release(container);
            g_free(container);
            close(fd);
            error_report("vfio: memory listener initialization failed for container\n");
            return ret;
        }

        container->iommu_data.type1.initialized = true;

    } else {
        error_report("vfio: No available IOMMU models");
        g_free(container);
        close(fd);
        return -EINVAL;
    }

    QLIST_INIT(&container->group_list);
    QLIST_INSERT_HEAD(&container_list, container, next);

    group->container = container;
    QLIST_INSERT_HEAD(&container->group_list, group, container_next);

    return 0;
}

static void vfio_disconnect_container(VFIOGroup *group)
{
    VFIOContainer *container = group->container;

    if (ioctl(group->fd, VFIO_GROUP_UNSET_CONTAINER, &container->fd)) {
        error_report("vfio: error disconnecting group %d from container",
                     group->groupid);
    }

    QLIST_REMOVE(group, container_next);
    group->container = NULL;

    if (QLIST_EMPTY(&container->group_list)) {
        if (container->iommu_data.release) {
            container->iommu_data.release(container);
        }
        QLIST_REMOVE(container, next);
        DPRINTF("vfio_disconnect_container: close container->fd\n");
        close(container->fd);
        g_free(container);
    }
}

static VFIOGroup *vfio_get_group(int groupid)
{
    VFIOGroup *group;
    char path[32];
    struct vfio_group_status status = { .argsz = sizeof(status) };

    QLIST_FOREACH(group, &group_list, next) {
        if (group->groupid == groupid) {
            return group;
        }
    }

    group = g_malloc0(sizeof(*group));

    snprintf(path, sizeof(path), "/dev/vfio/%d", groupid);
    group->fd = qemu_open(path, O_RDWR);
    if (group->fd < 0) {
        error_report("vfio: error opening %s: %m", path);
        g_free(group);
        return NULL;
    }

    if (ioctl(group->fd, VFIO_GROUP_GET_STATUS, &status)) {
        error_report("vfio: error getting group status: %m");
        close(group->fd);
        g_free(group);
        return NULL;
    }

    if (!(status.flags & VFIO_GROUP_FLAGS_VIABLE)) {
        error_report("vfio: error, group %d is not viable, please ensure "
                     "all devices within the iommu_group are bound to their "
                     "vfio bus driver.", groupid);
        close(group->fd);
        g_free(group);
        return NULL;
    }

    group->groupid = groupid;
    QLIST_INIT(&group->device_list);

    if (vfio_connect_container(group)) {
        error_report("vfio: failed to setup container for group %d", groupid);
        close(group->fd);
        g_free(group);
        return NULL;
    }

    if (QLIST_EMPTY(&group_list)) {
        qemu_register_reset(vfio_pci_reset_handler, NULL);
    }

    QLIST_INSERT_HEAD(&group_list, group, next);

    vfio_kvm_device_add_group(group);

    return group;
}

static void vfio_put_group(VFIOGroup *group)
{
    if (!QLIST_EMPTY(&group->device_list)) {
        return;
    }

    vfio_kvm_device_del_group(group);
    vfio_disconnect_container(group);
    QLIST_REMOVE(group, next);
    DPRINTF("vfio_put_group: close group->fd\n");
    close(group->fd);
    g_free(group);

    if (QLIST_EMPTY(&group_list)) {
        qemu_unregister_reset(vfio_pci_reset_handler, NULL);
    }
}

static int vfio_get_device(VFIOGroup *group, VFIODevice *vdev)
{

    struct vfio_device_info dev_info = { .argsz = sizeof(dev_info) };
    struct vfio_region_info reg_info = { .argsz = sizeof(reg_info) };
    struct vfio_irq_info irq_info = { .argsz = sizeof(irq_info) };

    int ret, i;
	char dev_bdf[16]="0000:00:02.0";
	
    ret = ioctl(group->fd, VFIO_GROUP_GET_DEVICE_FD, dev_bdf);
    if (ret < 0) {
        error_report("vfio: error getting device %s from group %d: %m",
                     dev_bdf, group->groupid);
        error_printf("Verify all devices in group %d are bound to vfio-pci "
                     "or pci-stub and not already in use\n", group->groupid);
        return ret;
    }

    vdev->fd = ret;
    vdev->group = group;
    QLIST_INSERT_HEAD(&group->device_list, vdev, next);

    /* Sanity check device */
    ret = ioctl(vdev->fd, VFIO_DEVICE_GET_INFO, &dev_info);
    if (ret) {
        error_report("vfio: error getting device info: %m");
        goto error;
    }

    DPRINTF("Device %s flags: %u, regions: %u, irgs: %u\n", dev_bdf,
            dev_info.flags, dev_info.num_regions, dev_info.num_irqs);

    if (!(dev_info.flags & VFIO_DEVICE_FLAGS_PCI)) {
        error_report("vfio: Um, this isn't a PCI device");
        goto error;
    }

    vdev->reset_works = !!(dev_info.flags & VFIO_DEVICE_FLAGS_RESET);

    if (dev_info.num_regions < VFIO_PCI_CONFIG_REGION_INDEX + 1) {
        error_report("vfio: unexpected number of io regions %u",
                     dev_info.num_regions);
        goto error;
    }

    if (dev_info.num_irqs < VFIO_PCI_MSIX_IRQ_INDEX + 1) {
        error_report("vfio: unexpected number of irqs %u", dev_info.num_irqs);
        goto error;
    }

    for (i = VFIO_PCI_BAR0_REGION_INDEX; i < VFIO_PCI_ROM_REGION_INDEX; i++) {
        reg_info.index = i;

        ret = ioctl(vdev->fd, VFIO_DEVICE_GET_REGION_INFO, &reg_info);
        if (ret) {
            error_report("vfio: Error getting region %d info: %m", i);
            goto error;
        }

        DPRINTF("Device %s region %d:\n", dev_bdf, i);
        DPRINTF("  size: 0x%lx, offset: 0x%lx, flags: 0x%lx\n",
                (unsigned long)reg_info.size, (unsigned long)reg_info.offset,
                (unsigned long)reg_info.flags);

        vdev->bars[i].flags = reg_info.flags;
        vdev->bars[i].size = reg_info.size;
        vdev->bars[i].fd_offset = reg_info.offset;
        vdev->bars[i].fd = vdev->fd;
        vdev->bars[i].nr = i;
        QLIST_INIT(&vdev->bars[i].quirks);
    }

    reg_info.index = VFIO_PCI_CONFIG_REGION_INDEX;

    ret = ioctl(vdev->fd, VFIO_DEVICE_GET_REGION_INFO, &reg_info);
    if (ret) {
        error_report("vfio: Error getting config info: %m");
        goto error;
    }

    DPRINTF("Device %s config:\n", dev_bdf);
    DPRINTF("  size: 0x%lx, offset: 0x%lx, flags: 0x%lx\n",
            (unsigned long)reg_info.size, (unsigned long)reg_info.offset,
            (unsigned long)reg_info.flags);

    vdev->config_size = reg_info.size;
    if (vdev->config_size == PCI_CONFIG_SPACE_SIZE) {
        vdev->pdev.cap_present &= ~QEMU_PCI_CAP_EXPRESS;
    }
    vdev->config_offset = reg_info.offset;

  

	struct vfio_region_info vga_info = {
			.argsz = sizeof(vga_info),
			.index = VFIO_PCI_VGA_REGION_INDEX,
			};
	ret = ioctl(vdev->fd, VFIO_DEVICE_GET_REGION_INFO, &vga_info);
	if (ret) {
		error_report("vfio: Device does not support requested feature x-vga");
		goto error;
	}

	if (!(vga_info.flags & VFIO_REGION_INFO_FLAG_READ) ||
			!(vga_info.flags & VFIO_REGION_INFO_FLAG_WRITE) ||
			vga_info.size < 0xbffff + 1) {
		error_report("vfio: Unexpected VGA info, flags 0x%lx, size 0x%lx",
						(unsigned long)vga_info.flags,
						(unsigned long)vga_info.size);
		goto error;
	}

	vdev->vga.fd_offset = vga_info.offset;
	vdev->vga.fd = vdev->fd; //Assign VFIO fd to vga.fd

	vdev->vga.region[QEMU_PCI_VGA_MEM].offset = QEMU_PCI_VGA_MEM_BASE;
	vdev->vga.region[QEMU_PCI_VGA_MEM].nr = QEMU_PCI_VGA_MEM;
	QLIST_INIT(&vdev->vga.region[QEMU_PCI_VGA_MEM].quirks);

	vdev->vga.region[QEMU_PCI_VGA_IO_LO].offset = QEMU_PCI_VGA_IO_LO_BASE;
	vdev->vga.region[QEMU_PCI_VGA_IO_LO].nr = QEMU_PCI_VGA_IO_LO;
	QLIST_INIT(&vdev->vga.region[QEMU_PCI_VGA_IO_LO].quirks);

	vdev->vga.region[QEMU_PCI_VGA_IO_HI].offset = QEMU_PCI_VGA_IO_HI_BASE;
	vdev->vga.region[QEMU_PCI_VGA_IO_HI].nr = QEMU_PCI_VGA_IO_HI;
	QLIST_INIT(&vdev->vga.region[QEMU_PCI_VGA_IO_HI].quirks);

	vdev->has_vga = true;
    
    irq_info.index = VFIO_PCI_ERR_IRQ_INDEX;

    ret = ioctl(vdev->fd, VFIO_DEVICE_GET_IRQ_INFO, &irq_info);
    if (ret) {
        /* This can fail for an old kernel or legacy PCI dev */
        DPRINTF("VFIO_DEVICE_GET_IRQ_INFO failure: %m\n");
        ret = 0;
    } else if (irq_info.count == 1) {
        vdev->pci_aer = true;
    } else {
        error_report("vfio: %04x:%02x:%02x.%x "
                     "Could not enable error recovery for the device",
                     0, 0, 2, 0);
    }

error:
    if (ret) {
        QLIST_REMOVE(vdev, next);
        vdev->group = NULL;
        close(vdev->fd);
    }
    return ret;
}

static void vfio_put_device(VFIODevice *vdev)
{
    QLIST_REMOVE(vdev, next);
    vdev->group = NULL;
    DPRINTF("vfio_put_device: close vdev->fd\n");
    close(vdev->fd);
    if (vdev->msix) {
        g_free(vdev->msix);
        vdev->msix = NULL;
    }
}

static void vfio_err_notifier_handler(void *opaque)
{
    VFIODevice *vdev = opaque;

    if (!event_notifier_test_and_clear(&vdev->err_notifier)) {
        return;
    }

    /*
     * TBD. Retrieve the error details and decide what action
     * needs to be taken. One of the actions could be to pass
     * the error to the guest and have the guest driver recover
     * from the error. This requires that PCIe capabilities be
     * exposed to the guest. For now, we just terminate the
     * guest to contain the error.
     */

    error_report("%s(%04x:%02x:%02x.%x) Unrecoverable error detected.  "
                 "Please collect any data possible and then kill the guest",
                 __func__, vdev->host.domain, vdev->host.bus,
                 vdev->host.slot, vdev->host.function);

    vm_stop(RUN_STATE_IO_ERROR);
}

/*
 * Registers error notifier for devices supporting error recovery.
 * If we encounter a failure in this function, we report an error
 * and continue after disabling error recovery support for the
 * device.
 */
static void vfio_register_err_notifier(VFIODevice *vdev)
{
    int ret;
    int argsz;
    struct vfio_irq_set *irq_set;
    int32_t *pfd;

    if (!vdev->pci_aer) {
        return;
    }

    if (event_notifier_init(&vdev->err_notifier, 0)) {
        error_report("vfio: Unable to init event notifier for error detection");
        vdev->pci_aer = false;
        return;
    }

    argsz = sizeof(*irq_set) + sizeof(*pfd);

    irq_set = g_malloc0(argsz);
    irq_set->argsz = argsz;
    irq_set->flags = VFIO_IRQ_SET_DATA_EVENTFD |
                     VFIO_IRQ_SET_ACTION_TRIGGER;
    irq_set->index = VFIO_PCI_ERR_IRQ_INDEX;
    irq_set->start = 0;
    irq_set->count = 1;
    pfd = (int32_t *)&irq_set->data;

    *pfd = event_notifier_get_fd(&vdev->err_notifier);
    qemu_set_fd_handler(*pfd, vfio_err_notifier_handler, NULL, vdev);

    ret = ioctl(vdev->fd, VFIO_DEVICE_SET_IRQS, irq_set);
    if (ret) {
        error_report("vfio: Failed to set up error notification");
        qemu_set_fd_handler(*pfd, NULL, NULL, vdev);
        event_notifier_cleanup(&vdev->err_notifier);
        vdev->pci_aer = false;
    }
    g_free(irq_set);
}

static void vfio_unregister_err_notifier(VFIODevice *vdev)
{
    int argsz;
    struct vfio_irq_set *irq_set;
    int32_t *pfd;
    int ret;

    if (!vdev->pci_aer) {
        return;
    }

    argsz = sizeof(*irq_set) + sizeof(*pfd);

    irq_set = g_malloc0(argsz);
    irq_set->argsz = argsz;
    irq_set->flags = VFIO_IRQ_SET_DATA_EVENTFD |
                     VFIO_IRQ_SET_ACTION_TRIGGER;
    irq_set->index = VFIO_PCI_ERR_IRQ_INDEX;
    irq_set->start = 0;
    irq_set->count = 1;
    pfd = (int32_t *)&irq_set->data;
    *pfd = -1;

    ret = ioctl(vdev->fd, VFIO_DEVICE_SET_IRQS, irq_set);
    if (ret) {
        error_report("vfio: Failed to de-assign error fd: %m");
    }
    g_free(irq_set);
    qemu_set_fd_handler(event_notifier_get_fd(&vdev->err_notifier),
                        NULL, NULL, vdev);
    event_notifier_cleanup(&vdev->err_notifier);
}

static int is_igd_available(void)
{
	char path[PATH_MAX];
	struct stat st;
	DPRINTF("%s\n", __func__);
	snprintf(path, sizeof(path),
			  "/sys/bus/pci/devices/%04x:%02x:%02x.%01x/",
			  0, 0, 2, 0);
	 if (stat(path, &st) < 0) {
		 error_report("vfio: error: no such host device: %s", path);
		 return -errno;
	 }else
	 	return 1;

}
static int vfio_get_iommu_group_id(void)
{
	char path[PATH_MAX];
	char iommu_group_path[PATH_MAX], *group_name;
	ssize_t len;
	int groupid;
	
	snprintf(path, sizeof(path),
			  "/sys/bus/pci/devices/%04x:%02x:%02x.%01x/",
			  0, 0, 2, 0);

	strncat(path, "iommu_group", sizeof(path) - strlen(path) - 1);

	len = readlink(path, iommu_group_path, sizeof(path));
    if (len <= 0 || len >= sizeof(path)) {
        error_report("vfio: error no iommu_group for device");
        return len < 0 ? -errno : ENAMETOOLONG;
    }

    iommu_group_path[len] = 0;
	
    group_name = basename(iommu_group_path);

    if (sscanf(group_name, "%d", &groupid) != 1) {
        error_report("vfio: error reading %s: %m", path);
        return -errno;
    }
	
	DPRINTF("%s(%04x:%02x:%02x.%x) group %d\n", __func__, 0, 0, 2, 0, groupid);
	
	return groupid;

}

static int is_vfio_device_attached(VFIOGroup *group, VFIODevice *vdev)
{
	VFIODevice *pvdev;

	QLIST_FOREACH(pvdev, &group->device_list, next) {
        if (pvdev->host.domain == vdev->host.domain &&
            pvdev->host.bus == vdev->host.bus &&
            pvdev->host.slot == vdev->host.slot &&
            pvdev->host.function == vdev->host.function) {

            error_report("vfio: error: igd is already attached");
            
            return -EBUSY;
        }
    }
	
	return 1;
	
}

static void vfio_pci_load_rom(VFIODevice *vdev)
{
    struct vfio_region_info reg_info = {
        .argsz = sizeof(reg_info),
        .index = VFIO_PCI_ROM_REGION_INDEX
    };
    uint64_t size;
    off_t off = 0;
    size_t bytes;

    if (ioctl(vdev->fd, VFIO_DEVICE_GET_REGION_INFO, &reg_info)) {
        error_report("vfio: Error getting ROM info: %m");
        return;
    }

    DPRINTF("Device %04x:%02x:%02x.%x ROM:\n", 0, 0, 2, 0);
    DPRINTF("  size: 0x%lx, offset: 0x%lx, flags: 0x%lx\n",
            (unsigned long)reg_info.size, (unsigned long)reg_info.offset,
            (unsigned long)reg_info.flags);

    vdev->rom_size = size = reg_info.size;
    vdev->rom_offset = reg_info.offset;

    if (!vdev->rom_size) {
        vdev->rom_read_failed = true;
        error_report("vfio-pci: Cannot read device rom at "
                    "%04x:%02x:%02x.%x\n", 0, 0, 2, 0);
        error_printf("Device option ROM contents are probably invalid "
                    "(check dmesg).\nSkip option ROM probe with rombar=0, "
                    "or load from file with romfile=\n");
        return;
    }

    vdev->rom = g_malloc(size);
    memset(vdev->rom, 0xff, size);

    while (size) {
        bytes = pread(vdev->fd, vdev->rom + off, size, vdev->rom_offset + off);
        if (bytes == 0) {
            break;
        } else if (bytes > 0) {
            off += bytes;
            size -= bytes;
        } else {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            error_report("vfio: Error reading device ROM: %m");
            break;
        }
    }
}

static uint64_t vfio_rom_read(void *opaque, hwaddr addr, unsigned size)
{
    VFIODevice *vdev = opaque;
    uint64_t val = ((uint64_t)1 << (size * 8)) - 1;

    /* Load the ROM lazily when the guest tries to read it */
    if (unlikely(!vdev->rom)) {
        vfio_pci_load_rom(vdev);
        if (unlikely(!vdev->rom && !vdev->rom_read_failed)) {
            vfio_pci_load_rom(vdev);
        }
    }

    memcpy(&val, vdev->rom + addr,
           (addr < vdev->rom_size) ? MIN(size, vdev->rom_size - addr) : 0);

    DPRINTF("%s(%04x:%02x:%02x.%x, 0x%"HWADDR_PRIx", 0x%x) = 0x%"PRIx64"\n",
            __func__, 0, 0, 2, 0, addr, size, val);

    return val;
}

static void vfio_rom_write(void *opaque, hwaddr addr,
                           uint64_t data, unsigned size)
{
}

static const MemoryRegionOps vfio_rom_ops = {
    .read = vfio_rom_read,
    .write = vfio_rom_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void vfio_pci_size_rom(VFIODevice *vdev)
{
	PCIDevice *pdev = &vdev->pdev;
	
	int size;
	char *path;
	void *ptr;
	char name[32];
	const VMStateDescription *vmsd;
	
	if (!pdev->romfile)
		return ;
	if (strlen(pdev->romfile) == 0)
		return ;
	
	DPRINTF("%s\n", __func__);
	
    rom_add_vga(pdev->romfile);
	
	
}


static int vfio_igd_initfn(PCIDevice *pdev)
{
    VFIODevice *vdev = DO_UPCAST(VFIODevice, pdev, pdev);
	VFIOGroup *group;
	int groupID;
    int ret;

#ifndef CONFIG_INTEL_IGD_PASSTHROUGH
	error_report("vfio: Intel IGD Passthrough is not enabled");
	return -1;
#endif
	vdev->host.domain=0;
	vdev->host.bus=0;
    vdev->host.slot=2;
	vdev->host.function=0;
	
    /* Check that the host device exists */
 	if( (ret = is_igd_available()) < 0 ) 
		return ret;

   
	if( (groupID = vfio_get_iommu_group_id()) < 0 ) 
		return groupID;
	
	group = vfio_get_group(groupID); 
    if (!group) {
        error_report("vfio: failed to get group %d", groupID);
        return -ENOENT;
    }

	if( (ret = is_vfio_device_attached(group, vdev) <= 0) )
		goto out_put_group;

    ret = vfio_get_device(group, vdev);
	if (ret) {
        error_report("vfio: %s: failed to get IGD", __func__);
        goto out_put_group;
        return ret;
    }
	
	vfio_pci_size_rom(vdev);
	
    /* Get a copy of config space */
    ret = pread(vdev->fd, vdev->pdev.config,
                MIN(pci_config_size(&vdev->pdev), vdev->config_size),
                vdev->config_offset);
    if (ret < (int)MIN(pci_config_size(&vdev->pdev), vdev->config_size)) {
        ret = ret < 0 ? -errno : -EFAULT;
        error_report("vfio: Failed to read device config space");
        goto out_put;
    }

    /* vfio emulates a lot for us, but some bits need extra love */
    vdev->emulated_config_bits = g_malloc0(vdev->config_size);
	//memset(vdev->emulated_config_bits + PCI_ROM_ADDRESS, 0xff, 4);
	memset(&vdev->pdev.config[PCI_ROM_ADDRESS], 0, 4);
	memset(vdev->emulated_config_bits + 0x5c, 0xff, 4);
	*(uint32_t *)&vdev->pdev.config[0x5c] = GFX_STOLEN_BASE | 0x1;
	vdev->emulated_config_bits[0x63] = 0xf;
	vdev->pdev.config[0x63] = 0x0;
	vdev->emulated_config_bits[PCI_HEADER_TYPE] = PCI_HEADER_TYPE_MULTI_FUNCTION;
    vdev->pdev.config[PCI_HEADER_TYPE] |= PCI_HEADER_TYPE_MULTI_FUNCTION;
    

    ret = vfio_early_setup_msix(vdev);
    if (ret) {
        goto out_put;
    }

    vfio_map_bars(vdev);

    ret = vfio_add_capabilities(vdev);
    if (ret) {
        goto out_teardown;
    }

    /* QEMU emulates all of MSI & MSIX */
    if (pdev->cap_present & QEMU_PCI_CAP_MSIX) {
        memset(vdev->emulated_config_bits + pdev->msix_cap, 0xff,
               MSIX_CAP_LENGTH);
    }

    if (pdev->cap_present & QEMU_PCI_CAP_MSI) {
        memset(vdev->emulated_config_bits + pdev->msi_cap, 0xff,
               vdev->msi_cap_size);
    }

    if (vfio_pci_read_config(&vdev->pdev, PCI_INTERRUPT_PIN, 1)) {
        vdev->intx.mmap_timer = timer_new_ms(QEMU_CLOCK_VIRTUAL,
                                                  vfio_intx_mmap_enable, vdev);
        pci_device_set_intx_routing_notifier(&vdev->pdev, vfio_update_irq);
        ret = vfio_enable_intx(vdev);
        if (ret) {
            goto out_teardown;
        }
    }
	
	add_boot_device_path(vdev->bootindex, &pdev->qdev, NULL);
    vfio_register_err_notifier(vdev);

	//vnc_drawing_init(pdev);
	memset(&vdev->hw_ops, 0, sizeof(GraphicHwOps));
	graphic_console_init(DEVICE(pdev), 0, &vdev->hw_ops, NULL);

    return 0;

out_teardown:
    pci_device_set_intx_routing_notifier(&vdev->pdev, NULL);
    vfio_teardown_msi(vdev);
    vfio_unmap_bars(vdev);
out_put:
    g_free(vdev->emulated_config_bits);
    vfio_put_device(vdev);
out_put_group:
    vfio_put_group(group);
    return ret;
}

static void vfio_igd_exitfn(PCIDevice *pdev)
{
    VFIODevice *vdev = DO_UPCAST(VFIODevice, pdev, pdev);
    VFIOGroup *group = vdev->group;

    vfio_unregister_err_notifier(vdev);
    pci_device_set_intx_routing_notifier(&vdev->pdev, NULL);
    vfio_disable_interrupts(vdev);
    if (vdev->intx.mmap_timer) {
        timer_free(vdev->intx.mmap_timer);
    }
    vfio_teardown_msi(vdev);
    vfio_unmap_bars(vdev);
    g_free(vdev->emulated_config_bits);
    g_free(vdev->rom);
    vfio_put_device(vdev);
    vfio_put_group(group);
}

static void vfio_pci_reset(DeviceState *dev)
{
    PCIDevice *pdev = DO_UPCAST(PCIDevice, qdev, dev);
    VFIODevice *vdev = DO_UPCAST(VFIODevice, pdev, pdev);

    DPRINTF("%s(%04x:%02x:%02x.%x)\n\n", __func__, 0,0,2,0);

    vfio_pci_pre_reset(vdev);

    if (vdev->reset_works && (vdev->has_flr || !vdev->has_pm_reset) &&
        !ioctl(vdev->fd, VFIO_DEVICE_RESET)) {
        DPRINTF("%04x:%02x:%02x.%x FLR/VFIO_DEVICE_RESET\n", 0,0,2,0);
        goto post_reset;
    }

    /* See if we can do our own bus reset */
    if (!vfio_pci_hot_reset_one(vdev)) {
        goto post_reset;
    }

    /* If nothing else works and the device supports PM reset, use it */
    if (vdev->reset_works && vdev->has_pm_reset &&
        !ioctl(vdev->fd, VFIO_DEVICE_RESET)) {
        DPRINTF("%04x:%02x:%02x.%x PCI PM Reset\n", 0,0,2,0);
        goto post_reset;
    }

post_reset:
    vfio_pci_post_reset(vdev);
}

static Property vfio_pci_dev_properties[] = {
    DEFINE_PROP_PCI_HOST_DEVADDR("host", VFIODevice, host),
    DEFINE_PROP_UINT32("x-intx-mmap-timeout-ms", VFIODevice,
                       intx.mmap_timeout, 2100),
    DEFINE_PROP_INT32("bootindex", VFIODevice, bootindex, -1),
    DEFINE_PROP_END_OF_LIST(),
};

static const VMStateDescription vfio_pci_vmstate = {
    .name = "vfio-igd",
    .unmigratable = 1,
};

static void vfio_pci_dev_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *pdc = PCI_DEVICE_CLASS(klass);

    dc->props = vfio_pci_dev_properties;
    dc->vmsd = &vfio_pci_vmstate;
    dc->desc = "VFIO-based Intel VGA";
	dc->reset = vfio_pci_reset;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
    pdc->init = vfio_igd_initfn;
    pdc->exit = vfio_igd_exitfn;
    pdc->config_read = vfio_pci_read_config;
    pdc->config_write = vfio_pci_write_config;
    pdc->is_express = 1; /* We might be */
	//pdc->romfile = "hsw_1028_qnap.dat";
}

static const TypeInfo vfio_pci_dev_info = {
    .name = "vfio-igd",
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(VFIODevice),
    .class_init = vfio_pci_dev_class_init,
};

static void register_vfio_pci_dev_type(void)
{
    type_register_static(&vfio_pci_dev_info);
}

type_init(register_vfio_pci_dev_type)
