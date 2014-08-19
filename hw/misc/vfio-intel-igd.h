/*
 * Supporting intel IGD in VFIO 
 * hjwu@qnap.com
 */
#ifndef _VFIO_VGA_INTEL_H
#define _VFIO_VGA_INTEL_H
#define IS_IGD_HASWELL(id)              (id == 0x0402 \
                                         || id == 0x0406 \
                                         || id == 0x040a \
                                         || id == 0x0412 \
                                         || id == 0x0416 \
                                         || id == 0x041a \
                                         || id == 0x0a04 \
                                         || id == 0x0a16 \
                                         || id == 0x0a22 \
                                         || id == 0x0a26 \
                                         || id == 0x0a2a )
#define IS_IGD_IVYBRIDGE(id)		    (id == 0x0162 \
                                         || id == 0x0166 \
                                         || id == 0x016a \
                                         || id == 0x0152 \
                                         || id == 0x0156 \
                                         || id == 0x015a )
#define IS_IGD_SANDYBRIDGE(id)          (id == 0x0102 \
                                         || id == 0x0106 \
                                         || id == 0x0112 \
                                         || id == 0x0116 \
                                         || id == 0x0122 \
                                         || id == 0x0126 \
                                         || id ==0x010a )
#define IS_IGD_IRONLAKE_CLARKDALE(id)	(id == 0x0042 )
#define IS_IGD_IRONLAKE_ARRANDALE(id)	(id == 0x0046 )
#define IS_IGD(id)                      (IS_IGD_IRONLAKE_CLARKDALE(id) \
                                         || IS_IGD_IRONLAKE_IRONDALE(id) \
                                         || IS_IGD_SANDYBRIDGE(id) \
                                         || IS_IGD_IVYBRIDGE(id) \
                                         || IS_IGD_HASWELL(id) )

#define IGD_BAR_MASK                    0xFFFFFFFFFFFF0000
#define DMAR_OPERATION_TIMEOUT          ((s_time_t)((_ms) * 1000000ULL))

#endif


