#include "lcd_7_inch.h"

#define DBG_TAG "7inch.lcd"
#define DBG_LVL DBG_LOG

#include <rtdbg.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <semaphore.h>

static rt_device_t uart_device   = RT_NULL;
static rt_sem_t    rx_sem        = RT_NULL;

static uint8_t   _oxygen_concentration_value = 0;
static rt_bool_t _oxygen_concentration_valid = RT_FALSE;
static uint8_t   _oxygen_work                = 0;
static rt_bool_t _oxygen_work_valid          = RT_FALSE;

/* 接收数据回调函数 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size) {
    // 中断接收模式, 每次仅收到一个byte的数
    // LOG_D("%s recv isr!, %d", LCD_7_INCH_UART, size);
    for (int i = 0; i < size; i++) {
        rt_sem_release(rx_sem);
    }
    return RT_EOK;
}

// LCD发送 0x55 0x01 0x01 氧浓度设置值(1byte)  
// 是否开始制氧(1byte)  压力上限值(1byte) 充气时间(1byte) 放气时间(1byte) 是否暂停制氧(1byte) 充放气状态(1byte)
// 板卡发送 0x55 0x02 0x0C 0x00 lcd_7_send_struct 4 + 14 = 18 bytes

typedef enum {
    LCD_7_RECV_STATE_HEADER = 0,
    LCD_7_RECV_STATE_TYPE   = 1,
    LCD_7_RECV_STATE_LEN    = 2,
    LCD_7_RECV_STATE_DATA   = 3,
    LCD_7_RECV_STATE_WORK   = 4,
} LCD_7_RECV_STATE;

static void lcd_process_thread(void* param) {
    uint8_t buffer[RT_SERIAL_RB_BUFSZ] = {0};
    uint8_t buffer_len                 = 0;

    LCD_7_RECV_STATE state = LCD_7_RECV_STATE_HEADER;

    uint8_t oxygen_concentration = 0;

    while (1) {
        rt_sem_take(rx_sem, RT_WAITING_FOREVER); // 等待信号量
        rt_size_t sz = rt_device_read(uart_device, 0, buffer + buffer_len, 1);
        buffer_len += sz;
        LOG_D("recv:%#02X", buffer[buffer_len - 1]);

        switch (state) {
            case LCD_7_RECV_STATE_HEADER:
                if (buffer[buffer_len - 1] == 0x55) {
                    state = LCD_7_RECV_STATE_TYPE;
                    LOG_D("recv head");
                } else {
                    buffer_len = 0; // 重置缓冲区
                    state      = LCD_7_RECV_STATE_HEADER;
                }
                break;

            case LCD_7_RECV_STATE_TYPE:
                if (buffer[buffer_len - 1] == 0x01) {
                    state = LCD_7_RECV_STATE_LEN;
                } else {
                    buffer_len = 0; // 重置缓冲区
                    state      = LCD_7_RECV_STATE_HEADER;
                }
                break;

            case LCD_7_RECV_STATE_LEN:
                if (buffer[buffer_len - 1] == 0x01) {
                    state = LCD_7_RECV_STATE_DATA;
                } else {
                    buffer_len = 0; // 重置缓冲区
                    state      = LCD_7_RECV_STATE_HEADER;
                }
                break;

            case LCD_7_RECV_STATE_DATA:
                LOG_D("recv set:%d", buffer[buffer_len - 1]);
                oxygen_concentration        = buffer[buffer_len - 1];
                _oxygen_concentration_value = oxygen_concentration;
                _oxygen_concentration_valid = RT_TRUE;
                state                       = LCD_7_RECV_STATE_WORK;
                break;

            case LCD_7_RECV_STATE_WORK:
                LOG_D("recv work sw:%d", buffer[buffer_len - 1]);
                _oxygen_work_valid = RT_TRUE;
                _oxygen_work       = buffer[buffer_len - 1];
                buffer_len         = 0; // 重置缓冲区
                state              = LCD_7_RECV_STATE_HEADER;
                break;

            default:
                break;
        }

        // 处理数据
    }
}

void lcd_7_inch_init(void) {
    uart_device = rt_device_find(LCD_7_INCH_UART);

    if (uart_device == RT_NULL) {
        LOG_E("%s not found", LCD_7_INCH_UART);
    }

    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    if (rt_device_control(uart_device, RT_DEVICE_CTRL_CONFIG, &config) != RT_EOK) {
        LOG_E("%s config failed");
    }

    // 创建信号量
    rx_sem        = rt_sem_create("lcd_rx", 0, RT_IPC_FLAG_FIFO);

    rt_device_set_rx_indicate(uart_device, uart_input);

    rt_thread_t thread = rt_thread_create(
        "lcd_process",
        lcd_process_thread,
        RT_NULL,
        2 * 1024,
        RT_THREAD_PRIORITY_MAX / 2,
        10);

    if (thread == RT_NULL) {
        LOG_E("lcd_process thread create failed");
        rt_device_close(uart_device);
    }

    if (rt_thread_startup(thread) != RT_EOK) {
        LOG_E("lcd_process thread startup failed");
        rt_thread_delete(thread);
        rt_device_close(uart_device);
    }

    if (rt_device_open(uart_device, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK) {
        LOG_E("%s open failed", LCD_7_INCH_UART);
    }
}

rt_err_t lcd_7_send_data(lcd_7_send_struct_t data) {
    rt_device_write(uart_device, 0, "\x55\x02\x0C\x00", 4);
    rt_device_write(uart_device, 0, data, sizeof(lcd_7_send_struct));
    return RT_EOK;
}

rt_err_t lcd_7_wait_o2_value_valid(rt_int32_t timeout) {
    rt_tick_t start_tick = rt_tick_get();
    while (_oxygen_concentration_valid == RT_FALSE) {
        if (rt_tick_get() - start_tick > timeout) {
            return -RT_ETIMEOUT; // 超时
        }
        rt_thread_mdelay(100); // 等待10ms
    }
    return RT_EOK;
}

uint8_t lcd_7_get_o2_value(void) {
    return _oxygen_concentration_value;
}

rt_err_t lcd_7_wait_o2_work_valid(rt_int32_t timeout) {
    rt_tick_t start_tick = rt_tick_get();
    while (_oxygen_work_valid == RT_FALSE) {
        if (rt_tick_get() - start_tick > timeout) {
            return -RT_ETIMEOUT; // 超时
        }
        rt_thread_mdelay(100); // 等待10ms
    }
    return RT_EOK;
}

uint8_t lcd_7_get_o2_work(void) {
    return _oxygen_work;
}
