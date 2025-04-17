#pragma once

#include <drv_gpio.h>

// 3.5寸串口屏
#define LCD_3_5_INCH_UART    "uart4"
// 7寸串口屏
#define LCD_7_INCH_UART      "uart7"
// 负氧离子浓度传感器
#define NAI_CONC_UART        "uart5"
// 氧气浓度传感器
#define O2_CONC_UART         "uart3"
// SC03-O2 化学氧浓度、流量、传感器
#define SCO3_O2_UART         "uart0"

// 气体压力传感器ADC设备名
#define GAS_PRESSURE_ADC_DEV "adc0"
// 气体压力传感器ADC通道号
#define GAS_PRESSURE_ADC_CH  (11)

// DHT11
#define DHT11_PIN            GET_PIN(C, 2)

inline static uint16_t swap_u16(uint16_t val) {
    return (val >> 8) | (val << 8);
}
