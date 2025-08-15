#pragma once

#include <drv_gpio.h>

#define RELAY_1_PIN                GET_PIN(E, 4)
#define RELAY_2_PIN                GET_PIN(E, 3)
#define RELAY_3_PIN                GET_PIN(E, 2)

#define COUNTROLER_OUT_1_PIN       GET_PIN(A, 4)
#define COUNTROLER_OUT_2_PIN       GET_PIN(A, 5)
#define COUNTROLER_OUT_3_PIN       GET_PIN(A, 6)
#define COUNTROLER_OUT_4_PIN       GET_PIN(A, 7)
#define COUNTROLER_OUT_5_PIN       GET_PIN(C, 4)
#define COUNTROLER_OUT_6_PIN       GET_PIN(C, 5)
#define COUNTROLER_OUT_7_PIN       GET_PIN(B, 0)
#define COUNTROLER_OUT_8_PIN       GET_PIN(B, 1)

#define TEMPERATURE_SENSOR_1_PIN   GET_PIN(C, 2)
#define TEMPERATURE_SENSOR_2_PIN   GET_PIN(C, 3)

////////////////////////////////////////////////////////////////////////
// redirect

// 制氧泵
#define OXYGEN_GENERATION_PUMP_PIN RELAY_1_PIN
// 升压泵
#define BOOST_PUMP_PIN             RELAY_2_PIN
// 负氧泵
#define NEGATIVE_OXYGEN_PUMP_PIN   RELAY_3_PIN

// 报警
#define ALARM_PIN                  COUNTROLER_OUT_5_PIN
// 1#灯
#define LAMP_1_PIN                 COUNTROLER_OUT_6_PIN
// 2#灯
#define LAMP_2_PIN                 COUNTROLER_OUT_7_PIN
// 阀1
#define SOLENOID_VALVE_1_PIN       COUNTROLER_OUT_1_PIN
// 阀4
#define SOLENOID_VALVE_4_PIN       COUNTROLER_OUT_4_PIN

// 分子筛
#define MOLECULAR_SIEVE_1_PIN      COUNTROLER_OUT_2_PIN
#define MOLECULAR_SIEVE_2_PIN      COUNTROLER_OUT_3_PIN
