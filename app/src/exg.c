#include "ads1298_public.h"
#include "exg_conf.h"
#include "gpio_init.h"

#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <BFP.pb.h>

LOG_MODULE_REGISTER(exg, LOG_LEVEL_DBG);

#define SAMPLES_IN_SLAB 20

#ifdef CONFIG_NOCACHE_MEMORY
#define MEM_SLAB_CACHE_ATTR __nocache
#else
#define MEM_SLAB_CACHE_ATTR
#endif

static char MEM_SLAB_CACHE_ATTR
	__aligned(WB_UP(32)) _k_mem_slab_buf_exg_0_mem_slab[(SAMPLES_IN_SLAB + 2) * WB_UP(sizeof(ads1298_sample_t))];
STRUCT_SECTION_ITERABLE(k_mem_slab, exg_0_mem_slab) = Z_MEM_SLAB_INITIALIZER(exg_0_mem_slab,
                                                                             _k_mem_slab_buf_exg_0_mem_slab,
                                                                             WB_UP(sizeof(ads1298_sample_t)),
                                                                             SAMPLES_IN_SLAB);

#define EXG_RX_THREAD_STACK_SIZE 1024
#define EXG_RX_THREAD_PRIORITY   4

#define EXG_SAMPLE_SIZE    (sizeof(((ads1298_sample_t *)0)->status) - 3)
#define PB_EXG_SAMPLE_SIZE (sizeof(((ExGDataEvent *)0)->readings))
BUILD_ASSERT(EXG_SAMPLE_SIZE <= PB_EXG_SAMPLE_SIZE,
             "ads1298_sample_t readings do not fit in ExGDataEvent readings field");

static struct k_thread exg_rx_thread_data;
K_THREAD_STACK_DEFINE(exg_rx_thread_stack, EXG_RX_THREAD_STACK_SIZE);

/* TODO, this thread could disappear if the driver is re-factored to accept the main queue via a register_queue function
 *   That will remove the extra queue and double handling. Lets get it working first. */
void exg_rx_thread_func(void *p1, void *p2, void *p3) {
	const struct device *dev_exg = (const struct device *)p1;
	struct k_msgq *main_queue = (struct k_msgq *)p2;
	void *rx_block;
	size_t rx_size;
	int ret;

	LOG_INF("EXG RX thread started");

	// Create and send event to main queue
	BaseEvent event = BaseEvent_init_default;
	event.has_event_type = true;
	event.event_type = EventType_EVENT_TYPE_EXG_DATA;
	event.has_priority = true;
	event.priority = Priority_PRIORITY_NORMAL;
	event.has_timestamp_us = true;
	event.which_event_data = BaseEvent_exg_data_event_tag;
	event.event_data.exg_data_event.has_status = true;
	event.event_data.exg_data_event.has_readings = true;
	event.event_data.exg_data_event.has_sequence_number = true;

	uint32_t sample_count = 0;

	while (1) {
		ret = i2s_read(dev_exg, &rx_block, &rx_size);
		if (ret < 0) {
			LOG_ERR("Failed to read EXG RX stream (%d)", ret);
			k_sleep(K_MSEC(10));
			continue;
		}


		// ads1298_sample_t *sample = (ads1298_sample_t *)rx_block;
		// LOG_DBG("Received sample: status=%02x%02x%02x, timestamp=%llu, sequence_number=%u",
		//         sample->status[0],
		//         sample->status[1],
		//         sample->status[2],
		//         sample->timestamp,
		//         sample->sequence_number);

		// sample_count++;
		// if (sample_count %100 == 0) {
		// 	LOG_HEXDUMP_DBG(sample->status, sizeof(sample->status), "E");
		//
		// 	// LOG_INF("EXG Sample %u: seq=%u, ts=%llu", sample_count, sample->sequence_number, sample->timestamp);
		// }


		// event.timestamp_us = sample->timestamp;
		memcpy(&event.event_data.exg_data_event.status, rx_block, 3);

		// event.event_data.exg_data_event.sequence_number = sample->sequence_number;

		memcpy(event.event_data.exg_data_event.readings, &rx_block[3],  3*8);

		ret = k_msgq_put(main_queue, &event, K_NO_WAIT);
		if (ret != 0) {
			LOG_WRN("Failed to put event to main queue: %d", ret);
		}

		k_mem_slab_free(&exg_0_mem_slab, rx_block);
	}
}

int init_exg(struct k_msgq *mq) {
	const struct device *const dev = DEVICE_DT_GET_ONE(ti_ads1298_i2s);
	struct exg_config exg_cfg;
	int ret;

	LOG_WRN("diong deffered init exg");
	ret = device_init(dev);
	if (ret < 0) {
		LOG_ERR("Deffered init of EXG failed (%d)", ret);
		return ret;
	}

	if (!device_is_ready(dev)) {
		LOG_ERR("Device %s is not ready\n", dev->name);
		k_sleep(K_MSEC(200));
	}

	exg_cfg.channels = 1;
	exg_cfg.sample_clk_freq = 250;
	exg_cfg.mem_slab = &exg_0_mem_slab;
	exg_cfg.timeout = 1000;

	ret = i2s_configure(dev, I2S_DIR_RX, (struct i2s_config *)&exg_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure I2S RX stream (%d)", ret);
		return ret;
	}

	ret = i2s_trigger(dev, I2S_DIR_RX, I2S_TRIGGER_START);
	if (ret < 0) {
		LOG_ERR("Failed to start I2S RX stream (%d)", ret);
		return ret;
	}

	/* Start a thread to handle incoming ExG data */
	k_thread_create(&exg_rx_thread_data,
	                exg_rx_thread_stack,
	                K_THREAD_STACK_SIZEOF(exg_rx_thread_stack),
	                exg_rx_thread_func,
	                (void *)dev,
	                mq,
	                NULL,
	                EXG_RX_THREAD_PRIORITY,
	                0,
	                K_NO_WAIT);

	LOG_INF("EXG Running");
	return 0;
}
