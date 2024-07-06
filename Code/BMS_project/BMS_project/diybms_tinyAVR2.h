

#define DIYBMS_tinyAVR2_H

#pragma once


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

// Used for temperature readings only (13 bit ADC with oversample)
#define MAXIUMUM_ATTINY_ADC_SCALE 8191.0F

/*
This class wraps the hardware pins of DIYBMS away from the core logic/code
if you are porting to another chipset, clone this class and modify it.
*/
class diyBMSHAL
{
	public:
	static void ConfigurePorts();

	inline static void ResumePWM() __attribute__((always_inline))
	{
		TCA0.SINGLE.CTRLA |= 0x01; // TCA_SINGLE_ENABLE_bm = 0x01
		sei(); // Enable global interrupts
	}

	inline static void StopTimer1() __attribute__((always_inline))
	{
		TCA0.SINGLE.CTRLA &= ~0x01; // TCA_SINGLE_ENABLE_bm = 0x01
	}

	static void StartTimer1()
	{
		// Top value...
		TCA0.SINGLE.PER = (F_CPU / 4) / 1000;
		//5000000/4/1000 = 1250

		// CMP0, Compare Channel 0 interrupt = Match between the counter value and the Compare 0 register
		//TCA0.SINGLE.CMP0=0x1000;
		TCA0.SINGLE.INTCTRL = 0x01; // TCA_SINGLE_OVF_bm = 0x01
		TCA0.SINGLE.CTRLA = 0x04; // TCA_SINGLE_CLKSEL_DIV4_gc = 0x04
		ResumePWM();
	}

	static void PausePWM()
	{
		StopTimer1();
		DumpLoadOff();
	}

	static void SetPrescaler()
	{
		// This isn't needed for tiny2, chip runs at 5Mhz internal clock (to allow down to 1.8V operation)
	}

	static inline void DumpLoadOn() { PORTB.OUTSET = 0x02; } // PIN1_bm = 0x02

	static inline void DumpLoadOff() { PORTB.OUTCLR = 0x02; } // PIN1_bm = 0x02

	inline static void ReferenceVoltageOn() __attribute__((always_inline))
	{
		// Switch REFERENCE VOLTAGE on
		// Ref voltage ON (PA1)
		PORTA.OUTSET = 0x02; // PIN1_bm = 0x02
	}

	inline static void ReferenceVoltageOff() __attribute__((always_inline))
	{
		// Ref voltage (PA1)
		PORTA.OUTCLR = 0x02; // PIN1_bm = 0x02
	}

	inline static void TemperatureVoltageOn() __attribute__((always_inline))
	{
		// PB0
		PORTB.OUTSET = 0x01; // PIN0_bm = 0x01
	}

	inline static void TemperatureVoltageOff() __attribute__((always_inline))
	{
		// PB0
		PORTB.OUTCLR = 0x01; // PIN0_bm = 0x01
	}

	static void FlashNotificationLed(size_t times, uint32_t milliseconds);
	static void PowerOn_Notification_led();

	static inline void NotificationLedOn() __attribute__((always_inline)) { PORTA.OUTSET = 0x40; } // PIN6_bm = 0x40

	static inline void NotificationLedOff() __attribute__((always_inline)) { PORTA.OUTCLR = 0x40; } // PIN6_bm = 0x40

	static inline void SpareToggle() __attribute__((always_inline)) { PORTA.OUTTGL = 0x04; } // PIN2_bm = 0x04

	static inline void SpareOn() __attribute__((always_inline)) { PORTA.OUTSET = 0x04; } // PIN2_bm = 0x04

	static inline void SpareOff() __attribute__((always_inline)) { PORTA.OUTCLR = 0x04; } // PIN2_bm = 0x04

	static inline void FlushSerial0() __attribute__((always_inline)) { while (!(USART0.STATUS & 0x40)); } // Serial flush, check TXCIF bit

	static inline void DisableSerial0TX() __attribute__((always_inline))
	{
		// On tiny1624 this saves about 7mA of current
		USART0.CTRLB &= ~(0x08); // USART_TXEN_bm = 0x08
	}

	static inline void EnableSerial0TX() __attribute__((always_inline))
	{
		// When the transmitter is disabled, it will no longer override the TXD pin, and the pin
		// direction is automatically set as input by hardware, even if it was configured as output by the user
		USART0.CTRLB |= 0x08; // USART_TXEN_bm = 0x08
		PORTB.DIRSET = 0x04; // PIN2_bm = 0x04
	}

	// The Start-of-Frame Detection feature enables the USART to wake up from Standby Sleep mode upon data reception.
	static inline void EnableStartFrameDetection() __attribute__((always_inline))
	{
		USART0.CTRLB |= 0x10; // USART_SFDEN_bm = 0x10
	}

	static void SetWatchdog8sec()
	{
		// Setup a watchdog timer for 8 seconds
		CCP = 0xD8;
		// 8 seconds
		WDT.CTRLA = 0x0A; // WDT_PERIOD_8KCLK_gc = 0x0A
		wdt_reset();
	}

	static uint16_t BeginADCReading(uint8_t adcmode);

	static void Sleep()
	{
		sei(); // Enable global interrupts

		// Switch off TX - save power
		diyBMSHAL::DisableSerial0TX();

		// RUNSTBY

		// Standby mode is needed to allow the "USART Start-of-Frame interrupts" to wake CPU

		// Wake up on Serial port RX
		diyBMSHAL::EnableStartFrameDetection();

		set_sleep_mode(SLEEP_MODE_STANDBY);
		sleep_enable();
		sleep_cpu();

		// Snoring can be heard at this point....

		sleep_disable();
	}

	static void SelectCellVoltageChannel()
	{
		// FREERUN / LEFTADJ / SAMPNUM[3:0]
		ADC0.CTRLF = 0x50; // ADC_SAMPNUM_ACC128_gc = 0x50
		// PA5 = VREF pin
		ADC0.MUXPOS = 0x05; // ADC_MUXPOS_AIN5_gc = 0x05
	}

	static inline void SelectInternalTemperatureChannel()
	{
		// PA7
		// FREERUN / LEFTADJ / SAMPNUM[3:0]
		ADC0.CTRLF = 0x10; // ADC_SAMPNUM_ACC2_gc = 0x10
		// PA7 = AIN7 pin
		ADC0.MUXPOS = 0x07; // ADC_MUXPOS_AIN7_gc = 0x07
	}

	static inline void SelectExternalTemperatureChannel()
	{
		// PA3
		// FREERUN / LEFTADJ / SAMPNUM[3:0]
		ADC0.CTRLF = 0x10; // ADC_SAMPNUM_ACC2_gc = 0x10
		// PA3 = AIN3 pin
		ADC0.MUXPOS = 0x03; // ADC_MUXPOS_AIN3_gc = 0x03
	}

	static void double_tap_Notification_led();
};

#endif

#endif
