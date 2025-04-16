#pragma once

#include <drv_gpio.h>

// 压缩机继电器1
#define COMPRESSOR_RELAY_1_PIN GET_PIN(E, 4)
// 压缩机继电器2
#define COMPRESSOR_RELAY_2_PIN GET_PIN(E, 3)
// 消毒器继电器
#define LITTLE_FAN_PIN         GET_PIN(E, 2)

#define COUNTROLER_OUT_1       GET_PIN(A, 4)
#define COUNTROLER_OUT_2       GET_PIN(A, 6)
#define COUNTROLER_OUT_3       GET_PIN(C, 4)
#define COUNTROLER_OUT_4       GET_PIN(B, 0)
#define COUNTROLER_OUT_5       GET_PIN(A, 5)
#define COUNTROLER_OUT_6       GET_PIN(A, 7)
#define COUNTROLER_OUT_7       GET_PIN(C, 5)
#define COUNTROLER_OUT_8       GET_PIN(B, 1)