#ifndef DEMO_H
#define DEMO_H

#include <stdint.h>
#include <stdbool.h>

#define DEMO_CRUISE_MS 3000

void demo_start(void);
void demo_update(uint32_t now);
bool demo_is_running(void);
void demo_abort(void);

#endif
