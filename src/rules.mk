include mk/subdir_pre.mk

$(eval $(call subdir,sims))
$(eval $(call subdir,accvm))
$(eval $(call subdir,drivers))
$(eval $(call subdir,exec))
$(eval $(call subdir,hw))
$(eval $(call subdir,bpf))

include mk/subdir_post.mk
