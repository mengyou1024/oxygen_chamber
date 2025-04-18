#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "sensors_config.h"
#include <rtdef.h>

void lcd_3_5_init(void);

typedef struct
{
    rt_int16_t o2_conc;      ///< 制氧浓度
    rt_int16_t o2_conc_set;  ///< 制氧设置浓度
    rt_int16_t sco3_o2_conc; ///< 空间氧浓度
    rt_int16_t temperature;  ///< 温度值
    rt_int16_t humidity;     ///< 湿度值
    rt_int16_t nai_conc;     ///< 负氧离子浓度
} lcd_3_5_send_struct, *lcd_3_5_send_struct_t;

rt_err_t lcd_3_5_send_data(lcd_3_5_send_struct_t data);

#ifdef __cplusplus
}
#endif
