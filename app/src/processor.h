#pragma once
#include <zephyr/kernel.h>

struct k_msgq *start_processing(struct k_msgq *storage_queue);