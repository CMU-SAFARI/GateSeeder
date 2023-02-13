#include "fpga.h"
#include <string>
#include <xrt/xrt_bo.h>
#include <xrt/xrt_kernel.h>

#define DEVICE_INDEX 0
#define KERNEL_NAME "demeter_kernel"
#define INSTANCE_NAME "cu"

#define LB_SIZE (1ULL << 29)

struct cu_t {
	read_buf_t read_buf;
	uint64_t *loc_buf;
	xrt::bo seq;
	xrt::bo loc;
	xrt::run run;
};

static xrt::device device;
static xrt::bo map;
static xrt::bo key;
static cu_t *cu_buf;

static unsigned NB_CUS;
static uint32_t BATCH_CAPACITY;

void fpga_init(const unsigned nb_cus, const uint32_t batch_capacity, const char *const binary_file,
               const index_t index) {
	NB_CUS         = nb_cus;
	BATCH_CAPACITY = batch_capacity;
	device         = xrt::device(DEVICE_INDEX);
	std::cerr << "[SFM_PROF] Device " << DEVICE_INDEX << std::endl;
	auto uuid = device.load_xclbin(binary_file);
	std::cerr << "[SFM_PROF] " << binary_file << " loaded\n";

	cu_buf            = new cu_t[NB_CUS];
	xrt::kernel *krnl = new xrt::kernel[NB_CUS];

	const std::string pre_name = std::string(KERNEL_NAME) + std::string(":{") + INSTANCE_NAME;

	// Initialize the krnls
	for (unsigned i = 0; i < nb_cus; i++) {
		const std::string name = pre_name + std::to_string(i) + "}";
		krnl[i]                = xrt::kernel(device, uuid, name);
	}

	//  Initialize the buffer objects for the index
	map = xrt::bo(device, index.map, 1ULL << (index.map_size + 2), krnl->group_id(2));
	key = xrt::bo(device, index.key, index.key_len << 3, krnl->group_id(3));

	// Transfer the index
	map.sync(XCL_BO_SYNC_BO_TO_DEVICE);
	key.sync(XCL_BO_SYNC_BO_TO_DEVICE);
	std::cerr << "[SFM_PROF] Index transferred\n";

	// Initialize the buffers for the I/O and their states
	for (unsigned i = 0; i < NB_CUS; i++) {
		// Initialize read buf
		read_buf_init(&cu_buf[i].read_buf, BATCH_CAPACITY);
		// Initialize loc buf
		POSIX_MEMALIGN(cu_buf[i].loc_buf, 4096, LB_SIZE);

		// Initialize device buffers
		cu_buf[i].seq = xrt::bo(device, cu_buf[i].read_buf.seq, BATCH_CAPACITY, krnl[i].group_id(1));
		cu_buf[i].loc = xrt::bo(device, cu_buf[i].loc_buf, LB_SIZE, krnl[i].group_id(4));

		// Initialize the run
		cu_buf[i].run = xrt::run(krnl[i]);
		cu_buf[i].run.set_arg(1, cu_buf[i].seq);
		cu_buf[i].run.set_arg(2, map);
		cu_buf[i].run.set_arg(3, key);
		cu_buf[i].run.set_arg(4, cu_buf[i].loc);
	}
	delete[] krnl;
}

static void *transfer_host_2_device_routine(void *arg) {
	cu_t *cu = (cu_t *)arg;
	cu->seq.sync(XCL_BO_SYNC_BO_TO_DEVICE);
	return (void *)NULL;
}

static void *execute_kernel_routine(void *arg) {
	cu_t *cu = (cu_t *)arg;
	cu->run.set_arg(0, cu->read_buf.len);
	cu->run.start();
	cu->run.wait();
	return (void *)NULL;
}

static void *transfer_device_2_host_routine(void *arg) {
	cu_t *cu = (cu_t *)arg;
	cu->loc.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

	uint64_t *loc = cu->loc_buf;
	/*
	for (uint32_t i = 0; i < (LB_SIZE >> 3); i++) {
	        fprintf(stdout, "LOCS:\n");
	        printf("L\t%lx\n", loc[i]);
	        if (loc[i] == UINT64_MAX) {
	                break;
	        }
	}
	*/
	return (void *)NULL;
}

int fpga_pipeline() {
	pthread_t *threads;
	MALLOC(threads, pthread_t, NB_CUS);

	// Fill the buffers
	for (unsigned i = 0; i < NB_CUS; i++) {
		if (fa_parse(&cu_buf[i].read_buf) == 1) {
			free(threads);
			return 1;
		}
	}

	PROF_INIT;
	PROF_START;
	// Transfer host to device
	for (unsigned i = 0; i < NB_CUS; i++) {
		THREAD_START(threads[i], transfer_host_2_device_routine, &cu_buf[i]);
	}
	for (unsigned i = 0; i < NB_CUS; i++) {
		THREAD_JOIN(threads[i]);
	}
	PROF_END;
	PRINT_PROF("H2D");

	PROF_START;
	// Execute the kernel
	for (unsigned i = 0; i < NB_CUS; i++) {
		THREAD_START(threads[i], execute_kernel_routine, &cu_buf[i]);
	}
	for (unsigned i = 0; i < NB_CUS; i++) {
		THREAD_JOIN(threads[i]);
	}
	PROF_END;
	PRINT_PROF("KRNL");

	PROF_START;
	// Transfer device to host
	for (unsigned i = 0; i < NB_CUS; i++) {
		THREAD_START(threads[i], transfer_device_2_host_routine, &cu_buf[i]);
	}
	for (unsigned i = 0; i < NB_CUS; i++) {
		THREAD_JOIN(threads[i]);
	}
	PROF_END;
	PRINT_PROF("D2H");

	free(threads);
	return 0;
}
