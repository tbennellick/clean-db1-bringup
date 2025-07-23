#pragma once
#include <stdbool.h>

int init_storage(void);
int storage_test(void);
bool is_storage_ready(void);
const char* get_storage_mount_point(void);
