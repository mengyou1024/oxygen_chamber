#include "o2_conc.h"

#define DBG_TAG "O2_CONC"
#define DBG_LVL DBG_LOG

#include <rtdbg.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <semaphore.h>

static rt_device_t    uart_device        = RT_NULL;
static rt_sem_t       rx_sem             = RT_NULL;
static o2_conc_struct o2_conc_data       = {0};
static rt_bool_t      o2_conc_data_valid = RT_FALSE;
static rt_mutex_t     o2_conc_data_mutex = RT_NULL;

static rt_err_t uart_input(rt_device_t dev, rt_size_t size) {
    // 中断接收模式, 每次仅收到一个byte的数
    for (int i = 0; i < size; i++) {
        rt_sem_release(rx_sem);
    }
    return RT_EOK;
}

typedef enum {
    O2_CONC_WAIT_HEADER  = 0,
    O2_CONC_RESOLVE_DATA = 1,
    O2_CONC_CHECKSUM     = 2,
} O2_CONC_STATUS;

static void o2_conc_process_thread(void* param) {
    uint8_t        buffer[RT_SERIAL_RB_BUFSZ] = {0};
    uint8_t        buffer_len                 = 0;
    O2_CONC_STATUS status                     = O2_CONC_WAIT_HEADER;
    uint8_t        payload_buf[8]             = {0};
    uint8_t        payload_len                = 0;

    while (1) {
        rt_sem_take(rx_sem, RT_WAITING_FOREVER);
        // 读取串口数据
        buffer_len += rt_device_read(uart_device, 0, buffer + buffer_len, 1);
        switch (status) {
            case O2_CONC_WAIT_HEADER:
                if (buffer[buffer_len - 1] == 0x16) {
                    rt_sem_take(rx_sem, RT_WAITING_FOREVER);
                    // 读取长度
                    buffer_len += rt_device_read(uart_device, 0, buffer + buffer_len, 1);
                    uint8_t head = buffer[buffer_len - 1];
                    if (head != 0x09) {
                        LOG_E("head error %d", head);
                        buffer_len = 0; // 重置缓冲区
                        break;
                    }
                    rt_sem_take(rx_sem, RT_WAITING_FOREVER);
                    // 读取长度
                    buffer_len += rt_device_read(uart_device, 0, buffer + buffer_len, 1);
                    uint8_t cmd = buffer[buffer_len - 1];
                    if (cmd != 0x01) {
                        LOG_E("cmd error %d", cmd);
                        buffer_len = 0; // 重置缓冲区
                        break;
                    }
                    status      = O2_CONC_RESOLVE_DATA;
                    payload_len = 0;
                } else {
                    buffer_len = 0; // 重置缓冲区
                }
                break;

            case O2_CONC_RESOLVE_DATA:
                payload_buf[payload_len++] = buffer[buffer_len - 1];
                if (payload_len >= sizeof(payload_buf)) {
                    status      = O2_CONC_CHECKSUM;
                    payload_len = 0;
                }
                break;

            case O2_CONC_CHECKSUM: {
                uint8_t checksum = 0;
                for (int i = 0; i < sizeof(payload_buf); i++) {
                    checksum += payload_buf[i];
                }
                checksum = 0x00 - (0x20 + checksum);
                if (buffer[buffer_len - 1] == checksum) {
                    rt_mutex_take(o2_conc_data_mutex, RT_WAITING_FOREVER);
                    rt_memcpy(&o2_conc_data, payload_buf, sizeof(o2_conc_struct));
                    o2_conc_data.o2_concentration = swap_u16(o2_conc_data.o2_concentration);
                    o2_conc_data.o2_flow          = swap_u16(o2_conc_data.o2_flow);
                    o2_conc_data.temperature      = swap_u16(o2_conc_data.temperature);
                    LOG_I("O2 Concentration: %d, O2 Flow: %d, Temperature: %d\r\n",
                          o2_conc_data.o2_concentration,
                          o2_conc_data.o2_flow,
                          o2_conc_data.temperature);
                    o2_conc_data_valid = RT_TRUE;
                    rt_mutex_release(o2_conc_data_mutex);
                } else {
                    LOG_E("checksum error %#02x != recv:%#02d", checksum, buffer[buffer_len - 1]);
                    // 未验证校验和是否正确
                    rt_mutex_take(o2_conc_data_mutex, RT_WAITING_FOREVER);
                    rt_memcpy(&o2_conc_data, payload_buf, sizeof(o2_conc_struct));
                    o2_conc_data.o2_concentration = swap_u16(o2_conc_data.o2_concentration);
                    o2_conc_data.o2_flow          = swap_u16(o2_conc_data.o2_flow);
                    o2_conc_data.temperature      = swap_u16(o2_conc_data.temperature);
                    LOG_I("O2 Concentration: %d, O2 Flow: %d, Temperature: %d\r\n",
                          o2_conc_data.o2_concentration,
                          o2_conc_data.o2_flow,
                          o2_conc_data.temperature);

                    o2_conc_data_valid = RT_TRUE;
                    rt_mutex_release(o2_conc_data_mutex);
                }
                buffer_len = 0; // 重置缓冲区
                status     = O2_CONC_WAIT_HEADER;
                break;
            }

            default:
                break;
        }
    }
}

void o2_conc_init(void) {
    uart_device = rt_device_find(O2_CONC_UART);

    if (uart_device == RT_NULL) {
        LOG_E("%s not found", O2_CONC_UART);
    }

    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    config.baud_rate               = BAUD_RATE_9600;

    if (rt_device_control(uart_device, RT_DEVICE_CTRL_CONFIG, &config) != RT_EOK) {
        LOG_E("%s config failed");
    }

    // 创建信号量
    rx_sem             = rt_sem_create("o2c_sem", 0, RT_IPC_FLAG_FIFO);
    o2_conc_data_mutex = rt_mutex_create("o2c_mt", RT_IPC_FLAG_FIFO);

    rt_device_set_rx_indicate(uart_device, uart_input);

    rt_thread_t thread = rt_thread_create(
        "o2con_proc",
        o2_conc_process_thread,
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
        LOG_E("%s open failed", O2_CONC_UART);
    }
}

rt_err_t o2_conc_value_wait_valid(rt_int32_t timeout) {
    rt_tick_t start_tick = rt_tick_get();
    while (o2_conc_data_valid == RT_FALSE) {
        if (rt_tick_get() - start_tick > timeout) {
            return -RT_ETIMEOUT; // 超时
        }
        rt_thread_mdelay(100); // 等待10ms
    }
    return RT_EOK;
}

o2_conc_struct get_o2_conc_value(void) {
    o2_conc_struct ret;
    rt_mutex_take(o2_conc_data_mutex, RT_WAITING_FOREVER);
    rt_memcpy(&ret, &o2_conc_data, sizeof(o2_conc_struct));
    rt_mutex_release(o2_conc_data_mutex);
    return ret;
}
