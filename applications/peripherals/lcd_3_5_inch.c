#include "lcd_3_5_inch.h"

#define DBG_TAG "3.5inch.lcd"
#define DBG_LVL DBG_LOG

#include <rtdbg.h>
#include <rtdevice.h>
#include <rtthread.h>

static rt_device_t device = RT_NULL;

void lcd_3_5_init() {
    device = rt_device_find(LCD_3_5_UART);

    if (device == RT_NULL) {
        LOG_E("%s not found", LCD_3_5_UART);
    }

    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    if (rt_device_control(device, RT_DEVICE_CTRL_CONFIG, &config) != RT_EOK) {
        LOG_E("%s config failed");
    }

    if (rt_device_open(device, RT_DEVICE_FLAG_WRONLY) != RT_EOK) {
        LOG_E("%s open failed", LCD_3_5_UART);
    }
}

rt_err_t lcd_3_5_send_data(lcd_3_5_send_struct_t data) {
    if (device == RT_NULL || data == RT_NULL) {
        LOG_E("lcd_3_5_send_data: device or data is NULL");
        return -RT_ERROR;
    }

    rt_device_write(device, 0, "\x11\x22\x33\x44", 4);
    rt_device_write(device, 0, data, sizeof(lcd_3_5_send_struct_t));
    return RT_EOK;
}
