#include "parsing.h"
//#include "profile.h"
#include "util.h"
#include <err.h>
#include <fstream>
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

void print_results(uint64_t *results, std::ostream &o) {
	uint32_t i   = 0;
	uint64_t res = results[0];
	while (res != UINT64_MAX) {
		o << std::hex << i << ": " << res << "\n";
		i++;
		res = results[i];
	}
}

int main(int argc, char *argv[]) {

	if (argc != 5) {
		errx(1, "Usage: %s <nb_CUs> <kernel.xclbin> <index.dti> <query.fastq>", argv[0]);
	}

	// TODO: Maybe an xrt function that tells you how many kernel
	const unsigned nb_cus = std::stoi(argv[1]);
	std::cout << "Nb CUs: " << nb_cus << std::endl;

	index_t index = parse_index(argv[3]);
	open_fastq(OPEN_MMAP, argv[4]);

	read_buf_t *read_buf = new read_buf_t[nb_cus];
	for (unsigned i = 0; i < nb_cus; i++) {
		read_buf_init(&read_buf[i], BATCH_SIZE);
	}

	// Setup the Environment
	unsigned device_index  = 0;
	std::string binaryFile = argv[2];
	std::cout << "Open the device" << device_index << std::endl;
	auto device = xrt::device(device_index);
	std::cout << "Load the xclbin " << binaryFile << std::endl;
	auto uuid = device.load_xclbin(binaryFile);

	auto krnl0 = xrt::kernel(device, uuid, "kernel:{krnl0}");
	auto krnl1 = xrt::kernel(device, uuid, "kernel:{krnl1}");
	auto krnl2 = xrt::kernel(device, uuid, "kernel:{krnl2}");
	auto krnl3 = xrt::kernel(device, uuid, "kernel:{krnl3}");

	// TODO: for the key create an array with the right size
	//  Create the buffer objects
	auto map_i   = xrt::bo(device, MS_SIZE, krnl0.group_id(2));
	auto key_0_i = xrt::bo(device, MS_SIZE, krnl0.group_id(3));
	auto key_1_i = xrt::bo(device, MS_SIZE, krnl0.group_id(4));

	auto seq_i_0 = xrt::bo(device, MS_SIZE, krnl0.group_id(1));
	auto out_o_0 = xrt::bo(device, MS_SIZE, krnl0.group_id(5));

	auto seq_i_1 = xrt::bo(device, MS_SIZE, krnl1.group_id(1));
	auto out_o_1 = xrt::bo(device, MS_SIZE, krnl1.group_id(5));

	auto seq_i_2 = xrt::bo(device, MS_SIZE, krnl2.group_id(1));
	auto out_o_2 = xrt::bo(device, MS_SIZE, krnl2.group_id(5));

	auto seq_i_3 = xrt::bo(device, MS_SIZE, krnl3.group_id(1));
	auto out_o_3 = xrt::bo(device, MS_SIZE, krnl3.group_id(5));

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

	xrt::run run_0;
	xrt::run run_1;
	xrt::run run_2;
	xrt::run run_3;

	struct timespec start_exec, end_exec, start, end;
	double host_dev_time = 0;
	double krnl_time     = 0;
	double read_time     = 0;
	double dev_host_time = 0;
	clock_gettime(CLOCK_MONOTONIC, &start_exec);

	clock_gettime(CLOCK_MONOTONIC, &start);
	int parse_0 = parse_fastq(&read_buf[0]);
	int parse_1 = parse_fastq(&read_buf[1]);
	int parse_2 = parse_fastq(&read_buf[2]);
	int parse_3 = parse_fastq(&read_buf[3]);
	clock_gettime(CLOCK_MONOTONIC, &end);
	read_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

	while (parse_0 == 0 && parse_1 == 0 && parse_2 == 0 && parse_3 == 0) {

		clock_gettime(CLOCK_MONOTONIC, &start);
		seq_i_0.write(read_buf[0].seq);
		seq_i_1.write(read_buf[1].seq);
		seq_i_2.write(read_buf[2].seq);
		seq_i_3.write(read_buf[3].seq);

		seq_i_0.sync(XCL_BO_SYNC_BO_TO_DEVICE);
		seq_i_1.sync(XCL_BO_SYNC_BO_TO_DEVICE);
		seq_i_2.sync(XCL_BO_SYNC_BO_TO_DEVICE);
		seq_i_3.sync(XCL_BO_SYNC_BO_TO_DEVICE);
		clock_gettime(CLOCK_MONOTONIC, &end);
		host_dev_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

		std::cout << "[INFO] Sequence length 0: " << read_buf[0].len << std::endl;
		std::cout << "[INFO] Sequence length 1: " << read_buf[1].len << std::endl;
		std::cout << "[INFO] Sequence length 2: " << read_buf[2].len << std::endl;
		std::cout << "[INFO] Sequence length 3: " << read_buf[3].len << std::endl;

		clock_gettime(CLOCK_MONOTONIC, &start);
		if (first_exec) {
			run_0      = krnl0(read_buf[0].len, seq_i_0, map_i, key_0_i, key_1_i, out_o_0);
			run_1      = krnl1(read_buf[1].len, seq_i_1, map_i, key_0_i, key_1_i, out_o_1);
			run_2      = krnl2(read_buf[2].len, seq_i_2, map_i, key_0_i, key_1_i, out_o_2);
			run_3      = krnl3(read_buf[3].len, seq_i_3, map_i, key_0_i, key_1_i, out_o_3);
			first_exec = false;
		} else {
			run_0.set_arg(0, read_buf[0].len);
			run_0.start();
			run_1.set_arg(0, read_buf[1].len);
			run_1.start();
			run_2.set_arg(0, read_buf[2].len);
			run_2.start();
			run_3.set_arg(0, read_buf[3].len);
			run_3.start();
		}

		run_0.wait();
		run_1.wait();
		run_2.wait();
		run_3.wait();
		clock_gettime(CLOCK_MONOTONIC, &end);
		krnl_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

		clock_gettime(CLOCK_MONOTONIC, &start);
		out_o_0.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
		out_o_1.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
		out_o_2.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
		out_o_3.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

		out_o_0.read(res);
		out_o_1.read(res);
		out_o_2.read(res);
		out_o_3.read(res);
		clock_gettime(CLOCK_MONOTONIC, &end);
		dev_host_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
		clock_gettime(CLOCK_MONOTONIC, &start);

		parse_0 = parse_fastq(&read_buf[0]);
		parse_1 = parse_fastq(&read_buf[1]);
		parse_2 = parse_fastq(&read_buf[2]);
		parse_3 = parse_fastq(&read_buf[3]);
		clock_gettime(CLOCK_MONOTONIC, &end);
		read_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
	}

	clock_gettime(CLOCK_MONOTONIC, &start);
	seq_i_0.write(read_buf[0].seq);
	seq_i_1.write(read_buf[1].seq);
	seq_i_2.write(read_buf[2].seq);
	seq_i_3.write(read_buf[3].seq);

	seq_i_0.sync(XCL_BO_SYNC_BO_TO_DEVICE);
	seq_i_1.sync(XCL_BO_SYNC_BO_TO_DEVICE);
	seq_i_2.sync(XCL_BO_SYNC_BO_TO_DEVICE);
	seq_i_3.sync(XCL_BO_SYNC_BO_TO_DEVICE);
	clock_gettime(CLOCK_MONOTONIC, &end);
	host_dev_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

	std::cout << "[INFO] Sequence length 0: " << read_buf[0].len << std::endl;
	std::cout << "[INFO] Sequence length 1: " << read_buf[1].len << std::endl;
	std::cout << "[INFO] Sequence length 2: " << read_buf[2].len << std::endl;
	std::cout << "[INFO] Sequence length 3: " << read_buf[3].len << std::endl;

	clock_gettime(CLOCK_MONOTONIC, &start);
	if (first_exec) {
		run_0      = krnl0(read_buf[0].len, seq_i_0, map_i, key_0_i, key_1_i, out_o_0);
		run_1      = krnl1(read_buf[1].len, seq_i_1, map_i, key_0_i, key_1_i, out_o_1);
		run_2      = krnl2(read_buf[2].len, seq_i_2, map_i, key_0_i, key_1_i, out_o_2);
		run_3      = krnl3(read_buf[3].len, seq_i_3, map_i, key_0_i, key_1_i, out_o_3);
		first_exec = false;
	} else {
		run_0.set_arg(0, read_buf[0].len);
		run_0.start();
		run_1.set_arg(0, read_buf[1].len);
		run_1.start();
		run_2.set_arg(0, read_buf[2].len);
		run_2.start();
		run_3.set_arg(0, read_buf[3].len);
		run_3.start();
	}

	run_0.wait();
	run_1.wait();
	run_2.wait();
	run_3.wait();
	clock_gettime(CLOCK_MONOTONIC, &end);
	krnl_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
	clock_gettime(CLOCK_MONOTONIC, &start);

	out_o_0.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
	out_o_1.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
	out_o_2.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
	out_o_3.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

	out_o_0.read(res);
	out_o_1.read(res);
	out_o_2.read(res);
	out_o_3.read(res);

	parse_0 = parse_fastq(&read_buf[0]);
	parse_1 = parse_fastq(&read_buf[1]);
	parse_2 = parse_fastq(&read_buf[2]);
	parse_3 = parse_fastq(&read_buf[3]);
	clock_gettime(CLOCK_MONOTONIC, &end);
	dev_host_time += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
	/*
	        std::ofstream file;
	        file.open("res.txt");
	        print_results(res, file);
	        file.close();
	        */

	clock_gettime(CLOCK_MONOTONIC, &end_exec);
	const double exec_time =
	    (end_exec.tv_sec - start_exec.tv_sec) + (end_exec.tv_nsec - start_exec.tv_nsec) / 1000000000.0;

	std::cout << "[INFO] Total execution time: " << exec_time << " sec" << std::endl;
	std::cout << "[INFO] Host -> Device execution time: " << host_dev_time << " sec" << std::endl;
	std::cout << "[INFO] Kernel execution time: " << krnl_time << " sec" << std::endl;
	std::cout << "[INFO] Device -> Host execution time: " << dev_host_time << " sec" << std::endl;
	std::cout << "[INFO] Read execution time: " << read_time << " sec" << std::endl;

	// TODO delete each read_buf
	delete read_buf;
	return 0;
}
