#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "sensors_config.h"
#include <rtdef.h>

void lcd_3_5_init(void);

typedef struct {
    float battery;     ///< 电池电量
    float conc_sensor; ///< 氧浓度传感器
    float neg_ion;     ///< 负氧离子浓度
    float temperature; ///< 温度
    float humidity;    ///< 湿度
} lcd_3_5_recv_struct, *lcd_3_5_recv_struct_t;

typedef struct {
    float set_conc;    ///< 制氧设置浓度
    float oxygen_conc; ///< 氧浓度
} lcd_3_5_send_struct, *lcd_3_5_send_struct_t;

lcd_3_5_recv_struct lcd_3_5_recv_data();

rt_err_t lcd_3_5_send_data(lcd_3_5_send_struct_t data);

#ifdef __cplusplus
}
#endif
