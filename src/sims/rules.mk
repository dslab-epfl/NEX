include mk/subdir_pre.mk

$(eval $(call subdir,lpn))

ifeq ($(CONFIG_MEM_LPN), 1)
$(eval $(call subdir,mem))
endif

ifeq ($(CONFIG_PCIE_LPN), 1)
$(eval $(call subdir,pcie))
endif
$(eval $(call subdir,common))

# Include legacy subfolder if any legacy DSIM options are enabled
ifeq ($(CONFIG_LEGACY_JPEG_DSIM),1)
$(eval $(call subdir,legacy))
else ifeq ($(CONFIG_LEGACY_VTA_DSIM),1)
$(eval $(call subdir,legacy))
else ifeq ($(CONFIG_LEGACY_PROTOACC_DSIM),1)
$(eval $(call subdir,legacy))
endif

include mk/subdir_post.mk
