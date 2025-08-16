#ifndef _ADS1220_H_
#define _ADS1220_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


// ____ ADS1220 ___________________________________________________________________________________

#include "main.h"

typedef uint8_t ADS1220register;
typedef uint8_t ADS1220byte;

typedef enum
{
    reset       = 0b00000110,
    startSync   = 0b00001000,
    powerdown   = 0b00000010,
    rdata       = 0b00010000
} ADS1220command;

typedef enum
{
    unspecified = 0x00,
    noErrors    = 0x00,
    onDRDY      = 0x0D,
    keepSSLo    = 0x55,
    deviceReady = 0xFF,
} ADS1220flag;

typedef enum
{
    gain001 = 0b000,
    gain002 = 0b001,
    gain004 = 0b010,
    gain008 = 0b011,
    gain016 = 0b100,
    gain032 = 0b101,
    gain064 = 0b110,
    gain128 = 0b111
} ADS1220gain;

class ADS1220
{
public:
    /* Find the datasheet here: https://www.ti.com/lit/ds/symlink/ads1220.pdf */
    ADS1220(SPI_HandleTypeDef *spiHandler);

public:
    ADS1220register configReg0; ADS1220register configReg1;
    ADS1220register configReg2; ADS1220register configReg3;

private:
    SPI_HandleTypeDef *_spiHandler;

    ADS1220byte _isReady;

    uint32_t _raw; float _filteredRaw; int32_t _offsetRaw = 0;
    float _filterAlpha = 0.0f; float _filterAlphaCompl = 1.0f;
    float _regressionCoeffA = 0.0f; float _regressionCoeffB = 0.0f;
    float _regressionResult = 0.0f;

    void (*_hwCtrSSLo)(void);
    void (*_hwCtrSSHi)(void);

public:
    /* Append custom functions for pulling CS pin low and high, respectively */
    void appendHardwareControlSS(void (*hwCtrSSLo)(void), void (*hwCtrSSHi)(void));
    /*
    Write configurations to ADS1220 registers.
    If unwritten, default configuration is written.
    CS pin is pulled high after completion.
    @return 0 if no errors, else 1, 2, 3, or 4 for error writing to register 0, 1, 2, or 3, respectively.
    */
    ADS1220byte init();
    /*
    Set ADC gain and write to registers. CS pin is pulled high after completion.
    @param gain Gain value to be set. Only valid if using ADS1220gain::gainxxx.
    @return 0 if no errors, 1 if otherwise.
    */
    ADS1220byte setGain(ADS1220gain gain);
    /*
    Extract gain value from register 0 value.
    It is recommended that updateRegisters() is called to obtain the latest values.
    @return Gain level from last register 0 value update.
    */
    ADS1220byte getGain();
    /*
    Set intensity for the built-in Low-pass Filter, ranging from 0.0 to 1.0.
    @param intensity Filter intensity from 0.0 to 1.0.
    */
    void setFilterIntensity(float intensity);
    /*
    Call the appended custom CS low function and inject delaying interval.
    Nothing done if no HW Control functions are appended.
    */
    void slaveSelect();
    /*
    Call the appended custom CS high function and inject delaying interval.
    Nothing done if no HW Control functions are appended.
    */
    void slaveRelease();
    /*
    Send commands to ADS1220:
    reset // startSync // powerdown // rdata
    CS pin is NOT actively driven.
    @param command Command to be sent.
    */
    void command(ADS1220byte command);
    /* Write, then read for confirmation all registers. CS pin is NOT actively driven. */
    void updateRegisters();
    /*
    Set ADS1220 as ready for continuous operation, else update() does nothing.
    CS pin is pulled low, start/sync command is sent.
    */
    void ready();
    /* Stop continuous operation. */
    /*
    Stop continuous operation, update() does nothing.
    CS pin is pulled high.
    */
    void halt();
    /*
    Read raw conversion data. The method ready() must first be called.
    @param DRDYdependence If called in DRDY falling event ISR, set as "onDRDY" to ignore sending "rdata" command.
    @param pullSSHi If set as "keepSSLo", CS pin is kept low, assuming custom function is appended.
    */
    void update(ADS1220byte DRDYdependence = ADS1220flag::unspecified,
                ADS1220byte pullSSHi = ADS1220flag::unspecified);
    /*
    Obtain raw data.
    Does not affect the operation of the device.
    */
    uint32_t getRaw();
    /*
    Two-point linear interpolation to convert raw value to a different scale.
    Does not affect the operation of the device.
    @param value1 Target regression value of point 1, typically taken at no-load or 0 Newton.
    @param raw1 Raw value corresponding to target regression value 1.
    @param value2 Target regression value of point 2.
    @param raw2 Raw value corresponding to target regression value 2.
    */
    void setLinearRegression(float value1, uint32_t raw1, float value2, uint32_t raw2);
    /*
    Compute the linear regression value.
    Does not affect the operation of the device.
    */
    float getLinearRegression();

private:
    /* Full duplex SPI transaction of 1 byte. */
    void transceive8(ADS1220byte sendBuffer, ADS1220byte *receiveBuffer);
};

// ____ ADS1220 ___________________________________________________________________________________


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ADS1220_H_ */
