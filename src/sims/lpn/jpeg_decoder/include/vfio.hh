#ifndef GUEST_VFIO_H_
#define GUEST_VFIO_H_

#include <cstddef>
#define VFIO_GROUP "/dev/vfio/noiommu-0"

#define VFIO_API_VERSION 0

int vfio_init(const char *pci_dev);
int vfio_map_region(int dev, int idx, void **addr, size_t *len);
int vfio_get_region_info(int dev, int idx, struct vfio_region_info *reg);
int vfio_busmaster_enable(int dev);

#endif /* ndef GUEST_VFIO_H_ */
