/*
 * File:   newmain.c
 * Author: CETEC
 *
 * Created on 26 March, 2024, 9:32 PM
 */




// CONFIG

#pragma config FOSC = HS       // Oscillator Selection bits (HS oscillator)

#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)

#pragma config PWRTE = OFF       // Power-up Timer Enable bit (PWRT enabled)

#pragma config BOREN = ON        // Brown-out Reset Enable bit (BOR enabled)

#pragma config LVP = OFF        // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit (RB3 is digital I/O, HV on MCLR must be used for programming)

#pragma config CPD = OFF        // Data EEPROM Memory Code Protection bit (Data EEPROM code protection off)

#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off; all program memory may be written to by EECON control)

#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)

//End of CONFIG registers


#define _XTAL_FREQ 20000000
char buffer[20];
//double voltage;

#include<xc.h>
#include<stdio.h>
#include<stdint.h>
#include<pic16f877a.h>


void initADC(void) {
    ADCON1 = 0b00000000; // Set ADCON1 for analog input
    ADCON0 = 0b00000001; // Turn on ADC and select channel 0 (AN0)
    PIE1bits.ADIE = 1;      // Enable ADC interrupt
    PIR1bits.ADIF = 0;      // Clear ADC interrupt flag
    __delay_us(5); // Acquisition time delay
}

unsigned int readADC(void) {
    
    GO_nDONE = 1; // Start conversion
    while (GO_nDONE); // Wait for conversion to complete
    return (((unsigned int)ADRESH << 8) + ADRESL) ;// Return 10-bit ADC result
            
}
    
void initUART(void) {
    TRISC6 = 0; // TX pin as output
    TRISC7 = 1; // RX pin as input
    SPBRG =129; // 9600 baud rate for 20 MHz crystal
    TXSTAbits.TX9=0;
    TXSTAbits.TXEN=1;
    TXSTAbits.SYNC=0;
    TXSTAbits.BRGH=1;
    RCSTAbits.SPEN=1;
    RCSTAbits.CREN=1;
    INTCONbits.GIE=1;
    INTCONbits.PEIE=1;
    //TXSTA = 0b00100000; // TX enable, asynchronous mode, high baud rate select
    //RCSTA = 0b10010000; // Serial port enable, continuous receive enable
}
void sendUART(char byte)  
{
TXREG = byte;
while(!TXIF);  
while(!TRMT);

}
void UARTstring(char* string)
{
    while (*string != '\0') {
        sendUART(*string); // Send each character in the string
        if (*string == ' ') {
            sendUART('\n'); // If the current character is a space, send a newline
        }
        string++;
    }
}

void broadcast_BT()

{
 TXREG = 13;  // to start from left end
 __delay_ms(50);

}
void main(void)
{
    float voltage;
    initADC();
    TRISB0=0;
    TRISB2 = 0; // Set RB3 as output for LED control
    volatile unsigned int steps = 0;
    unsigned int threshold = 1;
    unsigned int adc_value=0;
    unsigned int adc_valuep=0;
    unsigned int force=0;
    initUART();
    __delay_ms(500);
    UARTstring("ready");
            broadcast_BT();
    
    while (1) // Infinite loop
        
    {   
        
       PORTBbits.RB2=0;
       __delay_ms(1000);
       PORTBbits.RB2=1;
       __delay_ms(1000);
        
        //if (ADIF){
        // Combine ADRESH and ADRESL to form a 10-bit value
       adc_value = readADC();

      voltage = (float)(adc_value * 5.0 / 65535.0); 
       if (  voltage==0 || voltage ==1)
       {
       force = 20;
       }
       else if (voltage ==2)
       {
           force = 40;
       }
       else if (voltage ==3)
       {
           force = 60;
       }
       else if (voltage ==4)
       {
           force = 80;
       }
       else
       {
           UARTstring("overload");
           broadcast_BT();
                   
       }
        
        
        // Check if the voltage is greater than 1V
        if (voltage >= threshold) {
            
            if (adc_valuep!=adc_value)
            {
            // Increment steps
            steps++;
            }
            PORTBbits.RB0=1;
            
            // Send the value over UART
            UARTstring("ADC_value:");
            snprintf(buffer,sizeof(adc_value), "%u\n", adc_value);
            UARTstring(buffer);
            broadcast_BT();
            UARTstring("Voltage:");
            //int t;
            //t=voltage;
            //int t2;
            //t2=(voltage-t)*1000;
            snprintf(buffer,sizeof(buffer)," %f\n", voltage);
            UARTstring(buffer);
            broadcast_BT();
            UARTstring("Force:");
            snprintf(buffer,sizeof(buffer), "%u\n", force);
            UARTstring(buffer);
            broadcast_BT();
            UARTstring("steps:");
            //sendUART(voltage >> 8); 
            snprintf(buffer,sizeof(buffer), "%u\n", steps); // Convert adc_value to string
            UARTstring(buffer);
            broadcast_BT();// Send high byte
            //sendUART(voltage & 0xFF);  // Send low byte
            //sprintf(buffer, "Voltage: %u,",voltage);
        }
        else{
       PORTBbits.RB0=0;
       UARTstring("ADC_value:");
            snprintf(buffer,sizeof(buffer), "%u\n", adc_value);
            UARTstring(buffer);
            broadcast_BT();
            UARTstring("Voltage:");
            snprintf(buffer,sizeof(buffer), "%f\n", voltage);
            UARTstring(buffer);
            broadcast_BT();
            UARTstring("steps:");
            //sendUART(voltage >> 8); 
            snprintf(buffer,sizeof(buffer), "%u\n", steps); // Convert adc_value to string
            UARTstring(buffer);
            broadcast_BT();
        }
       adc_valuep=adc_value;
       __delay_ms(50);

        ADIF = 0;  // Clear ADC interrupt flag
        GO_nDONE = 1;    // Start next ADC conversion
        }
    
}

