#ifndef TASK_H
#define TASK_H

#include <stdbool.h>

bool task_busy(void);
void task_abort_all(void);
void task_send_name(void);

#endif
