#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "sensors_config.h"
#include <rtdef.h>

void lcd_3_5_init(void);

typedef struct
{
    rt_int16_t data_1;
    rt_int16_t data_2;
    rt_int16_t data_3;
    rt_int16_t data_4;
    rt_int16_t data_5;
    rt_int16_t data_6;
} lcd_3_5_send_struct, *lcd_3_5_send_struct_t;

rt_err_t lcd_3_5_send_data(lcd_3_5_send_struct_t data);

#ifdef __cplusplus
}
#endif
