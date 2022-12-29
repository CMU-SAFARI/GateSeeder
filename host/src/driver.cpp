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
	volatile unsigned used : 1;
};

static d_worker_t *worker_buf;
static d_device_t *device_buf;

extern unsigned IDX_MAP_SIZE;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
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

		device_buf[i].used     = 0;
		worker_buf[i].id       = i;
		worker_buf[i].running  = 0;
		worker_buf[i].input_h  = buf_empty;
		worker_buf[i].input_d  = buf_empty;
		worker_buf[i].output_d = buf_empty;
		worker_buf[i].output_h = buf_empty;

		// Initialize the mutex
		pthread_mutex_init(&worker_buf[i].mutex, NULL);
	}
}

void demeter_host(const d_worker_t worker) {
	const unsigned id = worker.id;
	worker_buf[id]    = worker;

	// TODO: measure time

	// Transfer input data
	device_buf[id].seq.write(worker.read_buf.seq);
	device_buf[id].seq.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	// Start the kernel
	// std::cout << "HOST id: " << id << std::endl;
	device_buf[id].run = device_buf[id].krnl(worker.read_buf.len, device_buf[id].seq, map, key, device_buf[id].loc);
	worker_buf[id].running = 1;
	device_buf[id].used    = 0;
}

d_worker_t demeter_get_worker() {
	d_worker_t worker;
	LOCK(mutex);
	for (unsigned i = 0;; i++) {
		unsigned id = i % NB_WORKERS;
		// Check if the run is completed
		if (device_buf[id].used == 0) {
			if (worker_buf[id].running == 0) {
				device_buf[id].used = 1;
				worker              = worker_buf[id];
				break;
			} else if (device_buf[id].run.state() == ERT_CMD_STATE_COMPLETED) {
				// std::cout << "GET id: " << id << std::endl;
				//  Transfer output data
				device_buf[id].loc.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
				device_buf[id].loc.read(worker_buf[id].loc);
				device_buf[id].used = 1;
				worker              = worker_buf[id];
				break;
			}
		}
	}
	UNLOCK(mutex);
	return worker;
}

/*
void demeter_get_results(const d_worker_t worker) {
        const unsigned id = worker.id;
        device_buf[id].loc.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
}

void demeter_release_worker(const d_worker_t worker) {
        const unsigned id       = worker.id;
        worker_buf[id].complete = 0;
        device_buf[id].used     = 0;
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
*/

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
