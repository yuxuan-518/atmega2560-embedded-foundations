#ifndef LIMIT_H
#define LIMIT_H

#include <stdint.h>
#include <stdbool.h>

void limit_init(void);
void limit_update(uint32_t now);
bool limit_min_raw(void);
bool limit_max_raw(void);
bool limit_min_active(void);
bool limit_max_active(void);
bool limit_any_active(void);

#endif
