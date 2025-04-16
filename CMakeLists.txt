CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_PROCESSOR cortex-m3)
#SET(CMAKE_VERBOSE_MAKEFILE ON)

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

SET(CMAKE_C_COMPILER "D:/SoftWares/env-windows/tools/bin/../../tools/gnu_gcc/arm_gcc/mingw/bin/arm-none-eabi-gcc.exe")
SET(CMAKE_ASM_COMPILER "D:/SoftWares/env-windows/tools/bin/../../tools/gnu_gcc/arm_gcc/mingw/bin/arm-none-eabi-gcc.exe")
SET(CMAKE_C_FLAGS " -mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections -Dgcc -O0 -gdwarf-2 -g")
SET(CMAKE_ASM_FLAGS " -c -mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections -x assembler-with-cpp -Wa,-mimplicit-it=thumb  -gdwarf-2")
SET(CMAKE_C_COMPILER_WORKS TRUE)

SET(CMAKE_CXX_COMPILER "D:/SoftWares/env-windows/tools/bin/../../tools/gnu_gcc/arm_gcc/mingw/bin/arm-none-eabi-g++.exe")
SET(CMAKE_CXX_FLAGS " -mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections -Dgcc -O0 -gdwarf-2 -g")
SET(CMAKE_CXX_COMPILER_WORKS TRUE)

SET(CMAKE_OBJCOPY "D:/SoftWares/env-windows/tools/bin/../../tools/gnu_gcc/arm_gcc/mingw/bin/arm-none-eabi-objcopy.exe")
SET(CMAKE_SIZE "D:/SoftWares/env-windows/tools/bin/../../tools/gnu_gcc/arm_gcc/mingw/bin/arm-none-eabi-size.exe")

SET(CMAKE_EXE_LINKER_FLAGS " -mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections -Wl,--gc-sections,-Map=rtthread.map,-cref,-u,Reset_Handler -T ${CMAKE_SOURCE_DIR}/board/linker_scripts/link.ld")

SET(CMAKE_CXX_STANDARD 14)
PROJECT(rtthread C CXX ASM)
INCLUDE_DIRECTORIES(
	applications
	.
	rt-thread/components/libc/compilers/common/include
	rt-thread/components/libc/compilers/newlib
	rt-thread/libcpu/arm/common
	rt-thread/libcpu/arm/cortex-m3
	rt-thread/components/drivers/include
	packages/dhtxx-latest/inc
	board
	libraries/gd32_drivers
	rt-thread/components/finsh
	rt-thread/include
	libraries/GD32F20x_Firmware_Library/CMSIS/GD/GD32F20x/Include
	libraries/GD32F20x_Firmware_Library/CMSIS
	libraries/GD32F20x_Firmware_Library/GD32F20x_standard_peripheral/Include
	rt-thread/components/libc/posix/io/poll
	rt-thread/components/libc/posix/io/stdio
	rt-thread/components/libc/posix/ipc
)

ADD_DEFINITIONS(
	-DGD32F20X_CL
	-DHAVE_CCONFIG_H
	-DRT_USING_LIBC
	-DRT_USING_NEWLIB
	-DUSE_STDPERIPH_DRIVER
	-D_POSIX_C_SOURCE=1
	-D__RTTHREAD__
)

SET(PROJECT_SOURCES
	applications/sensors/o2_conc.c
	applications/main.c
	applications/sensors/lcd_7_inch.c
	applications/sensors/sco3_o2.c
	applications/sensors/lcd_3_5_inch.c
	applications/sensors/nai_conc.c
	rt-thread/components/libc/compilers/common/cctype.c
	rt-thread/components/libc/compilers/common/cstdio.c
	rt-thread/components/libc/compilers/common/cstdlib.c
	rt-thread/components/libc/compilers/common/cstring.c
	rt-thread/components/libc/compilers/common/ctime.c
	rt-thread/components/libc/compilers/common/cwchar.c
	rt-thread/components/libc/compilers/newlib/syscalls.c
	rt-thread/libcpu/arm/common/backtrace.c
	rt-thread/libcpu/arm/common/div0.c
	rt-thread/libcpu/arm/common/showmem.c
	rt-thread/libcpu/arm/cortex-m3/context_gcc.S
	rt-thread/libcpu/arm/cortex-m3/cpuport.c
	rt-thread/components/drivers/ipc/completion.c
	rt-thread/components/drivers/ipc/dataqueue.c
	rt-thread/components/drivers/ipc/pipe.c
	rt-thread/components/drivers/ipc/ringblk_buf.c
	rt-thread/components/drivers/ipc/ringbuffer.c
	rt-thread/components/drivers/ipc/waitqueue.c
	rt-thread/components/drivers/ipc/workqueue.c
	rt-thread/components/drivers/misc/adc.c
	rt-thread/components/drivers/misc/pin.c
	rt-thread/components/drivers/serial/serial.c
	packages/dhtxx-latest/src/dhtxx.c
	board/board.c
	libraries/GD32F20x_Firmware_Library/CMSIS/GD/GD32F20x/Source/GCC/startup_gd32f20x_cl.s
	libraries/gd32_drivers/drv_adc.c
	libraries/gd32_drivers/drv_gpio.c
	libraries/gd32_drivers/drv_usart.c
	rt-thread/components/finsh/msh.c
	rt-thread/components/finsh/shell.c
	rt-thread/components/finsh/msh_parse.c
	rt-thread/components/finsh/cmd.c
	rt-thread/src/clock.c
	rt-thread/src/components.c
	rt-thread/src/device.c
	rt-thread/src/idle.c
	rt-thread/src/ipc.c
	rt-thread/src/irq.c
	rt-thread/src/kservice.c
	rt-thread/src/mem.c
	rt-thread/src/mempool.c
	rt-thread/src/object.c
	rt-thread/src/scheduler.c
	rt-thread/src/thread.c
	rt-thread/src/timer.c
	libraries/GD32F20x_Firmware_Library/GD32F20x_standard_peripheral/Source/gd32f20x_gpio.c
	libraries/GD32F20x_Firmware_Library/GD32F20x_standard_peripheral/Source/gd32f20x_misc.c
	libraries/GD32F20x_Firmware_Library/GD32F20x_standard_peripheral/Source/gd32f20x_exti.c
	libraries/GD32F20x_Firmware_Library/GD32F20x_standard_peripheral/Source/gd32f20x_adc.c
	libraries/GD32F20x_Firmware_Library/CMSIS/GD/GD32F20x/Source/system_gd32f20x.c
	libraries/GD32F20x_Firmware_Library/GD32F20x_standard_peripheral/Source/gd32f20x_usart.c
	libraries/GD32F20x_Firmware_Library/GD32F20x_standard_peripheral/Source/gd32f20x_rcu.c
)

LINK_DIRECTORIES(
)

LINK_LIBRARIES(
	c
	m
)

ADD_EXECUTABLE(${CMAKE_PROJECT_NAME}.elf ${PROJECT_SOURCES})
ADD_CUSTOM_COMMAND(TARGET ${CMAKE_PROJECT_NAME}.elf POST_BUILD 
COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_PROJECT_NAME}.elf ${CMAKE_PROJECT_NAME}.bin COMMAND ${CMAKE_SIZE} ${CMAKE_PROJECT_NAME}.elf)