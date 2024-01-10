# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Parser.cpp \
../src/Arc.cpp \
../src/DFG.cpp \
../src/main.cpp \
../src/Node.cpp \
../src/Falcon.cpp

OBJS += \
../src/Parser.o \
../src/Arc.o \
../src/DFG.o \
../src/main.o \
../src/Node.o \
../src/Falcon.o


CPP_DEPS += \
../src/Parser.d \
../src/Arc.d \
../src/DFG.d \
../src/main.d \
../src/Node.d \
../src/Falcon.d


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++11 -g -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


