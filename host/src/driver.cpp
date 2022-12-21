#include "parsing.h"
#include <iostream>
#include <string>
#include <xrt/xrt_bo.h>
#include <xrt/xrt_kernel.h>

#define DEVICE_INDEX 0
#define KERNEL_NAME "kernel"
#define INSTANCE_NAME "krnl"
#define MS_SIZE (1ULL << 28)

static xrt::kernel *krnl;
static xrt::bo map;
static xrt::bo *key;

static xrt::bo *seq;
static xrt::bo *loc;

void fpga_init(const unsigned nb_kernels, const unsigned nb_ms_key, const char *const binary_file,
               const index_t index) {
	auto device = xrt::device(DEVICE_INDEX);
	std::cerr << "[INFO] Device " << DEVICE_INDEX << std::endl;
	auto uuid = device.load_xclbin(binary_file);
	std::cerr << "[INFO] " << DEVICE_INDEX << " loaded\n";

	// Initialize the kernels
	krnl                       = new xrt::kernel[nb_kernels];
	const std::string pre_name = std::string(KERNEL_NAME) + std::string(":{") + INSTANCE_NAME;

	for (unsigned i = 0; i < nb_kernels; i++) {
		const std::string name = pre_name + std::to_string(i) + "}";
		krnl[i]                = xrt::kernel(device, uuid, name);
	}

	// Initialize the buffer objects for the index
	map = xrt::bo(device, MS_SIZE, krnl->group_id(3));
	key = new xrt::bo[nb_ms_key];

	for (unsigned i = 0; i < nb_ms_key; i++) {
		key[i] = xrt::bo(device, MS_SIZE, krnl->group_id(4 + i));
	}

	// Initialize the buffer objects for the I/O
	seq = new xrt::bo[nb_kernels];
	loc = new xrt::bo[nb_kernels];
	for (unsigned i = 0; i < nb_kernels; i++) {
		seq[i] = xrt::bo(device, MS_SIZE, krnl[i].group_id(1));
		loc[i] = xrt::bo(device, MS_SIZE, krnl[i].group_id(2));
	}

	// Transfer the index
	map.write(index.map);
	for (unsigned i = 0; i < nb_ms_key; i++) {
		key[i].write(index.key[i]);
	}

	map.sync(XCL_BO_SYNC_BO_TO_DEVICE);
	for (unsigned i = 0; i < nb_ms_key; i++) {
		key[i].sync(XCL_BO_SYNC_BO_TO_DEVICE);
	}
}

void fpga_destroy() {
	delete[] krnl;
	delete &map;
	delete[] key;
	delete[] seq;
	delete[] loc;
}
