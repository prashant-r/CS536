################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../audiolisten.c \
../audiostream.c \
../fileserver.c 

O_SRCS += \
../audiolisten.o \
../audiostream.o 

OBJS += \
./audiolisten.o \
./audiostream.o \
./fileserver.o 

C_DEPS += \
./audiolisten.d \
./audiostream.d \
./fileserver.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


