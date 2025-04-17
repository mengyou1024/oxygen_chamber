/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-02     BruceOu      first implementation
 */

#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <stdio.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG

#include <rtdbg.h>
#include "sensors/board_sensors.h"

/* defined the LED1 pin: PF6 */
#define LED1_PIN GET_PIN(B, 1)

int main(void) {

    /* set LED1 pin mode to output */
    rt_pin_mode(LED1_PIN, PIN_MODE_OUTPUT);
    //o2_conc_init();
    //sco3_o2_init();
    //gas_pressure_init();
    lcd_7_inch_init();
    lcd_7_send_struct send;
    send.data_1 = 100;
    send.data_2 = 200;
    send.data_3 = 300;
    send.data_4 = 400;
    send.data_5 = 500;
    send.data_6 = 600;

    while (1) {
        lcd_7_send_data(&send);
        // o2_conc_struct o2_conc = get_o2_conc_value();
        //rt_uint8_t press = get_gas_pressure_value();
        //rt_kprintf("press %d\r\n", get_sco3_o2_value());
        rt_pin_write(LED1_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LED1_PIN, PIN_LOW);
        rt_thread_mdelay(500);
    }
}
