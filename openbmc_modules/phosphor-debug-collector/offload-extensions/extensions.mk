if ENABLE_PLDM_OFFLOAD
include offload-extensions/pldm/pldm.mk
endif

if DEFAULT_HOST_OFFLOAD
include offload-extensions/default/default.mk
endif
