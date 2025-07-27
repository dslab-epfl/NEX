include mk/subdir_pre.mk

# Include legacy subdirectories conditionally

ifeq ($(CONFIG_LEGACY_JPEG_DSIM),1)
$(eval $(call subdir,jpeg_decoder))
endif

ifeq ($(CONFIG_LEGACY_VTA_DSIM),1)
$(eval $(call subdir,vta))
endif

ifeq ($(CONFIG_LEGACY_PROTOACC_DSIM),1)
$(eval $(call subdir,protoacc))
endif

include mk/subdir_post.mk
