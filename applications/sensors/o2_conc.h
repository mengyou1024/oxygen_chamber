#pragma once

// 氧浓度检测板

/*
 * Description     :处理HCOALH20氧气传感器模块数据
 * Uart Bauds	   :9600		8位数据+1停止位			无奇偶校验位
 * Data Type	   :起始符			长度			命令符			数据1	......	数据N			校验和
 *                  Header			Len				CMD			  Data1			 DataN			 CS
 *                  0x16			0x09			0x01		  0x??			 0x??			0x??
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "sensors_config.h"
#include <rtdef.h>

void o2_conc_init(void);

typedef struct {
    rt_uint16_t o2_concentration; // 氧浓度值
    rt_uint8_t  o2_flow;          // 氧流量值
    rt_uint16_t temperature;      // 温度值
} o2_conc_struct, *o2_conc_struct_t;

/**
 * @brief 等待氧浓度数据有效
 * @param timeout 超时时间ms
 * @return RT_OK 成功, -RT_ETIMEOUT 超时
 */
rt_err_t o2_conc_value_valid(uint32_t timeout);

/**
 * @brief 获取氧浓度数据
 * @return 氧浓度数据, 如果值无效则会阻塞等待
 */
o2_conc_struct get_o2_conc_value(void);

#ifdef __cplusplus
}
#endif
