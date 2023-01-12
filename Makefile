HOST_DIR := host
UTIL_DIR := util
INDEX_DIR := indexing
INDEX := demeter_index
HOST := demeter

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
	$(MAKE) -C $(EVALUATION) clean
	$(RM) $(HOST) $(INDEX)

.PHONY: all clean
