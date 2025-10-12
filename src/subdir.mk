e_SRCS += \
./src/main.cpp \
./src/motor_controller.cpp \
./src/motor_settings.cpp \
./src/gpio.cpp \
./src/serial.cpp

C_DEPS += \
./src/main.d \
./src/motor_controller.d \
./src/motor_settings.d \
./src/gpio.d \
./src/serial.d

OBJS += \
./src/main.o \
./src/motor_controller.o \
./src/motor_settings.o \
./src/gpio.o \
./src/serial.o


src/%.o: ./src/%.cpp src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GNU Arm Cross C Compiler'
	arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -ffreestanding -fno-move-loop-invariants -g3 -DDEBUG -DUSE_FULL_ASSERT -DTRACE -DOS_USE_TRACE_SEMIHOSTING_STDOUT -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include" -I"../system/include" -I"../system/include/cmsis" -I"../system/include/stm32f4-hal" -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
