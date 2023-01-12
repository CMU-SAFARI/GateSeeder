HOST_DIR := host
UTIL_DIR := util
HOST := demeter


host:
	$(MAKE) -C $(HOST_DIR)
	ln -fs $(HOST_DIR)/$(HOST) $(HOST)

clean:
	$(MAKE) -C $(HOST_DIR) clean
	$(MAKE) -C $(UTIL_DIR) clean
	$(RM) $(HOST)

.PHONY: host clean
