#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lcd_3_5_inch.h"
#include "sensors_config.h"
#include <rtdef.h>

typedef lcd_3_5_send_struct lcd_7_send_struct;
typedef lcd_7_send_struct*  lcd_7_send_struct_t;

void lcd_7_inch_init(void);

/**
 * @brief 发送当前的氧浓度等数据
 * @param data 氧浓度等数据
 * @return rt_err_t
 */
rt_err_t lcd_7_send_data(lcd_7_send_struct_t data);

/**
 * @brief 等待lcd设置的氧浓度数据有效
 * @param timeout 超时时间ms
 * @return RT_OK 成功, -RT_ETIMEOUT 超时
 */
rt_err_t lcd_7_wait_o2_value_valid(rt_int32_t timeout);

/**
 * @brief 获取lcd设置的氧浓度数据
 * @return 氧浓度数据, 如果值无效则会阻塞等待
 */
uint8_t lcd_7_get_o2_value(void);

#ifdef __cplusplus
}
#endif
