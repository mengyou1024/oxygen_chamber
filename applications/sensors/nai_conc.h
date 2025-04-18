#pragma once

// 负氧离子(negative air oxygen ion, NAI)浓度传感器

#ifdef __cplusplus
extern "C" {
#endif

#include "sensors_config.h"
#include <rtdef.h>

void nai_conc_init(void);

rt_err_t nai_conc_wait_value_valid(rt_int32_t timeout);

rt_uint16_t get_nai_conc_value(void);

#ifdef __cplusplus
}
#endif
