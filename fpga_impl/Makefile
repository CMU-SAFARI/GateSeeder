vitis:
	vitis_hls -f scripts/run_hls.tcl

format:
	clang-format -i -style="{BasedOnStyle: llvm, IndentWidth: 4}" src/*.cpp src/*.hpp

clean:
	$(RM) -r work

.PHONY: vitis format clean
