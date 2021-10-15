vitis:
	vitis_hls -f scripts/run_hls.tcl

format:
	clang-format -i -style="{BasedOnStyle: llvm, IndentWidth: 4}" *.cpp *.h

clean:
	$(RM) -r seeding

.PHONY: vitis format clean
