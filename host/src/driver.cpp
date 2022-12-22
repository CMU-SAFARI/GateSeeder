#include "driver.h"
#include "util.h"
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
static xrt::bo *key;

struct d_device_t {
	xrt::bo seq;
	xrt::bo loc;
	xrt::kernel krnl;
	xrt::run run;
	volatile unsigned used : 1;
};

static d_worker_t *worker_buf;
static d_device_t *device_buf;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned NB_WORKERS;

void demeter_fpga_init(const unsigned nb_kernels, const unsigned nb_ms_key, const char *const binary_file,
                       const index_t index) {
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

	// TODO: can optimize de memory usage by using buffer created by a user-pointer for the index and then free the
	// user pointer

	//  Initialize the buffer objects for the index
	map = xrt::bo(device, MS_SIZE, device_buf->krnl.group_id(3));
	key = new xrt::bo[nb_ms_key];

	for (unsigned i = 0; i < nb_ms_key; i++) {
		key[i] = xrt::bo(device, MS_SIZE, device_buf->krnl.group_id(4 + i));
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

	// Initialize the buffers for the I/O

	// TODO: modify the parsing function, because we want to take these buffers

	for (unsigned i = 0; i < NB_WORKERS; i++) {
		uint8_t *seq;
		if (posix_memalign((void **)&seq, 4096, MS_SIZE) == 0) {
			errx(1, "%s:%d, posix_memalign", __FILE__, __LINE__);
		}
		uint64_t *loc;
		if (posix_memalign((void **)&loc, 4096, MS_SIZE) == 0) {
			errx(1, "%s:%d, posix_memalign", __FILE__, __LINE__);
		}
		device_buf[i].seq  = xrt::bo(device, seq, MS_SIZE, device_buf[i].krnl.group_id(1));
		device_buf[i].loc  = xrt::bo(device, loc, MS_SIZE, device_buf[i].krnl.group_id(2));
		device_buf[i].used = 0;
		worker_buf[i]      = {
		         .id       = i,
		         .seq      = seq,
		         .loc      = loc,
		         .len      = 0,
		         .complete = 0,
                };
	}
	// TODO: destroy the index but not in the driver
}

void demeter_host(const d_worker_t worker) {
	// Transfer input data
	const unsigned id = worker.id;
	device_buf[id].seq.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	// Start the kernel
	// TODO: switch case with the number ms for the key, or do python script, because it depends on the bitstream
	// anyway
	device_buf[id].run =
	    device_buf[id].krnl(worker.len, device_buf[id].seq, device_buf[id].loc, map, key[0], key[1]);
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
	delete[] key;
	for (unsigned i = 0; i < NB_WORKERS; i++) {
		delete[] worker_buf[i].seq;
		delete[] worker_buf[i].loc;
	}
	delete[] worker_buf;
	delete[] device_buf;
}
