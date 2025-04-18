#include "nai_conc.h"

#define DBG_TAG "NAI_CONC"
#define DBG_LVL DBG_LOG

#include <rtdbg.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <semaphore.h>

static rt_device_t uart_device         = RT_NULL;
static rt_sem_t    rx_sem              = RT_NULL;
static rt_uint16_t nai_conc_data       = 0;
static rt_bool_t   nai_conc_data_valid = RT_FALSE;

static void nai_conc_send_request(void) {
    if (rt_device_write(uart_device, 0, "\x01\x03\x10\x00\x00\x01\x80\xCA", 8) != 8) {
        LOG_E("send request failed");
    }
}

static rt_err_t uart_input(rt_device_t dev, rt_size_t size) {
    // 中断接收模式, 每次仅收到一个byte的数
    for (int i = 0; i < size; i++) {
        rt_sem_release(rx_sem);
    }
    return RT_EOK;
}

static void nai_conc_timeout(void* parameter) {
    nai_conc_send_request();
}

typedef enum {
    NAI_CONC_IP,
    NAI_CONC_OP,
    NAI_CONC_LEN,
    NAI_CONC_DATA,
    NAI_CONC_CRC,
} NAI_CONC_STATUS;

static void nai_conc_process_thread(void* param) {
    uint8_t         buffer[RT_SERIAL_RB_BUFSZ] = {0};
    uint8_t         buffer_len                 = 0;
    NAI_CONC_STATUS status                     = NAI_CONC_IP;
    uint8_t         payload_buf[2]             = {0};
    uint8_t         payload_len                = 0;
    uint16_t        crc_16                     = 0;

    while (1) {
        rt_sem_take(rx_sem, RT_WAITING_FOREVER);
        // 读取串口数据
        buffer_len += rt_device_read(uart_device, 0, buffer + buffer_len, 1);
        rt_kprintf("%#02X", buffer[buffer_len - 1]);
        switch (status) {
            case NAI_CONC_IP:
                if (buffer[buffer_len - 1] == 0x01) {
                    status = NAI_CONC_OP;
                } else {
                    buffer_len = 0; // 重置缓冲区
                }
                break;

            case NAI_CONC_OP:
                if (buffer[buffer_len - 1] == 0x03) {
                    status = NAI_CONC_LEN;
                } else {
                    buffer_len = 0; // 重置缓冲区
                    status     = NAI_CONC_IP;
                }
                break;

            case NAI_CONC_LEN:
                if (buffer[buffer_len - 1] == 0x02) {
                    status      = NAI_CONC_DATA;
                    payload_len = 0;
                } else {
                    buffer_len = 0; // 重置缓冲区
                    status     = NAI_CONC_IP;
                }
                break;

            case NAI_CONC_DATA:
                payload_buf[payload_len++] = buffer[buffer_len - 1];
                if (payload_len >= sizeof(payload_buf)) {
                    rt_enter_critical();
                    nai_conc_data       = (payload_buf[0] << 8) | payload_buf[1];
                    nai_conc_data_valid = RT_TRUE;
                    rt_exit_critical();
                    status      = NAI_CONC_CRC;
                    payload_len = 0;
                }
                break;

            case NAI_CONC_CRC:
                payload_buf[payload_len++] = buffer[buffer_len - 1];
                if (payload_len >= sizeof(payload_buf)) {
                    crc_16 = (payload_buf[0] << 8) | payload_buf[1];
                    LOG_D("crc_16: %#04X", crc_16);
                    status      = NAI_CONC_IP;
                    payload_len = 0;
                }
                break;

            default:
                break;
        }
    }
}

void nai_conc_init(void) {
    uart_device = rt_device_find(NAI_CONC_UART);

    if (uart_device == RT_NULL) {
        LOG_E("%s not found", NAI_CONC_UART);
    }

    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    config.baud_rate               = BAUD_RATE_9600;

    if (rt_device_control(uart_device, RT_DEVICE_CTRL_CONFIG, &config) != RT_EOK) {
        LOG_E("%s config failed");
    }

    // 创建信号量
    rx_sem = rt_sem_create("so_sem", 0, RT_IPC_FLAG_FIFO);

    rt_device_set_rx_indicate(uart_device, uart_input);

    rt_thread_t thread = rt_thread_create(
        "nai_conc",
        nai_conc_process_thread,
        RT_NULL,
        2 * 1024,
        RT_THREAD_PRIORITY_MAX / 2,
        10);

    if (thread == RT_NULL) {
        LOG_E("thread create failed");
        rt_device_close(uart_device);
    }

    if (rt_thread_startup(thread) != RT_EOK) {
        LOG_E("thread startup failed");
        rt_thread_delete(thread);
        rt_device_close(uart_device);
    }

    if (rt_device_open(uart_device, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK) {
        LOG_E("%s open failed", NAI_CONC_UART);
    }
    // 开启定时器, 定时请求数据
    rt_timer_t timer = rt_timer_create("nai_conc",
                                       nai_conc_timeout,
                                       RT_NULL,
                                       rt_tick_from_millisecond(1000),
                                       RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    if (timer == RT_NULL) {
        LOG_E("create timer failed!");
        return;
    }

    if (rt_timer_start(timer) != RT_EOK) {
        LOG_E("start timer failed!");
        return;
    }
}

rt_err_t nai_conc_wait_value_valid(rt_int32_t timeout) {
    rt_tick_t start_tick = rt_tick_get();
    while (nai_conc_data_valid == RT_FALSE) {
        if (rt_tick_get() - start_tick > timeout) {
            return -RT_ETIMEOUT; // 超时
        }
        rt_thread_mdelay(100); // 等待10ms
    }
    return RT_EOK;
}

rt_uint16_t get_nai_conc_value(void) {
    rt_uint16_t ret;
    rt_enter_critical();
    ret = nai_conc_data;
    rt_exit_critical();
    return ret;
}
