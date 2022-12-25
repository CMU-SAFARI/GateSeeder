#include "driver.h"
#include <err.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <xrt/xrt_bo.h>
#include <xrt/xrt_kernel.h>

#define DEVICE_INDEX 0
#define KERNEL_NAME "demeter_kernel"
#define INSTANCE_NAME "cu"
#define MS_SIZE (1ULL << 28)

static xrt::bo map;
static xrt::bo key;

struct d_device_t {
	xrt::bo seq;
	xrt::bo loc;
	xrt::kernel krnl;
	xrt::run run;
	volatile unsigned used : 1;
};

static d_worker_t *worker_buf;
static d_device_t *device_buf;

extern unsigned IDX_MAP_SIZE;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned NB_WORKERS;

void demeter_fpga_init(const unsigned nb_kernels, const char *const binary_file, const index_t index) {
	NB_WORKERS  = nb_kernels;
	auto device = xrt::device(DEVICE_INDEX);
	std::cerr << "[INFO] Device " << DEVICE_INDEX << std::endl;
	auto uuid = device.load_xclbin(binary_file);
	std::cerr << "[INFO] " << DEVICE_INDEX << " loaded\n";

	worker_buf = new d_worker_t[NB_WORKERS];
	device_buf = new d_device_t[NB_WORKERS];

	// Initialize the kernels
	const std::string pre_name = std::string(KERNEL_NAME) + std::string(":{") + INSTANCE_NAME;

	for (unsigned i = 0; i < nb_kernels; i++) {
		const std::string name = pre_name + std::to_string(i) + "}";
		device_buf[i].krnl     = xrt::kernel(device, uuid, name);
	}

	//  Initialize the buffer objects for the index
	map = xrt::bo(device, index.map, 1 << (IDX_MAP_SIZE + 2), device_buf->krnl.group_id(2));
	key = xrt::bo(device, index.key, index.key_len << 3, device_buf->krnl.group_id(3));

	// Transfer the index
	map.sync(XCL_BO_SYNC_BO_TO_DEVICE);
	key.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	// Initialize the buffers for the I/O
	for (unsigned i = 0; i < NB_WORKERS; i++) {
		// Initialize read buf
		read_buf_init(&worker_buf[i].read_buf, MS_SIZE);
		// Initialize loc buf
		POSIX_MEMALIGN(worker_buf[i].loc, 4096, MS_SIZE);

		device_buf[i].seq =
		    xrt::bo(device, worker_buf[i].read_buf.seq, MS_SIZE, device_buf[i].krnl.group_id(1));
		device_buf[i].loc      = xrt::bo(device, worker_buf[i].loc, MS_SIZE, device_buf[i].krnl.group_id(4));
		device_buf[i].used     = 0;
		worker_buf[i].id       = i;
		worker_buf[i].len      = 0;
		worker_buf[i].complete = 0;
	}
	// TODO: destroy the index but not in the driver
}

void demeter_host(const d_worker_t worker) {
	// Transfer input data
	const unsigned id = worker.id;
	device_buf[id].seq.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	// Start the kernel
	device_buf[id].run      = device_buf[id].krnl(worker.len, device_buf[id].seq, map, key, device_buf[id].loc);
	worker_buf[id].complete = 1;
	device_buf[id].used     = 0;
}

d_worker_t demeter_get_worker() {
	d_worker_t worker;
	LOCK(mutex);
	for (unsigned i = 0;; i++) {
		unsigned id = i % NB_WORKERS;
		// Check if the run is completed
		if (device_buf[id].used == 0) {
			if (worker_buf[id].complete == 0 || device_buf[id].run.state() == ERT_CMD_STATE_COMPLETED) {
				// Transfer output data
				device_buf[id].loc.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
				device_buf[id].used = 1;
				worker              = worker_buf[id];
				break;
			}
		}
	}
	UNLOCK(mutex);
	return worker;
}

int demeter_get_results(d_worker_t *worker) {
	LOCK(mutex);
	for (unsigned i = 0; i < NB_WORKERS; i++) {
		if (worker_buf[i].complete) {
			worker_buf[i].complete = 0;
			device_buf[i].run.wait();
			*worker = worker_buf[i];
			UNLOCK(mutex);
			return 1;
		}
	}
	UNLOCK(mutex);
	return 0;
}

void demeter_fpga_destroy() {
	delete &map;
	delete &key;
	for (unsigned i = 0; i < NB_WORKERS; i++) {
		read_buf_destroy(worker_buf[i].read_buf);
		delete[] worker_buf[i].loc;
	}
	delete[] worker_buf;
	delete[] device_buf;
}
