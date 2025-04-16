#pragma once

// SC03-O2 化学氧浓度传感器

#ifdef __cplusplus
extern "C" {
#endif

#include "sensors_config.h"
#include <rtdef.h>

void sco3_o2_init(void);

/**
 * @brief 等待氧浓度数据有效
 * @param timeout 超时时间ms
 * @return RT_OK 成功, -RT_ETIMEOUT 超时
 */
rt_err_t sco3_o2_value_valid(rt_int32_t timeout);

/**
 * @brief 获取氧浓度数据
 * @return 氧浓度数据(0.1%), 如果值无效则会阻塞等待
 */
rt_uint16_t get_sco3_o2_value(void);

#ifdef __cplusplus
}
#endif
