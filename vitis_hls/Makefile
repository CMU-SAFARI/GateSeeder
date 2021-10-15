vitis:
	vitis_hls -f run_hls.tcl

format:
	clang-format -i -style="{BasedOnStyle: llvm, IndentWidth: 4}" *.cpp *.h

clean:
	$(RM) -r seeding

.PHONY: vitis format clean
