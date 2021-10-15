vitis:
	vitis_hls -f scripts/run_hls.tcl

format:
	clang-format -i -style="{BasedOnStyle: llvm, IndentWidth: 4}" src/*.cpp src/*.h

clean:
	$(RM) -r seeding

.PHONY: vitis format clean
