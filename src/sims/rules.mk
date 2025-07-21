include mk/subdir_pre.mk

$(eval $(call subdir,lpn))

ifeq ($(CONFIG_MEM_LPN), 1)
$(eval $(call subdir,mem))
endif

ifeq ($(CONFIG_PCIE_LPN), 1)
$(eval $(call subdir,pcie))
endif
$(eval $(call subdir,common))

include mk/subdir_post.mk
