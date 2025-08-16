#if defined _START_UP_AUTO_ZERO_EXAMPLE_

/*
___________________________________________________________________________________________________

    AUTO ZERO ON STARTUP EXAMPLE.
___________________________________________________________________________________________________

*/



#include "app.h"
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
    SERIAL DEBUG
=====================================================================
*/
extern UART_HandleTypeDef huart1;
extern "C" {
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
PUTCHAR_PROTOTYPE
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
}


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

    forceSensor.setFilterIntensity(0.0f);

    forceSensor.ready();
    HAL_Delay(1000);

    float noLoadRaw = 0.0f;
    for (uint32_t calibZero = 0; calibZero < 20000; calibZero++)
        noLoadRaw += (float)forceSensor.getRaw();
    noLoadRaw /= 20000.0f;

    forceSensor.setFilterIntensity(0.86f);

    forceSensor.setLinearRegression(0.0f, (uint32_t)noLoadRaw, 50.0f, 805600); // Gain 128

    HAL_Delay(500);
}


/*
=====================================================================
    MAIN LOOP
=====================================================================
*/
void loop()
{
    printf("0\t%d\t%f\n",
            forceSensor.getRaw(),
            forceSensor.getLinearRegression());
}


/*
=====================================================================
    CALLBACKS
=====================================================================
*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_3)
    {
        forceSensor.update(ADS1220flag::onDRDY);
    }
}


#endif /* _START_UP_AUTO_ZERO_EXAMPLE_ */
