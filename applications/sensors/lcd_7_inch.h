#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lcd_3_5_inch.h"
#include "sensors_config.h"
#include <rtdef.h>

typedef struct
{
    rt_int16_t o2_conc;                    ///< 制氧浓度
    rt_int16_t o2_conc_set;                ///< 制氧设置浓度
    rt_int16_t sco3_o2_conc;               ///< 空间氧浓度
    rt_int16_t temperature;                ///< 温度值
    rt_int16_t humidity;                   ///< 湿度值
    rt_int16_t nai_conc;                   ///< 负氧离子浓度
    rt_int16_t pressure;                   ///< 压力值
} lcd_7_send_struct, *lcd_7_send_struct_t; // 14 bytes

typedef struct {
    rt_uint8_t o2_set_value;               ///< 制氧浓度设置值
    rt_uint8_t is_start_work;              ///< 是否开始制氧
    rt_uint8_t pressure_max_limit;         ///< 压力上限值
    rt_uint8_t inflate_time;               ///< 充气时间(分)
    rt_uint8_t deflate_time;               ///< 放气时间(分)
    rt_uint8_t is_pause_work;              ///< 是否暂停制氧
    rt_uint8_t is_inflate_status;          ///< 充放气状态 (0: NULL, 1: 充气, 0xFF: 放气)
} lcd_7_recv_struct, *lcd_7_recv_struct_t; // 7 bytes

void lcd_7_inch_init(void);

/**
 * @brief 等待接收结构体数据有效
 * @param timeout 超时时间ms
 * @return RT_OK 成功, -RT_ETIMEOUT 超时
 */
rt_err_t lcd_7_wait_recv_obj_valid(rt_int32_t timeout);

/**
 * @brief 获取lcd设置的氧浓度数据
 * @return 氧浓度数据
 */
uint8_t lcd_7_get_o2_value(void);

/**
 * @brief 获取制氧机是否开始工作
 * @return 0:开始工作 1:停止工作
 */
uint8_t lcd_7_get_o2_work(void);

/**
 * @brief 获取lcd接收到的数据
 * @return lcd_7_recv_struct
 */
lcd_7_recv_struct lcd_7_get_recv_obj(void);

#ifdef __cplusplus
}
#endif
