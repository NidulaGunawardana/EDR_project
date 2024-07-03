#include <stdint.h>
#include <stdio.h>

// Simulated delay function
void delay(uint32_t cycles) {
    while (cycles--);
}

void dumpLoadOn() { PORTB.OUTSET = PIN1_bm; }

void dumpLoadOff() { PORTB.OUTCLR = PIN1_bm; }

void referenceVoltageOn() {
    PORTA.OUTSET = PIN1_bm;  // Ref voltage ON (PA1)
    PORTB.OUTSET = PIN0_bm;  // Enable (PB0)
    delay(50);
}

void referenceVoltageOff() {
    PORTA.OUTCLR = PIN1_bm;  // Ref voltage OFF (PA1)
    PORTB.OUTCLR = PIN0_bm;  // Disable (PB0)
}

uint16_t readADC() { return ADC0.RES; }

uint16_t readADC1() { return ADC1.RES; }

void selectCellVoltageChannel() {
    // Do nothing, as default configuration is already set
}

void selectInternalTemperatureChannel() {
    ADC0.MUXPOS = ADC_MUXPOS_AIN7_gc;  // PA7
}

void selectExternalTemperatureChannel() {
    ADC0.MUXPOS = ADC_MUXPOS_AIN3_gc;  // PA3
}

void notificationLedOn() { PORTA.OUTSET = PIN6_bm; }

void notificationLedOff() { PORTA.OUTCLR = PIN6_bm; }

uint16_t beginADCReading(uint8_t adcMode) {
    uint16_t value = 0;

    if (adcMode == 0) {
        // Cell voltage (ADC1)
        ADC1.CTRLB = ADC_SAMPNUM_ACC16_gc;  // Oversample 16 times
        ADC1.CTRLC = ADC_REFSEL_VREFA_gc | ADC_PRESC_DIV8_gc | ADC_SAMPCAP_bm;
        ADC1.MUXPOS = ADC_MUXPOS_AIN0_gc;
        ADC1.CTRLA = ADC_RESSEL_10BIT_gc | ADC_ENABLE_bm;
        ADC1.COMMAND = ADC_STCONV_bm;  // Start conversion

        while (!(ADC1.INTFLAGS & ADC_RESRDY_bm)) {
            // Wait for conversion to complete
        }

        value = ADC1.RES >> 4;  // Adjust for oversampling
        ADC1.CTRLA &= ~ADC_ENABLE_bm;  // Disable ADC1
    } else {
        // Temperature (ADC0)
        ADC0.CTRLB = ADC_SAMPNUM_ACC16_gc;  // Oversample 16 times
        ADC0.CTRLC = ADC_REFSEL_VDDREF_gc | ADC_PRESC_DIV8_gc | ADC_SAMPCAP_bm;
        ADC0.CTRLA = ADC_RESSEL_10BIT_gc | ADC_ENABLE_bm;
        ADC0.COMMAND = ADC_STCONV_bm;  // Start conversion

        while (!(ADC0.INTFLAGS & ADC_RESRDY_bm)) {
            // Wait for conversion to complete
        }

        value = ADC0.RES >> 4;  // Adjust for oversampling
        ADC0.CTRLA &= ~ADC_ENABLE_bm;  // Disable ADC0
    }

    return value;
}

void configurePorts() {
    // Set Port A digital outputs: PA1 (REF_ENABLE), PA6 (NOTIFICATION LED)
    PORTA.DIRSET = PIN1_bm | PIN6_bm;

    // Set Port B digital outputs: PB0 (ENABLE), PB1 (DUMP LOAD ENABLE), PB2 (TXD)
    PORTB.DIRSET = PIN0_bm | PIN1_bm | PIN2_bm;
    PORTB.DIRCLR = PIN3_bm;  // RXD as input

    // Set Port A analog inputs: PA3 (EXT TEMP SENSOR), PA4 (VOLTAGE INPUT), PA7 (INT TEMP SENSOR)
    PORTA.DIRCLR = PIN3_bm | PIN4_bm | PIN7_bm;

    // Disable digital input buffers for unused or analog input/output pins
    PORTA.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTA.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTA.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTA.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTA.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTA.PIN5CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTA.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTB.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTB.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;

    // Set VREF configurations
    VREF.CTRLA = VREF_ADC0REFSEL_4V34_gc;  // Set ADC0 Vref to 4.3V
    VREF.CTRLC = VREF_ADC1REFSEL_1V5_gc;   // Set ADC1 Vref to 1.5V
    VREF.CTRLB = 0;  // Switch off "force" internal references

    // Set initial pin states
    dumpLoadOff();
    referenceVoltageOff();
    notificationLedOff();
}

void setup() {
    configurePorts();
    Serial.begin(115200);
    printf("\n\n\nI'm alive...\n");
}

void loop() {
    referenceVoltageOn();
    printf("Loop\n");

    printf("Voltage ADC=");
    uint16_t rawAdcVoltage = beginADCReading(0);
    printf("%u ", rawAdcVoltage);

    // Integer math to calculate voltage
    rawAdcVoltage <<= 1;  // Multiply by 2
    uint32_t calculatedVoltage = ((uint32_t)rawAdcVoltage * 21748UL) / 100000UL;
    printf("%lu mV\n", calculatedVoltage);

    referenceVoltageOff();

    referenceVoltageOn();
    selectInternalTemperatureChannel();
    printf("Internal Temp ADC=%u\n", beginADCReading(1));
    referenceVoltageOff();

    referenceVoltageOn();
    selectExternalTemperatureChannel();
    printf("External Temp ADC=%u\n", beginADCReading(2));
    referenceVoltageOff();

}

int main() {
    setup();
    while (1) {
        loop();
    }
    return 0;
}
