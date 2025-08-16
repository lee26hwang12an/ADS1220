#include "ADS1220.h"
#include "math.h"


/*
=====================================================================
    PRIVATE FUNCTIONS
=====================================================================
*/

#define __delayms(ms) HAL_Delay((ms))
#define __abs(val) (((val) >= 0) ? (val) : (0 - (val)))


/*
=====================================================================
    CONSTRUCTORS
=====================================================================
*/

ADS1220::ADS1220(SPI_HandleTypeDef *spiHandler)
    : _spiHandler(spiHandler)
{
    configReg0 = ADS1220flag::unspecified;
    configReg1 = ADS1220flag::unspecified;
    configReg2 = ADS1220flag::unspecified;
    configReg3 = ADS1220flag::unspecified;

    _hwCtrSSLo = nullptr;
    _hwCtrSSHi = nullptr;

    _isReady = ADS1220flag::unspecified;
}


/*
=====================================================================
    METHODS
=====================================================================
*/

void ADS1220::appendHardwareControlSS(void (*hwCtrSSLo)(void), void (*hwCtrSSHi)(void))
{
    if (hwCtrSSLo != nullptr) _hwCtrSSLo = hwCtrSSLo;
    if (hwCtrSSHi != nullptr) _hwCtrSSHi = hwCtrSSHi;
}

ADS1220byte ADS1220::init()
{
    __delayms(100);

    ADS1220::slaveSelect();
    ADS1220::command(ADS1220command::reset);

    __delayms(10);

    // _____________________________________ CONFIGURATION REGISTER 0
    if (ADS1220::configReg0 == ADS1220flag::unspecified)
        ADS1220::configReg0 =
                0b0000  << 4    | // Input multiplexer: 0000 = AIN0 & AIN1;
                0b111   << 1    | // Gain selection: 111 = 128;
                0b0     << 0;     // PGA bypass: enabled = 0 | 1 = disabled & bypassed.

    // _____________________________________ CONFIGURATION REGISTER 1
    if (ADS1220::configReg1 == ADS1220flag::unspecified)
        ADS1220::configReg1 =
                0b110   << 5    | // Data rate: 110 = (normal)1kSPS (duty)250SPS (turbo)2kSPS;
                0b10    << 3    | // Mode selection: normal | duty | turbo | reserved;
                0b1     << 2    | // Conversion mode: Single-shot = 0 | 1 = Continuous;
                0b0     << 1    | // Temperature mode: disabled = 0 | 1 = enabled;
                0b0     << 0;     // Burn-out CS: 0 = disabled | 1 = enabled.

    // _____________________________________ CONFIGURATION REGISTER 2
    if (ADS1220::configReg2 == ADS1220flag::unspecified)
        ADS1220::configReg2 =
                0b10    << 6    | // VREF select: 10 = AIN0 & AIN3;
                0b00    << 4    | // FIR config, set 0 for all except (normal)20SPS & (duty)5SPS;
                0b1     << 3    | // Lowside pwr sw: 0 = open | 1 = closes automatically;
                0b000   << 0;     // IDAC: 000 = off.

    // _____________________________________ CONFIGURATION REGISTER 3
    if (ADS1220::configReg3 == ADS1220flag::unspecified)
        ADS1220::configReg3 =
                0b000   << 5    | // IDAC1 routing: 000 = disabled;
                0b000   << 2    | // IDAC2 routing: 000 = disabled;
                0b0     << 1    | // DRDY mode: only nDRDY pin = 0 | 1 = both nDRDY & DOUT pins;
                0b0     << 0;       // Reserved, always 0.

    ADS1220register sanityCheck0 = ADS1220::configReg0;
    ADS1220register sanityCheck1 = ADS1220::configReg1;
    ADS1220register sanityCheck2 = ADS1220::configReg2;
    ADS1220register sanityCheck3 = ADS1220::configReg3;
    ADS1220byte errorsCount = ADS1220flag::noErrors;

    ADS1220::updateRegisters();
    ADS1220::slaveRelease();

    __delayms(10);

    if (sanityCheck0 != ADS1220::configReg0) return errorsCount = 1;
    if (sanityCheck1 != ADS1220::configReg1) return errorsCount = 2;
    if (sanityCheck2 != ADS1220::configReg2) return errorsCount = 3;
    if (sanityCheck3 != ADS1220::configReg3) return errorsCount = 4;
    return errorsCount;
}

ADS1220byte ADS1220::setGain(ADS1220gain gain)
{
    ADS1220register CR0save = ADS1220::configReg0;
    ADS1220::configReg0 = ADS1220::configReg0 & (~(0b111 << 1));
    ADS1220::configReg0 = ADS1220::configReg0 | (gain << 1);

    ADS1220::slaveSelect();
    ADS1220::updateRegisters();
    ADS1220::slaveRelease();

    if (CR0save == ADS1220::configReg0) return ADS1220flag::noErrors;
    return 1;
}

ADS1220byte ADS1220::getGain()
{
    ADS1220byte gain = (ADS1220::configReg0 >> 1) & 0b00000111;
    gain = pow(2, gain);
    return gain;
}

void ADS1220::setFilterIntensity(float intensity)
{
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity >= 1.0f) intensity = 0.999999f;

    _filterAlpha = intensity;
    _filterAlphaCompl = 1.0f - intensity;
}

void ADS1220::slaveSelect()
{
    if (ADS1220::_hwCtrSSLo == nullptr) return;

    ADS1220::_hwCtrSSLo();
    for (ADS1220byte wait = 0; wait < 100; wait++);
}

void ADS1220::slaveRelease()
{
    if (ADS1220::_hwCtrSSHi == nullptr) return;

    for (ADS1220byte wait = 0; wait < 100; wait++);
    ADS1220::_hwCtrSSHi();
}

void ADS1220::command(ADS1220byte command)
{ ADS1220::transceive8(command, nullptr); }

void ADS1220::updateRegisters()
{
    uint32_t rawSave = _raw;

    ADS1220::transceive8(0b01000011, (uint8_t *)&_raw);
    ADS1220::transceive8(ADS1220::configReg0, (uint8_t *)&_raw);
    ADS1220::transceive8(ADS1220::configReg1, (uint8_t *)&_raw);
    ADS1220::transceive8(ADS1220::configReg2, (uint8_t *)&_raw);
    ADS1220::transceive8(ADS1220::configReg3, (uint8_t *)&_raw);

    for (ADS1220byte wait = 0; wait < 100; wait++);

    ADS1220::transceive8(0b00100011, (uint8_t *)&_raw);
    ADS1220::transceive8(0xFF, &configReg0);
    ADS1220::transceive8(0xFF, &configReg1);
    ADS1220::transceive8(0xFF, &configReg2);
    ADS1220::transceive8(0xFF, &configReg3);

    _raw = rawSave;
}

void ADS1220::ready()
{
    ADS1220::slaveSelect();
    ADS1220::command(ADS1220command::startSync);
    _isReady = ADS1220flag::deviceReady;
}

void ADS1220::halt()
{
    _isReady = ADS1220flag::unspecified;
    ADS1220::slaveRelease();
}

void ADS1220::update(ADS1220byte DRDYdependence, ADS1220byte pullSSHi)
{
    if (_isReady != ADS1220flag::deviceReady) return;

    ADS1220byte buffer[3] = {0x00, 0x00, 0x00};

    ADS1220::slaveSelect();

    if (DRDYdependence != ADS1220flag::onDRDY)
        ADS1220::command(ADS1220command::rdata);

    ADS1220::transceive8(0xFF, (uint8_t *)(buffer+0));
    ADS1220::transceive8(0xFF, (uint8_t *)(buffer+1));
    ADS1220::transceive8(0xFF, (uint8_t *)(buffer+2));
    _raw = (buffer[0] << 16) | (buffer[1] << 8) | (buffer[2] << 0);

    // _______________________________ FILTER RAW & LINEAR REGRESSION
    if (_filterAlpha <= 0.0f) _filteredRaw = _raw;
    else _filteredRaw = _filterAlpha * _filteredRaw + _filterAlphaCompl * (float)_raw;

    _regressionResult = _regressionCoeffA * _filteredRaw + _regressionCoeffB;

    if (pullSSHi == ADS1220flag::keepSSLo)
        return;

    ADS1220::slaveRelease();
}

uint32_t ADS1220::getRaw()
{ return _filteredRaw; }

void ADS1220::setLinearRegression(float value1, uint32_t raw1, float value2, uint32_t raw2)
{
    _regressionCoeffA = (value2 - value1) / ((float)raw2 - (float)raw1);
    _regressionCoeffB = value1 - _regressionCoeffA * (float)raw1;
}

float ADS1220::getLinearRegression()
{ return _regressionResult; }

void ADS1220::transceive8(ADS1220byte sendBuffer, ADS1220byte *receiveBuffer)
{
    HAL_SPI_TransmitReceive(
            _spiHandler,
            (uint8_t *)&sendBuffer,
            (uint8_t *)receiveBuffer, 1, 100);
}
