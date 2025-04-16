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

/* defined the LED1 pin: PF6 */
#define LED1_PIN GET_PIN(B, 1)

int main(void) {
    int count = 1;

    /* set LED1 pin mode to output */
    rt_pin_mode(LED1_PIN, PIN_MODE_OUTPUT);

    while (count++) {
        rt_pin_write(LED1_PIN, PIN_HIGH);
        LOG_D("Hello World!");
        rt_thread_mdelay(1000);
        rt_pin_write(LED1_PIN, PIN_LOW);
        rt_thread_mdelay(1000);
    }

    return RT_EOK;
}
