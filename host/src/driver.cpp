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

extern uint32_t BATCH_CAPACITY;

static xrt::device device;
static xrt::bo map;
static xrt::bo key;

struct d_device_t {
	uint32_t seq_len;
	uint32_t i_batch_id;
	uint32_t o_batch_id;
	read_metadata_t *i_metadata;
	read_metadata_t *o_metadata;
	xrt::bo seq;
	xrt::bo loc;
	xrt::run run;
	volatile int is_running : 1;
};

static d_worker_t *worker_buf;
static d_device_t *device_buf;

static unsigned NB_WORKERS;

void demeter_fpga_init(const unsigned nb_cus, const char *const binary_file, const index_t index) {
	NB_WORKERS = nb_cus;
	device     = xrt::device(DEVICE_INDEX);
	std::cerr << "[INFO] Device " << DEVICE_INDEX << std::endl;
	auto uuid = device.load_xclbin(binary_file);
	std::cerr << "[INFO] " << binary_file << " loaded\n";

	worker_buf        = new d_worker_t[NB_WORKERS];
	device_buf        = new d_device_t[NB_WORKERS];
	xrt::kernel *krnl = new xrt::kernel[NB_WORKERS];

	// Initialize the kernels
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
	std::cerr << "[INFO] Index transferred\n";

	// Initialize the buffers for the I/O and their states
	for (unsigned i = 0; i < NB_WORKERS; i++) {
		// Initialize read buf
		read_buf_init(&worker_buf[i].read_buf, BATCH_CAPACITY);
		// Initialize loc buf
		POSIX_MEMALIGN(worker_buf[i].loc_buf.loc, 4096, LB_SIZE);

		device_buf[i].seq_len = 0;
		// Initialize device buffers
		device_buf[i].seq = xrt::bo(device, worker_buf[i].read_buf.seq, BATCH_CAPACITY, krnl[i].group_id(1));
		device_buf[i].loc = xrt::bo(device, worker_buf[i].loc_buf.loc, LB_SIZE, krnl[i].group_id(4));

		// Initialize the metadata buffers
		device_buf[i].i_metadata = NULL;
		device_buf[i].o_metadata = NULL;

		device_buf[i].is_running = 0;
		worker_buf[i].id         = i;
		worker_buf[i].input_h    = buf_empty;
		worker_buf[i].input_d    = buf_empty;
		worker_buf[i].output_d   = buf_empty;
		worker_buf[i].output_h   = buf_empty;

		// Initialize the mutex
		pthread_mutex_init(&worker_buf[i].mutex, NULL);

		// Initialize the run
		device_buf[i].run = xrt::run(krnl[i]);
		device_buf[i].run.set_arg(1, device_buf[i].seq);
		device_buf[i].run.set_arg(2, map);
		device_buf[i].run.set_arg(3, key);
		device_buf[i].run.set_arg(4, device_buf[i].loc);
	}
	delete[] krnl;
}

d_worker_t *demeter_get_worker(d_worker_t *const worker, const int no_input) {
	if (no_input) {
		d_worker_t *next_worker   = nullptr;
		const unsigned current_id = (worker == nullptr) ? 0 : (worker->id + 1) % NB_WORKERS;
		for (unsigned i = 0; i < NB_WORKERS; i++) {
			const unsigned id = (i + current_id) % NB_WORKERS;
			LOCK(worker_buf[id].mutex);
			if (worker_buf[id].input_h != buf_empty || worker_buf[id].input_d != buf_empty ||
			    worker_buf[id].output_d != buf_empty || worker_buf[id].output_h != buf_empty) {
				next_worker = &worker_buf[id];
				UNLOCK(worker_buf[id].mutex);
				break;
			}
			UNLOCK(worker_buf[id].mutex);
		}
		return next_worker;
	}

	if (worker == nullptr) {
		return &worker_buf[0];
	}
	const unsigned id = (worker->id + 1) % NB_WORKERS;
	return &worker_buf[id];
}

void demeter_load_seq(d_worker_t *const worker) {
	const unsigned id = worker->id;
#ifdef PROFILE
	PROF_INIT;
	PROF_START;
#endif
	device_buf[id].seq.sync(XCL_BO_SYNC_BO_TO_DEVICE);
#ifdef PROFILE
	PROF_END;
	PRINT_PROF("H2D");
#endif
	device_buf[id].seq_len    = worker->read_buf.len;
	device_buf[id].i_batch_id = worker->read_buf.batch_id;
	device_buf[id].i_metadata = worker->read_buf.metadata;
	MALLOC(worker->read_buf.metadata, read_metadata_t, worker->read_buf.metadata_capacity);
	LOCK(worker->mutex);
	worker->input_h = buf_empty;
	worker->input_d = buf_full;
	UNLOCK(worker->mutex);
}

void demeter_start_kernel(d_worker_t *const worker) {
	const unsigned id = worker->id;
	device_buf[id].run.set_arg(0, device_buf[id].seq_len);
#ifdef PROFILE
	PROF_INIT;
	PROF_START;
#endif
	device_buf[id].run.start();
#ifdef PROFILE
	device_buf[id].run.wait();
	PROF_END;
	PRINT_PROF("KRNL");
#endif
	device_buf[id].o_batch_id = device_buf[id].i_batch_id;
	device_buf[id].o_metadata = device_buf[id].i_metadata;
	device_buf[id].is_running = 1;
}
void demeter_load_loc(d_worker_t *const worker) {
	const unsigned id = worker->id;
#ifdef PROFILE
	PROF_INIT;
	PROF_START;
#endif
	device_buf[id].loc.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
#ifdef PROFILE
	PROF_END;
	PRINT_PROF("D2H");
#endif
	worker->loc_buf.batch_id = device_buf[id].o_batch_id;
	worker->loc_buf.metadata = device_buf[id].o_metadata;
	LOCK(worker->mutex);
	worker->output_d = buf_empty;
	worker->output_h = buf_full;
	UNLOCK(worker->mutex);
}

int demeter_is_complete(d_worker_t *const worker) {
	const unsigned id = worker->id;
	return !device_buf[id].is_running || device_buf[id].run.state() == ERT_CMD_STATE_COMPLETED;
}

void demeter_fpga_destroy() {
	for (unsigned i = 0; i < NB_WORKERS; i++) {
		read_buf_destroy(worker_buf[i].read_buf);
		delete[] worker_buf[i].loc_buf.loc;
	}
	delete[] worker_buf;
	delete[] device_buf;
}
