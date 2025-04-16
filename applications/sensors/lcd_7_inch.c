#include "lcd_7_inch.h"

#define DBG_TAG "7inch.lcd"
#define DBG_LVL DBG_LOG

#include <rtdbg.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <semaphore.h>

static rt_device_t uart_device   = RT_NULL;
static rt_sem_t    rx_sem        = RT_NULL;
static rt_mutex_t  lcd_dev_mutex = RT_NULL;
static rt_sem_t    recv_ack_sem  = RT_NULL;

static uint8_t   _oxygen_concentration_value = 0;
static rt_bool_t _oxygen_concentration_valid = RT_FALSE;

/* 接收数据回调函数 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size) {
    // 中断接收模式, 每次仅收到一个byte的数
    rt_sem_release(rx_sem);
    return RT_EOK;
}

// 数据格式 包头(1) + 类型(1) + 长度(1) + 数据(N) + 校验和(1)

// heartbeat
// LCD发送 0x55 0x01 0x01 氧浓度设置值(1byte)  0x00(checksum)  应答 0x56 0x01

// 更新数据
// 板卡发送 0x55 0x02 0x0C  lcd_7_send_struct  0x00(checksum) 应答 0x56 0x02

typedef enum {
    LCD_7_RECV_STATE_HEADER = 0,
    LCD_7_RECV_STATE_TYPE   = 1,
    LCD_7_RECV_STATE_LEN    = 2,
    LCD_7_RECV_STATE_DATA   = 3,
    LCD_7_RECV_STATE_CHECK  = 4,

    LCD_7_RECV_ACK = 5,
} LCD_7_RECV_STATE;

static void lcd_process_thread(void* param) {
    uint8_t buffer[RT_SERIAL_RB_BUFSZ] = {0};
    uint8_t buffer_len                 = 0;

    LCD_7_RECV_STATE state = LCD_7_RECV_STATE_HEADER;

    uint8_t oxygen_concentration = 0;

    while (1) {
        rt_sem_take(rx_sem, RT_WAITING_FOREVER); // 等待信号量

        rt_mutex_take(lcd_dev_mutex, RT_WAITING_FOREVER);
        rt_size_t sz = rt_device_read(uart_device, 0, buffer + buffer_len, 1);
        rt_mutex_release(lcd_dev_mutex);
        buffer_len += sz;

        switch (state) {
            case LCD_7_RECV_STATE_HEADER:
                if (buffer[buffer_len - 1] == 0x55) {
                    state = LCD_7_RECV_STATE_TYPE;
                } else if (buffer[buffer_len - 1] == 0x56) {
                    state = LCD_7_RECV_ACK;
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
                oxygen_concentration = buffer[buffer_len - 1];
                state                = LCD_7_RECV_STATE_CHECK;
                break;

            case LCD_7_RECV_STATE_CHECK:
                if (oxygen_concentration == buffer[buffer_len - 1]) {
                    // 校验和通过
                    // 更新数据
                    _oxygen_concentration_value = oxygen_concentration;
                    _oxygen_concentration_valid = RT_TRUE;
                    // 发送应答
                    rt_mutex_take(lcd_dev_mutex, RT_WAITING_FOREVER);
                    rt_device_write(uart_device, 0, "\x56\x01", 2);
                    rt_mutex_release(lcd_dev_mutex);
                }
                buffer_len = 0; // 重置缓冲区
                state      = LCD_7_RECV_STATE_HEADER;
                break;

            case LCD_7_RECV_ACK:
                if (buffer[buffer_len - 1] == 0x02) {
                    rt_sem_release(recv_ack_sem);
                }

                buffer_len = 0; // 重置缓冲区
                state      = LCD_7_RECV_STATE_HEADER;

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
    recv_ack_sem  = rt_sem_create("lcd_ack", 0, RT_IPC_FLAG_FIFO);
    lcd_dev_mutex = rt_mutex_create("lcd_mt", RT_IPC_FLAG_FIFO);

    rt_device_set_rx_indicate(uart_device, uart_input);

    if (rt_device_open(uart_device, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK) {
        LOG_E("%s open failed", LCD_7_INCH_UART);
    }

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
}

rt_err_t lcd_7_send_data(lcd_7_send_struct_t data) {
    rt_mutex_take(lcd_dev_mutex, RT_WAITING_FOREVER);
    rt_device_write(uart_device, 0, "\x55\x02\x0C", 3);
    rt_device_write(uart_device, 0, data, sizeof(lcd_3_5_send_struct_t));
    uint8_t check_sum = 0;
    for (int i = 0; i < sizeof(lcd_3_5_send_struct_t); i++) {
        check_sum += ((uint8_t*)data)[i];
    }
    rt_device_write(uart_device, 0, &check_sum, 1);
    rt_mutex_release(lcd_dev_mutex);
    // 等待应答
    if (rt_sem_take(recv_ack_sem, rt_tick_from_millisecond(500)) != RT_EOK) {
        LOG_E("lcd_7_send_data timeout");
        return -RT_ETIMEOUT;
    }
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
    lcd_7_wait_o2_value_valid(RT_WAITING_FOREVER);
    return _oxygen_concentration_value;
}
