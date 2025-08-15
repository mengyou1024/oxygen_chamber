#include "lcd_3_5_inch.h"

#define DBG_TAG "3.5inch.lcd"
#define DBG_LVL DBG_LOG

#include <cJSON.h>
#include <rtdbg.h>
#include <rtdevice.h>
#include <rtthread.h>

static rt_device_t         device      = RT_NULL;
static rt_sem_t            rx_sem      = RT_NULL;
static lcd_3_5_recv_struct s_recv_data = {0};

/* 接收数据回调函数 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size) {
    // 中断接收模式, 每次仅收到一个byte的数
    // LOG_D("%s recv isr!, %d", LCD_7_INCH_UART, size);
    for (int i = 0; i < size; i++) {
        rt_sem_release(rx_sem);
    }
    return RT_EOK;
}

static void lcd_process_thread(void* param) {
    uint8_t   buffer[RT_SERIAL_RB_BUFSZ]      = {0};
    uint8_t   buffer_len                      = 0;
    char      json_buffer[RT_SERIAL_RB_BUFSZ] = {0};
    uint8_t   json_buffer_len                 = 0;
    rt_bool_t start_parse                     = RT_FALSE;

    while (1) {
        rt_sem_take(rx_sem, RT_WAITING_FOREVER); // 等待信号量
        rt_size_t sz = rt_device_read(device, 0, buffer + buffer_len, 1);
        buffer_len += sz;
        LOG_D("recv:%#02X", buffer[buffer_len - 1]);
        if (buffer_len >= RT_SERIAL_RB_BUFSZ) {
            buffer_len = 0;
        }

        if (buffer[buffer_len - 1] == '{') {
            // 收到JSON包的起始值
            start_parse = RT_TRUE;
        }

        if (start_parse) {
            // 将数据拷贝值JSON Buffer中
            json_buffer[json_buffer_len] = buffer[buffer_len - 1];
            json_buffer_len++;
        }

        if (buffer[buffer_len - 1] == '}') {
            // 收到JSON包的结束值
            start_parse = RT_FALSE;
            // 解析JSON
            cJSON* json = cJSON_ParseWithLength(json_buffer, json_buffer_len);
            rt_enter_critical();
            // 临界区保护数据
            s_recv_data.battery     = cJSON_GetNumberValue(cJSON_GetObjectItem(json, "BATTERY"));
            s_recv_data.conc_sensor = cJSON_GetNumberValue(cJSON_GetObjectItem(json, "CONC_SENSOR"));
            s_recv_data.neg_ion     = cJSON_GetNumberValue(cJSON_GetObjectItem(json, "NEG_ION"));
            s_recv_data.temperature = cJSON_GetNumberValue(cJSON_GetObjectItem(json, "TEMP"));
            s_recv_data.humidity    = cJSON_GetNumberValue(cJSON_GetObjectItem(json, "HUMIDITY"));
            rt_exit_critical();
            cJSON_Delete(json);

            // 重置缓冲区
            buffer_len      = 0;
            json_buffer_len = 0;
        }
    }
}

void lcd_3_5_init() {
    device = rt_device_find(LCD_3_5_INCH_UART);

    if (device == RT_NULL) {
        LOG_E("%s not found", LCD_3_5_INCH_UART);
    }

    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    if (rt_device_control(device, RT_DEVICE_CTRL_CONFIG, &config) != RT_EOK) {
        LOG_E("%s config failed");
    }

    rx_sem = rt_sem_create("l3sem", 0, RT_IPC_FLAG_FIFO);

    rt_device_set_rx_indicate(device, uart_input);

    rt_thread_t thread = rt_thread_create(
        "l3p",
        lcd_process_thread,
        RT_NULL,
        2 * 1024,
        RT_THREAD_PRIORITY_MAX / 2,
        10);

    if (thread == RT_NULL) {
        LOG_E("lcd_process thread create failed");
        rt_device_close(device);
    }

    if (rt_thread_startup(thread) != RT_EOK) {
        LOG_E("lcd_process thread startup failed");
        rt_thread_delete(thread);
        rt_device_close(device);
    }

    if (rt_device_open(device, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK) {
        LOG_E("%s open failed", LCD_3_5_INCH_UART);
    }
}

lcd_3_5_recv_struct lcd_3_5_recv_data() {
    lcd_3_5_recv_struct data;

    rt_enter_critical();
    // 临界区拷贝数据
    data = s_recv_data;
    rt_exit_critical();

    return data;
}

rt_err_t lcd_3_5_send_data(lcd_3_5_send_struct_t data) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "SET_CONC", data->set_conc);
    cJSON_AddNumberToObject(root, "OXYGEN_CONC", data->oxygen_conc);
    char* out_str = cJSON_Print(root);
    rt_device_write(device, 0, out_str, rt_strlen(out_str));
    cJSON_free(out_str);
    cJSON_Delete(root);
    return RT_EOK;
}
