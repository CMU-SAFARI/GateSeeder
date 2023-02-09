HOST_DIR := host
UTIL_DIR := util
INDEX_DIR := indexing
INDEX := seedfarm_index
HOST := seedfarm

all: build_host build_indexing

build_host:
	$(MAKE) -C $(HOST_DIR)
	ln -fs $(HOST_DIR)/$(HOST) $(HOST)

build_indexing:
	$(MAKE) -C $(INDEX_DIR)
	ln -fs $(INDEX_DIR)/$(INDEX) $(INDEX)

clean:
	$(MAKE) -C $(HOST_DIR) clean
	$(MAKE) -C $(UTIL_DIR) clean
	$(MAKE) -C $(INDEX_DIR) clean
	$(RM) $(HOST) $(INDEX)

.PHONY: all clean
