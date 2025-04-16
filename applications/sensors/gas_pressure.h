#pragma once

// 负氧离子(negative air oxygen ion, NAI)浓度传感器

#ifdef __cplusplus
extern "C" {
#endif

#include "sensors_config.h"
#include <rtdef.h>

void gas_pressure_init(void);

/**
 * @brief 等待气体压力数据有效
 * @param timeout 超时时间ms
 * @return RT_OK 成功, -RT_ETIMEOUT 超时
 */
rt_err_t gas_pressure_value_valid(rt_int32_t timeout);

/**
 * @brief 获取气体压力数据
 * @return 气体压力数据, 如果值无效则会阻塞等待
 * 
 */
rt_uint8_t get_gas_pressure_value(void);

#ifdef __cplusplus
}
#endif
