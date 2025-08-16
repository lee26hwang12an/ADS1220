# Texas Instruments' ADS1220 24-bit ADC

Standalone Library for TI's ADS1220.


#
### Instancing Class Object
Class ADS1220 requires a hardware SPI handler. The SPI handler is passed into the class' constructor by reference:

```cpp
SPI_HandleTypeDef hspi1;
ADS1220 forceSensor = ADS1220(&hspi1);
```

#
### Appending Hardware Control for Slave-Select Pin
The library offers appending custom functions for driving the SS (or CS) pin for the SPI protocol. By default, the SS pin is NOT actively driven by class ADS1220's methods. To have the SS pin driven by the library, declare and define 2 functions driving the SS pin LOW and HIGH, then pass them into the class' method `appendHardwareControlSS` as function pointers in the respective order:

```cpp
SPI_HandleTypeDef hspi1;
ADS1220 forceSensor = ADS1220(&hspi1);

// In this example, pin PA4 of the STM32F103 was used:
void SSLo() { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); }
void SSHi() { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); }

void setup()
{
    forceSensor.appendHardwareControlSS(&SSLo, &SSHi);

    // ...
}

void loop()
{
    // ...
}
```


#
### Writing to Configuration Registers
The ADS1220 offers 04 configuration registers, the detailed description of which can be found in TI's datasheet. These registers can be found as public attributes of class ADS1220. Modify each register by assigning values directly to the respective attribute and calling the method `updateRegisters()` to write and then read:
```cpp
void setup()
{
    forceSensor.appendHardwareControlSS(&SSLo, &SSHi);
    while (forceSensor.init() != ADS1220flag::noErrors);

    forceSensor.configReg0 = 0x08;
    forceSensor.configReg1 = 0x04;
    forceSensor.configReg0 = 0x10;
    forceSensor.configReg0 = 0x00;

    // This writes to the ADS1220's registers, then read as confirmation
    forceSensor.updateRegisters();

    // ...
}
```
Or simply modify the attributes before calling the method `init()`. If any registers left at reset value `0x00`, the library will write its own prefered configuration:
```cpp
void setup()
{
    forceSensor.appendHardwareControlSS(&SSLo, &SSHi);

    forceSensor.configReg0 = 0x08;
    forceSensor.configReg1 = 0x04;
    forceSensor.configReg0 = 0x10;
    forceSensor.configReg0 = 0x00;

    // The above configurations, if different from 0x00,  will be written to the ADS1220
    while (forceSensor.init() != ADS1220flag::noErrors);

    // ...
}
```


#
### Using the Built-in Digital Low-pass Filter
The library offers a built-in Low-pass Filter, whose intensity can be adjusted from `0.0` to `0.999999`. By default, the intensity is set to `0.0`. Leaving the intensity at 0.0 or less than this value will ***deactivate*** the filter. Any value `>= 1.0` set will be ***constrained*** back to `0.999999`:
```cpp
void setup()
{
    forceSensor.appendHardwareControlSS(&SSLo, &SSHi);

    while (forceSensor.init() != ADS1220flag::noErrors);

    // Leave as 0.0 to skip the entire filtering calculations
    forceSensor.setFilterIntensity(0.75f);

    // ...
}
```


#
### Start and Stop Device Operation
Regardless of mode of operation (`continuous` or `single-shot`) selected in the configuration registers, the method `ready()` ***MUST*** be called before any sampling can occur. To stop the sampling operation, call `halt()`.
```cpp
void setup()
{
    // ...

    // After this, calling forceSensor.update() gives valid results:
    forceSensor.ready();

    // After this, calling forceSensor.update() does nothing:
    forceSensor.halt();

    // ...
}
```
Once in ***ready state***, calling `update()` will start the sampling and linear conversion processes. If update occurs in the `falling edge ISR` of the DRDY pin, raise the `onDRDY` flag by passing `ADS1220flag::onDRDY`, otherwise, ***don't specify*** or pass `ADS1220flag::unspecified`
```cpp
// Calling update() in DRDY falling edge ISR:
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_3)
    {
        forceSensor.update(ADS1220flag::onDRDY);
    }
}


// Calling update() by sending RDATA conmmand (see datasheet):
void loop()
{
    forceSensor.update();

    // OR

    forceSensor.update(ADS1220flag::unspecified);
}
```
Additionally, the method `update()` also accepts the flag `ADS1220flag::keepSSLo` to leave SS pin low after data acquisiton SPI transactions. Leave unspecified or pass `ADS1220flag::unspecified` to pull SS pin high after every update iteration:
```cpp
// Calling update() in DRDY falling edge ISR:
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_3)
    {
        // SS pin is pulled HIGH after every update:
        forceSensor.update(ADS1220flag::onDRDY);
        // OR
        forceSensor.update(ADS1220flag::onDRDY, ADS1220flag::unspecified);

        // SS pin is kept LOW after every update:
        forceSensor.update(ADS1220flag::onDRDY, ADS1220flag::keepSSLo);
    }
}
```


#
### Obtain the Raw ADC Data and Unit Conversion via Linear Interpolation
The library offers converting raw value to a different unit using the following linear assumption: `value = A * rawValue + B`. First, the linear relationship must be set up using 2-point linear interpolation by calling `setLinearRegression()`. The example below converts raw ADC value to ***Newton***, with `0.0N ~ 299545` and `50.0N ~ 805617`:
```cpp
void setup()
{
    // ...

    // Two-point linear interpolation: val1 | raw @val1 | val2 | raw @val2 |
    forceSensor.setLinearRegression(0.0f, 299545, 50.0f, 805617);

    // ...
}
```
To obtain raw ADC value stored in the data register of the ADS1220, call `getRaw()`. To obtain the unit converted result, call `getLinearRegression()`:
```cpp
void loop()
{
    printf("0\t%d\t%f\n",
        forceSensor.getRaw(),
        forceSensor.getLinearRegression());
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{ if (GPIO_Pin == GPIO_PIN_3) forceSensor.update(ADS1220flag::onDRDY); }
```


#
### Example Program
```cpp
#include "ADS1220/ADS1220.h"


/*
=====================================================================
    COMPONENTS
=====================================================================
*/
extern SPI_HandleTypeDef hspi1;
ADS1220 forceSensor = ADS1220(&hspi1);
void SSLo() { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); }
void SSHi() { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); }


/*
=====================================================================
    SETUP
=====================================================================
*/
void setup()
{
    HAL_Delay(500);

    forceSensor.appendHardwareControlSS(&SSLo, &SSHi);
    while (forceSensor.init() != ADS1220flag::noErrors);
    while (forceSensor.setGain(ADS1220gain::gain128) != ADS1220flag::noErrors);

    forceSensor.setFilterIntensity(0.86f);
    forceSensor.setLinearRegression(0.0f, 299545, 50.0f, 805617);

    forceSensor.ready();
    HAL_Delay(500);
}


/*
=====================================================================
    MAIN LOOP
=====================================================================
*/
void loop()
{
    printf("0\tRaw: %d\tNewton: %f\n",
        forceSensor.getRaw(),
        forceSensor.getLinearRegression());
}


/*
=====================================================================
    DRDY FALLING EDGE ISR
=====================================================================
*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{ if (GPIO_Pin == GPIO_PIN_3) forceSensor.update(ADS1220flag::onDRDY, ADS1220flag::keepSSLo); }

```
