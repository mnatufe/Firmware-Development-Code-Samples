#include <msp430.h>
//Reed Minor & Michael Natufe

void uartInit(void);
void uartPutC(char c);
void uartPutS(char *str);
void uartPrintFloat(float num);
void adcInit(void);
unsigned int read_adc_raw(void);
float TempCtoF(unsigned int raw);

int main(void)
{
   WDTCTL = WDTPW | WDTHOLD; #Hold watchdog timer
   PM5CTL0 &= ~LOCKLPM5; #Disable GPIO high impedance mode

  #Set GPIO pin directions
   P1DIR |= BIT0 | BIT2;
   P1OUT &= ~BIT2;

#Initialize UART and ADC
   uartInit();
   adcInit();

  #Set Capture-and-Control Register clock
   TA1CCR0 = 20000 - 1;
   TA1CCR1 = 1500;
   TA1CCTL0 = CCIE;
   TA1CCTL1 = CCIE;
   TA1CTL = TASSEL_2 | MC_1 | TACLR;
   __enable_interrupt();

  #Send ready message to terminal
   uartPutS("System A Ready\r\n");



#Wait for user input from terminal and returns output 
   while (1)
   {
       while (!(UCA0IFG & UCRXIFG));
       char input = UCA0RXBUF;
       switch (input)
       {
           case '0':
               uartPutS("\r\nMenu:\r\n");
               uartPutS("0. List\r\n");
               uartPutS("1. Name\r\n");
               uartPutS("2. 0deg\r\n");
               uartPutS("3. 45deg\r\n");
               uartPutS("4. -45deg\r\n");
               uartPutS("5. Temp\r\n");
               break;
           case '1':
               uartPutS("ESET 369: Reed and Michael\r\n");
               break;
           case '2':
               uartPutS("Servo 0 degrees\r\n");
               TA1CCR1 = 1500;                   // 1.5 ms pulse = 0°
               break;
           case '3':
               uartPutS("Servo +45 degrees\r\n");
               TA1CCR1 = 1000;
                                            // 1.0 ms pulse = +45°
               break;

           case '4':
               uartPutS("Servo -45 degrees\r\n");
               TA1CCR1 = 2000;           // 2.0 ms pulse = -45°
               break;
           case '5':{
               unsigned int raw_adc = read_adc_raw();
               float tempF = TempCtoF(raw_adc);

               uartPutS("Temperature: ");
               uartPrintFloat(tempF);
               uartPutS(" F\r\n");
               break;}

           default:
               uartPutS("Invalid input. Press 1 to see options.\r\n");
               break;
       }
   }
   return 0;
}

#UART Setup
void uartInit(void)
{
   UCA0CTLW0 |= UCSWRST;
   UCA0CTLW0 |= UCSSEL_2;
   UCA0BRW = 6;
   UCA0MCTLW = UCOS16 | (8 << 4) | (32 << 8);
  #Enable alternate mode for ADC conversion
   P2SEL1 |= BIT0 | BIT1;
   P2SEL0 &= ~(BIT0 | BIT1);
   UCA0CTLW0 &= ~UCSWRST;
}

void uartPutC(char c)
{
   while (!(UCA0IFG & UCTXIFG));
   UCA0TXBUF = c;
}

void uartPutS(char *str)
{
   while (*str)
       uartPutC(*str++);
}

void uartPrintFloat(float num)
{
   if (num < 0) {
       uartPutC('-');
       num = -num;
   }
  #Integer math to convert the temperature values for UART transmission
   int whole = (int)num;
   int decimal = (int)((num - whole) * 100 + 0.5);
   if (whole >= 100) {
       uartPutC('0' + (whole / 100));
       uartPutC('0' + ((whole / 10) % 10));
   } else if (whole >= 10) {
       uartPutC('0' + (whole / 10));
   } else {
       uartPutC('0');
   }

   uartPutC('0' + (whole % 10));
   uartPutC('.');
   uartPutC('0' + (decimal / 10));
   uartPutC('0' + (decimal % 10));
}

void adcInit(void)
{
   while(REFCTL0 & REFGENBUSY);          // Wait if reference busy
   REFCTL0 |= REFON | REFVSEL_2;         // Enable 2.5V ref
   __delay_cycles(75);                   // Wait for ref to settle

   ADC12CTL0 = ADC12SHT0_6 | ADC12ON;    // Sample hold
   ADC12CTL1 = ADC12SHP;                 // Pulse mode
   ADC12CTL2 = ADC12RES_2;               // 12-bit resolution
   ADC12CTL3 = ADC12TCMAP;               // Temp sensor mapped to channel 30
   ADC12MCTL0 = ADC12VRSEL_1 | ADC12INCH_30; // Use internal ref, channel 30
}

#Read microcontrollers internal raw ADC value
unsigned int read_adc_raw(void)
{
   ADC12CTL0 |= ADC12ENC | ADC12SC;
   while (ADC12CTL1 & ADC12BUSY);
   return ADC12MEM0;
}

#Convert degrees Celsius to degrees Fahrenheit 
float TempCtoF(unsigned int raw)
{
   unsigned int Ref_T30 = *((unsigned int *)(TLV_START + TLV_ADC12CAL + 0x09));
   unsigned int Ref_T85 = *((unsigned int *)(TLV_START + TLV_ADC12CAL + 0x0B));
   float TempDegC = (((float)raw - Ref_T30) * 55) / (Ref_T85 - Ref_T30) + 30.0;
   return TempDegC * 9.0f / 5.0f + 32.0f;
}

#pragma vector = TIMER1_A0_VECTOR
__interrupt void Timer1_A0_ISR(void) {
   P1OUT |= BIT0;
}

#pragma vector = TIMER1_A1_VECTOR
__interrupt void Timer1_A1_ISR(void) {
   if ((TA1CCTL1 & CCIFG)!=0) {
       P1OUT ^= BIT0;
       TA1CCTL1 &= ~CCIFG;
   }
}




