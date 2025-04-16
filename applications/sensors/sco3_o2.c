#include "sco3_o2.h"

#define DBG_TAG "SCO3_O2"
#define DBG_LVL DBG_LOG

#include <rtdbg.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <semaphore.h>

static rt_device_t uart_device        = RT_NULL;
static rt_sem_t    rx_sem             = RT_NULL;
static rt_uint16_t sco3_o2_data       = 0;
static rt_bool_t   sco3_o2_data_valid = RT_FALSE;
static rt_mutex_t  sco3_o2_data_mutex = RT_NULL;

static rt_err_t uart_input(rt_device_t dev, rt_size_t size) {
    // 中断接收模式, 每次仅收到一个byte的数
    rt_sem_release(rx_sem);
    return RT_EOK;
}

typedef enum {
    SCO3_O2_WAIT_HEADER  = 0,
    SCO3_O2_RESOLVE_DATA = 1,
    SCO3_O2_CHECKSUM     = 2,
} SCO3_O2_STATUS;

static void sco3_o2_process_thread(void* param) {
    uint8_t        buffer[RT_SERIAL_RB_BUFSZ] = {0};
    uint8_t        buffer_len                 = 0;
    SCO3_O2_STATUS status                     = SCO3_O2_WAIT_HEADER;
    uint8_t        payload_buf[8]             = {0};
    uint8_t        payload_len                = 0;

    while (1) {
        rt_sem_take(rx_sem, RT_WAITING_FOREVER);
        // 读取串口数据
        buffer_len += rt_device_read(uart_device, 0, buffer, 1);
        switch (status) {
            case SCO3_O2_WAIT_HEADER:
                if (buffer[buffer_len - 1] == 0x16) {
                    rt_sem_take(rx_sem, RT_WAITING_FOREVER);
                    // 读取长度
                    buffer_len += rt_device_read(uart_device, 0, buffer, 1);
                    uint8_t length = buffer[buffer_len - 1];
                    if (length != 0x09) {
                        LOG_E("length error %d", length);
                        buffer_len = 0; // 重置缓冲区
                        break;
                    }

                    // 读取命令符
                    buffer_len += rt_device_read(uart_device, 0, buffer, 1);
                    uint8_t cmd = buffer[buffer_len - 1];
                    if (length != 0x01) {
                        LOG_E("length CMD %d", cmd);
                        buffer_len = 0; // 重置缓冲区
                        break;
                    }

                    status      = SCO3_O2_RESOLVE_DATA;
                    payload_len = 0;
                } else {
                    buffer_len = 0; // 重置缓冲区
                }
                break;

            case SCO3_O2_RESOLVE_DATA:
                payload_buf[payload_len++] = buffer[buffer_len - 1];
                if (payload_len >= 8) {
                    status      = SCO3_O2_CHECKSUM;
                    payload_len = 0;
                }
                break;

            case SCO3_O2_CHECKSUM: {
                uint8_t checksum = 0;
                for (int i = 0; i < 8; i++) {
                    checksum += payload_buf[i];
                }
                checksum = 0x00 - (0x20 + checksum);
                if (buffer[buffer_len - 1] == checksum) {
                    rt_mutex_take(sco3_o2_data_mutex, RT_WAITING_FOREVER);
                    LOG_I("O2 Concentration: %d.%d%%", sco3_o2_data / 10, sco3_o2_data % 10);
                    rt_memcpy(&sco3_o2_data, payload_buf, sizeof(sco3_o2_data));
                    sco3_o2_data_valid = RT_TRUE;
                    rt_mutex_release(sco3_o2_data_mutex);
                } else {
                    LOG_E("checksum error %#02x != recv:%#02d", checksum, buffer[buffer_len - 1]);
                    // 未验证校验和是否正确
                    rt_mutex_take(sco3_o2_data_mutex, RT_WAITING_FOREVER);
                    LOG_I("O2 Concentration: %d.%d%%", sco3_o2_data / 10, sco3_o2_data % 10);
                    rt_memcpy(&sco3_o2_data, payload_buf, sizeof(sco3_o2_data));
                    sco3_o2_data_valid = RT_TRUE;
                    rt_mutex_release(sco3_o2_data_mutex);
                }
                buffer_len = 0; // 重置缓冲区
                status     = SCO3_O2_WAIT_HEADER;
                break;
            }

            default:
                break;
        }
    }
}

void sco3_o2_init(void) {
    uart_device = rt_device_find(SCO3_O2_UART);

    if (uart_device == RT_NULL) {
        LOG_E("%s not found", SCO3_O2_UART);
    }

    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    config.baud_rate               = BAUD_RATE_9600;

    if (rt_device_control(uart_device, RT_DEVICE_CTRL_CONFIG, &config) != RT_EOK) {
        LOG_E("%s config failed");
    }

    // 创建信号量
    rx_sem             = rt_sem_create("so_sem", 0, RT_IPC_FLAG_FIFO);
    sco3_o2_data_mutex = rt_mutex_create("so_mt", RT_IPC_FLAG_FIFO);

    rt_device_set_rx_indicate(uart_device, uart_input);

    if (rt_device_open(uart_device, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK) {
        LOG_E("%s open failed", SCO3_O2_UART);
    }

    rt_thread_t thread = rt_thread_create(
        "sco3_proc",
        sco3_o2_process_thread,
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
}

rt_err_t sco3_o2_value_valid(rt_int32_t timeout) {
    rt_tick_t start_tick = rt_tick_get();
    while (sco3_o2_data_valid == RT_FALSE) {
        if (rt_tick_get() - start_tick > timeout) {
            return -RT_ETIMEOUT; // 超时
        }
        rt_thread_mdelay(100); // 等待10ms
    }
    return RT_EOK;
}

rt_uint16_t get_sco3_o2_value(void) {
    rt_uint16_t ret;
    sco3_o2_value_valid(RT_WAITING_FOREVER);
    rt_mutex_take(sco3_o2_data_mutex, RT_WAITING_FOREVER);
    ret = sco3_o2_data;
    rt_mutex_release(sco3_o2_data_mutex);
    return ret;
}
