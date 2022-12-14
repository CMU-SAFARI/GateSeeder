#include "parsing.h"
#include "util.h"
#include <err.h>
#include <iostream>
#include <xrt/xrt_bo.h>
#include <xrt/xrt_kernel.h>

unsigned SE_W        = 12;
unsigned SE_K        = 15;
unsigned IDX_B       = 26;
unsigned IDX_MAX_OCC = 500;
unsigned MS_SIZE     = 1 << 28;

#define BATCH_SIZE (1ULL << 20)
#define MS_SIZE (1ULL << 28)

void print_results(uint64_t *results) {
	uint32_t i   = 0;
	uint64_t res = results[0];
	while (res != UINT64_MAX) {
		std::cout << std::hex << i << ": " << res << std::endl;
		i++;
		res = results[i];
	}
}

int main(int argc, char *argv[]) {

	if (argc != 4) {
		errx(1, "Usage: %s <kernel.xclbin> <index.dti> <query.fastq>", argv[0]);
	}

	index_t index = parse_index(argv[2]);
	open_fastq(OPEN_MMAP, argv[3]);
	read_buf_t read_buf;
	read_buf_init(&read_buf, BATCH_SIZE);

	// Setup the Environment
	unsigned device_index  = 0;
	std::string binaryFile = argv[1];
	std::cout << "Open the device" << device_index << std::endl;
	auto device = xrt::device(device_index);
	std::cout << "Load the xclbin " << binaryFile << std::endl;
	auto uuid = device.load_xclbin(binaryFile);
	auto krnl = xrt::kernel(device, uuid, "kernel");

	// TODO: for the key create an array with the right size
	//  Create the buffer objects
	auto seq_i   = xrt::bo(device, BATCH_SIZE, krnl.group_id(1));
	auto map_i   = xrt::bo(device, MS_SIZE, krnl.group_id(2));
	auto key_0_i = xrt::bo(device, MS_SIZE, krnl.group_id(3));
	auto key_1_i = xrt::bo(device, MS_SIZE, krnl.group_id(4));
	auto buf_0_i = xrt::bo(device, MS_SIZE, xrt::bo::flags::device_only, krnl.group_id(5));
	auto buf_1_i = xrt::bo(device, MS_SIZE, xrt::bo::flags::device_only, krnl.group_id(6));
	auto out_o   = xrt::bo(device, BATCH_SIZE, krnl.group_id(7));

	// Data transfer
	map_i.write(index.map);
	map_i.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	key_0_i.write(index.key[0]);
	key_0_i.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	key_1_i.write(index.key[1]);
	key_1_i.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	// TODO: destroy the index

	uint64_t *res;
	MALLOC(res, uint64_t, (1 << 20));

	parse_fastq(&read_buf);
	seq_i.write(read_buf.seq);
	seq_i.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	std::cout << "read_buf_len: " << read_buf.len << std::endl;

	auto run = krnl(read_buf.len, seq_i, map_i, key_0_i, key_1_i, buf_0_i, buf_1_i, out_o);
	run.wait();

	out_o.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
	out_o.read(res);

	print_results(res);

	std::cout << "Finish!\n";
	return 0;
}
