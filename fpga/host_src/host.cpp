#include "parsing.h"
#include "util.h"
#include <err.h>
#include <iostream>
#include <time.h>
#include <xrt/xrt_bo.h>
#include <xrt/xrt_kernel.h>

unsigned SE_W        = 12;
unsigned SE_K        = 15;
unsigned IDX_B       = 26;
unsigned IDX_MAX_OCC = 500;
unsigned MS_SIZE     = 1 << 28;

#define BATCH_SIZE (1ULL << 25)
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
	auto seq_i   = xrt::bo(device, MS_SIZE, krnl.group_id(1));
	auto map_i   = xrt::bo(device, MS_SIZE, krnl.group_id(2));
	auto key_0_i = xrt::bo(device, MS_SIZE, krnl.group_id(3));
	auto key_1_i = xrt::bo(device, MS_SIZE, krnl.group_id(4));
	auto out_0_o = xrt::bo(device, MS_SIZE, krnl.group_id(5));
	auto out_1_o = xrt::bo(device, MS_SIZE, krnl.group_id(6));

	std::cout << "Alloc done\n";

	// Data transfer
	map_i.write(index.map);
	map_i.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	key_0_i.write(index.key[0]);
	key_0_i.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	key_1_i.write(index.key[1]);
	key_1_i.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	// TODO: destroy the index

	uint64_t *res;
	MALLOC(res, uint64_t, (1 << 30));

	bool first_exec = true;
	xrt::run run;

	struct timespec start_exec, end_exec, start, end;
	double host_dev_time = 0;
	double krnl_time     = 0;
	double read_time     = 0;
	double dev_host_time = 0;
	clock_gettime(CLOCK_MONOTONIC, &start_exec);

	clock_gettime(CLOCK_MONOTONIC, &start);
	while (parse_fastq(&read_buf) == 0) {
		clock_gettime(CLOCK_MONOTONIC, &end);
		read_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

		clock_gettime(CLOCK_MONOTONIC, &start);
		seq_i.write(read_buf.seq);
		seq_i.sync(XCL_BO_SYNC_BO_TO_DEVICE);
		clock_gettime(CLOCK_MONOTONIC, &end);

		host_dev_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
		std::cout << "[INFO] Sequence length: " << read_buf.len << std::endl;

		clock_gettime(CLOCK_MONOTONIC, &start);
		if (first_exec) {
			run        = krnl(read_buf.len, seq_i, map_i, key_0_i, key_1_i, out_0_o, out_1_o);
			first_exec = false;
		} else {
			run.set_arg(0, read_buf.len);
			run.start();
		}
		run.wait();
		clock_gettime(CLOCK_MONOTONIC, &end);
		krnl_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

		clock_gettime(CLOCK_MONOTONIC, &start);
		out_0_o.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
		out_0_o.read(res);
		clock_gettime(CLOCK_MONOTONIC, &end);
		dev_host_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
		clock_gettime(CLOCK_MONOTONIC, &start);
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	read_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

	clock_gettime(CLOCK_MONOTONIC, &start);
	seq_i.write(read_buf.seq);
	seq_i.sync(XCL_BO_SYNC_BO_TO_DEVICE);
	clock_gettime(CLOCK_MONOTONIC, &end);

	host_dev_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
	std::cout << "[INFO] Sequence length: " << read_buf.len << std::endl;

	clock_gettime(CLOCK_MONOTONIC, &start);
	if (first_exec) {
		run        = krnl(read_buf.len, seq_i, map_i, key_0_i, key_1_i, out_0_o, out_1_o);
		first_exec = false;
	} else {
		run.set_arg(0, read_buf.len);
		run.start();
	}
	run.wait();
	clock_gettime(CLOCK_MONOTONIC, &end);
	krnl_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

	clock_gettime(CLOCK_MONOTONIC, &start);
	out_0_o.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
	out_0_o.read(res);
	clock_gettime(CLOCK_MONOTONIC, &end);
	dev_host_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
	clock_gettime(CLOCK_MONOTONIC, &end_exec);
	const double exec_time =
	    (end_exec.tv_sec - start_exec.tv_sec) + (end_exec.tv_nsec - start_exec.tv_nsec) / 1000000000.0;

	std::cout << "[INFO] Total execution time: " << exec_time << " sec" << std::endl;
	std::cout << "[INFO] Host -> Device execution time: " << host_dev_time << " sec" << std::endl;
	std::cout << "[INFO] Kernel execution time: " << krnl_time << " sec" << std::endl;
	std::cout << "[INFO] Device -> Host execution time: " << dev_host_time << " sec" << std::endl;
	std::cout << "[INFO] Read execution time: " << read_time << " sec" << std::endl;
	return 0;
}
