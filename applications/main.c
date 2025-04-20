#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <stdio.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG

#include "controler/board_controler.h"
#include "sensors/board_sensors.h"
#include <dhtxx.h>
#include <rtdbg.h>

// 气体压力传感器
#define CONFIG_PRESSURE_SENSOR (15)

// 初始化看门狗定时器
static void wdt_init(void);
/* 分子筛循环 */
static void molecular_sieve_loop(void* parameter);
/* 关闭制样泵 */
static void close_oxygen_generation_pump_oneshot(void* parameter);

int main(void) {
    wdt_init();
    // 初始化引脚
    rt_pin_mode(OXYGEN_GENERATION_PUMP_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(BOOST_PUMP_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(NEGATIVE_OXYGEN_PUMP_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(ALARM_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LAMP_1_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LAMP_2_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(MOLECULAR_SIEVE_1_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(MOLECULAR_SIEVE_2_PIN, PIN_MODE_OUTPUT);

    struct dht_device dht11_dev = {0};
    dht_init(&dht11_dev, DHT11_PIN);
    lcd_3_5_init();
    lcd_7_inch_init();
    nai_conc_init();
    sco3_o2_init();
    o2_conc_init();
    gas_pressure_init();

    lcd_3_5_send_struct lcd_3_5_data = {0};

    // 分子筛循环定时器
    rt_timer_t molecular_sieve_loop_timer = rt_timer_create("msl",
                                                            molecular_sieve_loop,
                                                            RT_NULL,
                                                            rt_tick_from_millisecond(1000),
                                                            RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);

    rt_timer_t close_oxygen_generation_pump_timer = rt_timer_create("cogp",
                                                                    close_oxygen_generation_pump_oneshot,
                                                                    RT_NULL,
                                                                    rt_tick_from_millisecond(1000 * 60 * 5), /*  5 minutes */
                                                                    RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_ONE_SHOT);

    while (1) {
        dht_read(&dht11_dev);

        // 氧气浓度设置值 1.0%
        uint8_t lcd7_o2_set = lcd_7_get_o2_value();
        // 开始制氧
        uint8_t lcd7_o2_work = lcd_7_get_o2_work();
        // 气体压力
        rt_uint8_t gas_pressure_value = get_gas_pressure_value();
        // 负氧离子浓度值 10个/cm³
        rt_uint16_t nai_conc_value = get_nai_conc_value();
        // 氧浓度、流量
        o2_conc_struct o2_conc_value = get_o2_conc_value();
        // 氧浓度 0.1%
        rt_uint16_t sco3_o2_conc_value = get_sco3_o2_value();
        // 温度值 0.1°C
        rt_uint16_t temperture = dht_get_temperature(&dht11_dev);
        // 湿度值 0.1%
        rt_uint16_t humidity = dht_get_humidity(&dht11_dev);

        // 更新LCD显示的数据
        lcd_3_5_data.o2_conc      = o2_conc_value.o2_concentration;
        lcd_3_5_data.o2_conc_set  = lcd7_o2_set * 10;
        lcd_3_5_data.sco3_o2_conc = sco3_o2_conc_value;
        lcd_3_5_data.temperature  = temperture;
        lcd_3_5_data.humidity     = humidity;
        lcd_3_5_data.nai_conc     = nai_conc_value * 10;
        lcd_3_5_send_data(&lcd_3_5_data);
        lcd_7_send_data(&lcd_3_5_data);

        // 开始制氧气
        if (lcd7_o2_work == 1) {
            // 如果被设置了5分钟后关闭制氧泵, 则取消
            if (close_oxygen_generation_pump_timer->parent.flag & RT_TIMER_FLAG_ACTIVATED) {
                // 如果定时器没有启动, 启动定时器
                rt_timer_stop(close_oxygen_generation_pump_timer);
            }

            // 开始制氧后, 制氧泵、负氧泵工作
            rt_pin_write(OXYGEN_GENERATION_PUMP_PIN, PIN_HIGH);
            rt_pin_write(NEGATIVE_OXYGEN_PUMP_PIN, PIN_HIGH);

            if (gas_pressure_value < CONFIG_PRESSURE_SENSOR) {
                // 压力小于设定值时, 报警、1#灯、2#灯 输出 12V
                rt_pin_write(ALARM_PIN, PIN_HIGH);
                rt_pin_write(LAMP_1_PIN, PIN_HIGH);
                rt_pin_write(LAMP_2_PIN, PIN_HIGH);
            } else if (gas_pressure_value >= CONFIG_PRESSURE_SENSOR) {
                // 压力达到设定值时, 报警 输出 0V
                rt_pin_write(ALARM_PIN, PIN_LOW);
                // 分子筛循环定时器开始工作
                if (!(molecular_sieve_loop_timer->parent.flag & RT_TIMER_FLAG_ACTIVATED)) {
                    // 如果定时器没有启动, 启动定时器
                    rt_timer_start(molecular_sieve_loop_timer);
                }
                // TAG: 正常制氧状态
                while (lcd7_o2_work) {
                    dht_read(&dht11_dev);

                    lcd7_o2_set        = lcd_7_get_o2_value();
                    lcd7_o2_work       = lcd_7_get_o2_work();
                    gas_pressure_value = get_gas_pressure_value();
                    nai_conc_value     = get_nai_conc_value();
                    o2_conc_value      = get_o2_conc_value();
                    sco3_o2_conc_value = get_sco3_o2_value();
                    temperture         = dht_get_temperature(&dht11_dev);
                    humidity           = dht_get_humidity(&dht11_dev);

                    // 更新LCD显示的数据
                    lcd_3_5_data.o2_conc      = o2_conc_value.o2_concentration;
                    lcd_3_5_data.o2_conc_set  = lcd7_o2_set * 10;
                    lcd_3_5_data.sco3_o2_conc = sco3_o2_conc_value;
                    lcd_3_5_data.temperature  = temperture;
                    lcd_3_5_data.humidity     = humidity;
                    lcd_3_5_data.nai_conc     = nai_conc_value * 10;
                    lcd_3_5_send_data(&lcd_3_5_data);
                    lcd_7_send_data(&lcd_3_5_data);

                    // 监控空间氧浓度
                    if (sco3_o2_conc_value > lcd7_o2_set) {
                        // 氧浓度大于设定值, 负氧泵停止工作
                        rt_pin_write(OXYGEN_GENERATION_PUMP_PIN, PIN_LOW);
                    } else if (sco3_o2_conc_value < lcd7_o2_set - 2) {
                        // 氧浓度小于设定值, 负氧泵工作
                        rt_pin_write(OXYGEN_GENERATION_PUMP_PIN, PIN_HIGH);
                    }
                    // 监控气体压力传感器
                    if (gas_pressure_value < CONFIG_PRESSURE_SENSOR - 10) {
                        // 压力小于设定值时, 报警、1#灯、2#灯 输出 12V
                        rt_pin_write(ALARM_PIN, PIN_HIGH);
                        rt_pin_write(LAMP_1_PIN, PIN_HIGH);
                        rt_pin_write(LAMP_2_PIN, PIN_HIGH);
                    } else {
                        rt_pin_write(ALARM_PIN, PIN_LOW);
                    }

                    rt_thread_mdelay(500);
                }
            }
        } else {
            // 停止制氧,
            rt_pin_write(ALARM_PIN, PIN_LOW);
            rt_pin_write(LAMP_1_PIN, PIN_LOW);
            rt_pin_write(LAMP_2_PIN, PIN_LOW);
            rt_pin_write(NEGATIVE_OXYGEN_PUMP_PIN, PIN_LOW);
            if (molecular_sieve_loop_timer->parent.flag & RT_TIMER_FLAG_ACTIVATED) {
                // 如果定时器已经启动, 停止定时器
                rt_timer_stop(molecular_sieve_loop_timer);
            }
            rt_thread_mdelay(200); // 延时200秒, 确保分子筛工作
            rt_pin_write(MOLECULAR_SIEVE_1_PIN, PIN_LOW);
            rt_pin_write(MOLECULAR_SIEVE_2_PIN, PIN_LOW);
            if (!(close_oxygen_generation_pump_timer->parent.flag & RT_TIMER_FLAG_ACTIVATED) &&
                rt_pin_read(OXYGEN_GENERATION_PUMP_PIN) == PIN_HIGH) {
                // 如果定时器没有启动, 启动定时器
                rt_timer_start(close_oxygen_generation_pump_timer);
            }
        }
        rt_thread_mdelay(500);
    }
}

/* 分子筛循环 */
static void molecular_sieve_loop(void* parameter) {
    static uint32_t process_count = 0;
    if (process_count++) {
        switch (process_count % 7) {
            case 0:
            case 1:
            case 2:
                rt_pin_write(MOLECULAR_SIEVE_1_PIN, PIN_LOW);
                rt_pin_write(MOLECULAR_SIEVE_2_PIN, PIN_HIGH);
                break;
            case 3:
                rt_pin_write(MOLECULAR_SIEVE_1_PIN, PIN_HIGH);
                rt_pin_write(MOLECULAR_SIEVE_2_PIN, PIN_HIGH);
                break;
            case 4:
            case 5:
            case 6:
                rt_pin_write(MOLECULAR_SIEVE_1_PIN, PIN_HIGH);
                rt_pin_write(MOLECULAR_SIEVE_2_PIN, PIN_LOW);
                break;
        }
    }
}

void close_oxygen_generation_pump_oneshot(void* parameter) {
    rt_pin_write(OXYGEN_GENERATION_PUMP_PIN, PIN_LOW);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 看门狗
#define WDT_DEVICE_NAME "wdt" /* 看门狗设备名称 */

static rt_device_t wdg_dev; /* 看门狗设备句柄 */

static void idle_hook(void) {
    /* 在空闲线程的回调函数里喂狗 */
    rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, NULL);
}

static void wdt_init(void) {
    wdg_dev = rt_device_find(WDT_DEVICE_NAME);
    if (wdg_dev == RT_NULL) {
        LOG_E("find %s failed!", WDT_DEVICE_NAME);
        return;
    }
    rt_device_init(wdg_dev);

    rt_uint32_t timeout = 3; /* 溢出时间，单位：秒 */
    /* 设置超时时间 */
    rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);
    /* 启动看门狗 */
    rt_err_t ret = rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_START, RT_NULL);
    if (ret != RT_EOK) {
        LOG_D("start %s failed!", WDT_DEVICE_NAME);
        return;
    }
    /* 设置空闲线程回调函数 */
    rt_thread_idle_sethook(idle_hook);
}
