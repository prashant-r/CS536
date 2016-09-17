################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../myping.c \
../mypingd.c \
../tcpclient.c \
../tcpserver.c 

OBJS += \
./myping.o \
./mypingd.o \
./tcpclient.o \
./tcpserver.o 

C_DEPS += \
./myping.d \
./mypingd.d \
./tcpclient.d \
./tcpserver.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


