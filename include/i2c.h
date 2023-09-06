/*
Functions taken from Switch-OC-Suite source code made by KazushiMe
Original repository link (Deleted, last checked 15.04.2023): https://github.com/KazushiMe/Switch-OC-Suite
*/

#include "max17050.h"

constexpr float max17050SenseResistor = MAX17050_BOARD_SNS_RESISTOR_UOHM / 1000; // in uOhm
constexpr float max17050CGain = 1.99993;
const u32 MA_RANGE_MIN = 512;
const u32 MA_RANGE_MAX = 4544;

const u8 BQ24193_CHARGE_CURRENT_CONTROL_REG = 0x2;

Result I2cReadRegHandler(u8 reg, I2cDevice dev, u16 *out)
{
	// I2C Bus Communication Reference: https://www.ti.com/lit/an/slva704/slva704.pdf
	struct { u8 reg;  } __attribute__((packed)) cmd;
	struct { u16 val; } __attribute__((packed)) rec;

	I2cSession _session;

	Result res = i2cOpenSession(&_session, dev);
	if (res)
		return res;

	cmd.reg = reg;
	res = i2csessionSendAuto(&_session, &cmd, sizeof(cmd), I2cTransactionOption_All);
	if (res)
	{
		i2csessionClose(&_session);
		return res;
	}

	res = i2csessionReceiveAuto(&_session, &rec, sizeof(rec), I2cTransactionOption_All);
	if (res)
	{
		i2csessionClose(&_session);
		return res;
	}

	*out = rec.val;
	i2csessionClose(&_session);
	return 0;
}

bool Max17050ReadReg(u8 reg, u16 *out)
{
	u16 data = 0;
	Result res = I2cReadRegHandler(reg, I2cDevice_Max17050, &data);

	if (res)
	{
		*out = res;
		return false;
	}

	*out = data;
	return true;
}

u8 I2c_Bq24193_Convert_mA_Raw(u32 ma) {
    // Adjustment is required
    u8 raw = 0;

    if (ma > MA_RANGE_MAX) // capping
        ma = MA_RANGE_MAX;

    bool pct20 = ma <= (MA_RANGE_MIN - 64);
    if (pct20) {
        ma = ma * 5;
        raw |= 0x1;
    }

    ma -= ma % 100; // round to 100
    ma -= (MA_RANGE_MIN - 64); // ceiling
    raw |= (ma >> 6) << 2;

    return raw;
};

u32 I2c_Bq24193_Convert_Raw_mA(u8 raw) {
    // No adjustment is allowed
    u32 ma = (((raw >> 2)) << 6) + MA_RANGE_MIN;

    bool pct20 = raw & 1;
    if (pct20)
        ma = ma * 20 / 100;

    return ma;
};

Result I2cSet_U8(I2cDevice dev, u8 reg, u8 val) {
    // ams::fatal::srv::StopSoundTask::StopSound()
    // I2C Bus Communication Reference: https://www.ti.com/lit/an/slva704/slva704.pdf
    struct {
        u8 reg;
        u8 val;
    } __attribute__((packed)) cmd;

    I2cSession _session;
    Result res = i2cOpenSession(&_session, dev);
    if (res)
        return res;

    cmd.reg = reg;
    cmd.val = val;
    res = i2csessionSendAuto(&_session, &cmd, sizeof(cmd), I2cTransactionOption_All);
    i2csessionClose(&_session);
    return res;
}

Result I2cRead_OutU8(I2cDevice dev, u8 reg, u8 *out) {
    struct { u8 reg; } __attribute__((packed)) cmd;
    struct { u8 val; } __attribute__((packed)) rec;

    I2cSession _session;
    Result res = i2cOpenSession(&_session, dev);
    if (res)
        return res;

    cmd.reg = reg;
    res = i2csessionSendAuto(&_session, &cmd, sizeof(cmd), I2cTransactionOption_All);
    if (res) {
        i2csessionClose(&_session);
        return res;
    }

    res = i2csessionReceiveAuto(&_session, &rec, sizeof(rec), I2cTransactionOption_All);
    i2csessionClose(&_session);
    if (res) {
        return res;
    }

    *out = rec.val;
    return 0;
}

Result I2c_Bq24193_GetFastChargeCurrentLimit(u32 *ma) {
    u8 raw;
    Result res = I2cRead_OutU8(I2cDevice_Bq24193, BQ24193_CHARGE_CURRENT_CONTROL_REG, &raw);
    if (res)
        return res;

    *ma = I2c_Bq24193_Convert_Raw_mA(raw);
    return 0;
}

Result I2c_Bq24193_SetFastChargeCurrentLimit(u32 ma) {
    u8 raw = I2c_Bq24193_Convert_mA_Raw(ma);
    return I2cSet_U8(I2cDevice_Bq24193, BQ24193_CHARGE_CURRENT_CONTROL_REG, raw);
}

