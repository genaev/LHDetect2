################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/lhdetect2.c \
../src/lhdetect2_main.c \
../src/tools.c 

OBJS += \
./src/lhdetect2.o \
./src/lhdetect2_main.o \
./src/tools.o 

C_DEPS += \
./src/lhdetect2.d \
./src/lhdetect2_main.d \
./src/tools.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -D_USE_MATH_DEFINES -I/usr/include/opencv -O0 -g3 -Wall -c -fmessage-length=0 -std=c99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


