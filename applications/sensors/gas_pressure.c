#include "gas_pressure.h"

#define DBG_TAG "gas"
#define DBG_LVL DBG_LOG

#include <rtdbg.h>
#include <rtdevice.h>
#include <rtthread.h>

static rt_uint8_t gas_pressure_values[20]          = {0};
static rt_uint8_t gas_pressure_index               = 0;
static rt_uint8_t gas_pressure_average_value       = 0;
static rt_bool_t  gas_pressure_average_value_valid = RT_FALSE;

static void gas_pressure_timeout(void* parameter) {
    rt_adc_device_t gas_pressure_dev            = (rt_adc_device_t)parameter;
    rt_uint32_t value                       = rt_adc_read(gas_pressure_dev, GAS_PRESSURE_ADC_CH);
    gas_pressure_values[gas_pressure_index] = (uint8_t)((value * 3.3 / 4095 - 0.5) * 200 / 4);
    gas_pressure_index++;
    if (gas_pressure_index >= sizeof(gas_pressure_values)) {
        gas_pressure_index           = 0;
        rt_uint32_t gas_pressure_sum = 0;
        for (int i = 0; i < sizeof(gas_pressure_values); i++) {
            gas_pressure_sum += gas_pressure_values[i];
        }
        rt_enter_critical();
        gas_pressure_average_value = gas_pressure_sum / sizeof(gas_pressure_values);
        rt_exit_critical();
        gas_pressure_average_value_valid = RT_TRUE;
    }
}

void gas_pressure_init(void) {
    rt_adc_device_t gas_pressure_dev = (rt_adc_device_t)rt_device_find(GAS_PRESSURE_ADC_DEV);
    if (gas_pressure_dev == RT_NULL) {
        LOG_E("find %s failed!", GAS_PRESSURE_ADC_DEV);
        return;
    }
    rt_err_t result = rt_adc_enable(gas_pressure_dev, GAS_PRESSURE_ADC_CH);
    if (result != RT_EOK) {
        LOG_E("enable %s:%d failed!", GAS_PRESSURE_ADC_DEV, GAS_PRESSURE_ADC_CH);
        return;
    }

    rt_timer_t timer = rt_timer_create("gas_pressure",
                                       gas_pressure_timeout,
                                       gas_pressure_dev,
                                       rt_tick_from_millisecond(50),
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

rt_err_t gas_pressure_value_valid(rt_int32_t timeout) {
    rt_tick_t start_tick = rt_tick_get();
    while (gas_pressure_average_value_valid == RT_FALSE) {
        if (rt_tick_get() - start_tick > timeout) {
            return -RT_ETIMEOUT; // 超时
        }
        rt_thread_mdelay(100); // 等待10ms
    }
    return RT_EOK;
}

rt_uint8_t get_gas_pressure_value(void) {
    rt_enter_critical();
    rt_uint8_t value = gas_pressure_average_value;
    rt_exit_critical();
    return value;
}
