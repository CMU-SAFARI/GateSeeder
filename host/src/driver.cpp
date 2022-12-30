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

static xrt::device device;
static xrt::bo map;
static xrt::bo key;

struct d_device_t {
	xrt::bo seq;
	xrt::bo loc;
	xrt::kernel krnl;
	xrt::run run;
	volatile int is_running : 1;
};

static d_worker_t *worker_buf;
static d_device_t *device_buf;

// TODO: maybe delete
extern unsigned IDX_MAP_SIZE;

static unsigned NB_WORKERS;

void demeter_fpga_init(const unsigned nb_cus, const char *const binary_file, const index_t index) {
	NB_WORKERS = nb_cus;
	device     = xrt::device(DEVICE_INDEX);
	std::cerr << "[INFO] Device " << DEVICE_INDEX << std::endl;
	auto uuid = device.load_xclbin(binary_file);
	std::cerr << "[INFO] " << binary_file << " loaded\n";

	worker_buf = new d_worker_t[NB_WORKERS];
	device_buf = new d_device_t[NB_WORKERS];

	// Initialize the kernels
	const std::string pre_name = std::string(KERNEL_NAME) + std::string(":{") + INSTANCE_NAME;

	// TODO: do a single for loop
	for (unsigned i = 0; i < nb_cus; i++) {
		const std::string name = pre_name + std::to_string(i) + "}";
		device_buf[i].krnl     = xrt::kernel(device, uuid, name);
	}

	//  Initialize the buffer objects for the index
	map = xrt::bo(device, index.map, 1 << (IDX_MAP_SIZE + 2), device_buf->krnl.group_id(2));
	key = xrt::bo(device, index.key, index.key_len << 3, device_buf->krnl.group_id(3));

	// Transfer the index
	map.sync(XCL_BO_SYNC_BO_TO_DEVICE);
	key.sync(XCL_BO_SYNC_BO_TO_DEVICE);
	std::cerr << "[INFO] Index transferred\n";

	// Initialize the buffers and the states for the I/O
	for (unsigned i = 0; i < NB_WORKERS; i++) {
		// Initialize read buf
		read_buf_init(&worker_buf[i].read_buf, RB_SIZE);
		// Initialize loc buf
		POSIX_MEMALIGN(worker_buf[i].loc, 4096, MS_SIZE);

		// Initialize device buffers
		device_buf[i].seq =
		    xrt::bo(device, worker_buf[i].read_buf.seq, RB_SIZE, device_buf[i].krnl.group_id(1));
		device_buf[i].loc = xrt::bo(device, worker_buf[i].loc, MS_SIZE, device_buf[i].krnl.group_id(4));

		device_buf[i].is_running = 0;
		worker_buf[i].id         = i;
		worker_buf[i].input_h    = buf_empty;
		worker_buf[i].input_d    = buf_empty;
		worker_buf[i].output_d   = buf_empty;
		worker_buf[i].output_h   = buf_empty;

		// Initialize the mutex
		pthread_mutex_init(&worker_buf[i].mutex, NULL);

		// Initialize the run
		// TODO: maybe don't need the krnl field in device
		device_buf[i].run = xrt::run(device_buf[i].krnl);
		device_buf[i].run.set_arg(1, device_buf[i].seq);
		device_buf[i].run.set_arg(2, map);
		device_buf[i].run.set_arg(3, key);
		device_buf[i].run.set_arg(4, device_buf[i].loc);
	}
}

d_worker_t *demeter_get_worker(d_worker_t *const worker) {
	if (worker == nullptr) {
		return &worker_buf[0];
	}
	const unsigned id = (worker->id + 1) % NB_WORKERS;
	return &worker_buf[id];
}

void demeter_load_seq(d_worker_t *const worker) {
	const unsigned id = worker->id;
	device_buf[id].seq.sync(XCL_BO_SYNC_BO_TO_DEVICE);
	LOCK(worker->mutex);
	worker->input_d = buf_full;
	UNLOCK(worker->mutex);
}

void demeter_start_kernel(d_worker_t *const worker) {
	const unsigned id = worker->id;
	std::cout << "kernel[" << id << "] started" << std::endl;
	device_buf[id].run.set_arg(0, worker->read_buf.len);
	device_buf[id].run.start();
	device_buf[id].is_running = 1;
}
void demeter_load_loc(d_worker_t *const worker) {
	const unsigned id = worker->id;
	device_buf[id].loc.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
	LOCK(worker->mutex);
	worker->output_d = buf_empty;
	UNLOCK(worker->mutex);
}

int demeter_is_complete(d_worker_t *const worker) {
	const unsigned id = worker->id;
	return !device_buf[id].is_running || device_buf[id].run.state() == ERT_CMD_STATE_COMPLETED;
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
