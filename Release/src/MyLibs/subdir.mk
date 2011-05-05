################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/MyLibs/AndysOpenCVLib.c 

OBJS += \
./src/MyLibs/AndysOpenCVLib.o 

C_DEPS += \
./src/MyLibs/AndysOpenCVLib.d 


# Each subdirectory must supply rules for building sources it contributes
src/MyLibs/%.o: ../src/MyLibs/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -D_USE_MATH_DEFINES -I/usr/include/opencv -O3 -Wall -c -fmessage-length=0 -std=c99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


