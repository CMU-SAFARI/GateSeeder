HOST_DIR := host
UTIL_DIR := util
INDEX_DIR := indexing
PROFILE_DIR := profiling

INDEX := seedfarm_index
PROFILE := seedfarm_profile
HOST := seedfarm

all: build_host build_indexing build_profiling

build_host:
	$(MAKE) -C $(HOST_DIR)
	ln -fs $(HOST_DIR)/$(HOST) $(HOST)

build_indexing:
	$(MAKE) -C $(INDEX_DIR)
	ln -fs $(INDEX_DIR)/$(INDEX) $(INDEX)

build_profiling:
	$(MAKE) -C $(PROFILE_DIR)
	ln -fs $(PROFILE_DIR)/$(PROFILE) $(PROFILE)

clean:
	$(MAKE) -C $(HOST_DIR) clean
	$(MAKE) -C $(UTIL_DIR) clean
	$(MAKE) -C $(INDEX_DIR) clean
	$(MAKE) -C $(PROFILE_DIR) clean
	$(RM) $(HOST) $(INDEX) $(PROFILE)

.PHONY: all clean build_host build_indexing build_profiling
