#pragma once

#include <zephyr/kernel.h>

void process_storage_queue(struct k_msgq *msgq, char *session_path);