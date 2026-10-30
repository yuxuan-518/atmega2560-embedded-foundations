#ifndef LIMIT_H
#define LIMIT_H

#include <stdint.h>
#include <stdbool.h>

#define LIMIT_ACTIVE_LEVEL 1
#define LIMIT_RELEASE_MS 20

void limit_init(void);
void limit_update(uint32_t now);
bool limit_min_raw(void);
bool limit_max_raw(void);
bool limit_min_active(void);
bool limit_max_active(void);

#endif
