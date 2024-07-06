#include <Arduino.h>

void dumpLoadOn() { PORTB.OUTSET = 0x02; }

void dumpLoadOff() { PORTB.OUTCLR = 0x02; }

void referenceVoltageOn() {
  PORTA.OUTSET = 0x02;  // Ref voltage ON (PA1)
  PORTB.OUTSET = 0x01;  // Enable (PB0)
  delayMicroseconds(50);
}

void referenceVoltageOff() {
  PORTA.OUTCLR = 0x02;  // Ref voltage OFF (PA1)
  PORTB.OUTCLR = 0x01;  // Disable (PB0)
}

uint16_t readADC() { return ADC0.RES; }

uint16_t readADC1() { return ADC1.RES; }

void selectCellVoltageChannel() {
  // Do nothing, as default configuration is already set
}

void selectInternalTemperatureChannel() {
  ADC0.MUXPOS = 0x07;  // PA7
}

void selectExternalTemperatureChannel() {
  ADC0.MUXPOS = 0x03;  // PA3
}

void notificationLedOn() { PORTA.OUTSET = 0x40; }

void notificationLedOff() { PORTA.OUTCLR = 0x40; }

uint16_t beginADCReading(uint8_t adcMode) {
  uint16_t value = 0;

  if (adcMode == 0) {
    // Cell voltage (ADC1)
    ADC1.CTRLB = 0x10;  // Oversample 16 times
    ADC1.CTRLC = 0x18;  // ADC_REFSEL_VREFA_gc | ADC_PRESC_DIV8_gc | ADC_SAMPCAP_bm;
    ADC1.MUXPOS = 0x00; // ADC_MUXPOS_AIN0_gc;
    ADC1.CTRLA = 0x02;  // ADC_RESSEL_10BIT_gc | ADC_ENABLE_bm;
    ADC1.COMMAND = 0x01;  // ADC_STCONV_bm;  // Start conversion

    while (!(ADC1.INTFLAGS & 0x01)) {
      // Wait for conversion to complete
    }

    value = ADC1.RES >> 4;  // Adjust for oversampling
    ADC1.CTRLA &= ~0x01;  // Disable ADC1
  } else {
    // Temperature (ADC0)
    ADC0.CTRLB = 0x10;  // Oversample 16 times
    ADC0.CTRLC = 0x08;  // ADC_REFSEL_VDDREF_gc | ADC_PRESC_DIV8_gc | ADC_SAMPCAP_bm;
    ADC0.CTRLA = 0x02;  // ADC_RESSEL_10BIT_gc | ADC_ENABLE_bm;
    ADC0.COMMAND = 0x01;  // ADC_STCONV_bm;  // Start conversion

    while (!(ADC0.INTFLAGS & 0x01)) {
      // Wait for conversion to complete
    }

    value = ADC0.RES >> 4;  // Adjust for oversampling
    ADC0.CTRLA &= ~0x01;  // Disable ADC0
  }

  return value;
}

void configurePorts() {
  // Set Port A digital outputs: PA1 (REF_ENABLE), PA6 (NOTIFICATION LED)
  PORTA.DIRSET = 0x02 | 0x40;

  // Set Port B digital outputs: PB0 (ENABLE), PB1 (DUMP LOAD ENABLE), PB2 (TXD)
  PORTB.DIRSET = 0x01 | 0x02 | 0x04;
  PORTB.DIRCLR = 0x08;  // RXD as input

  // Set Port A analog inputs: PA3 (EXT TEMP SENSOR), PA4 (VOLTAGE INPUT), PA7 (INT TEMP SENSOR)
  PORTA.DIRCLR = 0x08 | 0x10 | 0x80;

  // Disable digital input buffers for unused or analog input/output pins
  PORTA.PIN0CTRL = 0x00;
  PORTA.PIN1CTRL = 0x00;
  PORTA.PIN2CTRL = 0x00;
  PORTA.PIN3CTRL = 0x00;
  PORTA.PIN4CTRL = 0x00;
  PORTA.PIN5CTRL = 0x00;
  PORTA.PIN6CTRL = 0x00;
  PORTA.PIN7CTRL = 0x00;
  PORTB.PIN0CTRL = 0x00;
  PORTB.PIN1CTRL = 0x00;

  // Set VREF configurations
  VREF.CTRLA = 0x04;  // VREF_ADC0REFSEL_4V34_gc;  // Set ADC0 Vref to 4.3V
  VREF.CTRLC = 0x01;  // VREF_ADC1REFSEL_1V5_gc;   // Set ADC1 Vref to 1.5V
  VREF.CTRLB = 0x00;  // Switch off "force" internal references

  // Set initial pin states
  dumpLoadOff();
  referenceVoltageOff();
  notificationLedOff();
}

void setup() {
  configurePorts();
  Serial.begin(115200, SERIAL_8N1);
  Serial.println("\n\n\nI'm alive...");
}

void loop() {
  referenceVoltageOn();
  Serial.println("Loop");

  Serial.print("Voltage ADC=");
  uint16_t rawAdcVoltage = beginADCReading(0);
  Serial.print(rawAdcVoltage);
  Serial.print(' ');

  // Integer math to calculate voltage
  rawAdcVoltage <<= 1;  // Multiply by 2
  uint32_t calculatedVoltage = ((uint32_t)rawAdcVoltage * 21748UL) / 100000UL;
  Serial.print(calculatedVoltage);
  Serial.println(" mV");

  referenceVoltageOff();

  referenceVoltageOn();
  selectInternalTemperatureChannel();
  Serial.print("Internal Temp ADC=");
  Serial.println(beginADCReading(1));
  referenceVoltageOff();

  referenceVoltageOn();
  selectExternalTemperatureChannel();
  Serial.print("External Temp ADC=");
  Serial.println(beginADCReading(2));
  referenceVoltageOff();

  delay(4000);  // Delay between readings
}
