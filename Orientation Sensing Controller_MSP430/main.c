#include <msp430.h>
#include <math.h>


//Converts rawADC value to voltage using internal 3.3V source
#define Vout(v) ((v) * (3.3 / 4095.0))
unsigned int adc_raw[3];
float adc_convert[3];                    // ADC value array
float Vx, Vy, Vz, ax, ay, az, angleX, angleY, angleZ;


void readADC(void);
void LCD_Command(unsigned char);
void LCD_Write(unsigned char);
void LCD_Init(void);
void showVolt(void);
void tiltLED(void);
void convertADC(void);
void angle(void);
void initGPIO(void);


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;               // Stop WDT
    PM5CTL0 &= ~LOCKLPM5;                   // Disable GPIO internal resistance


    // Configure ADC12
    // ADC ON, Sample and hold time set to 128 cycles, Multiple sample for sequence
    ADC12CTL0 = ADC12SHT0_6 | ADC12ON | ADC12MSC;
    // Specifies ADC sample and holde timer (SHP = 1), Specifies sequence-of-channels  
    ADC12CTL1 = ADC12SHP | ADC12CONSEQ_1;                  
    ADC12CTL2 = ADC12RES_2;                 // Specifies 12 bit conversion


    //multi sample conversion. sequence of channel mode
    ADC12MCTL0 = ADC12INCH_9;               //Sets A9 (P4.1)      
    ADC12MCTL1 = ADC12INCH_10;              //Sets A10 (P4.2)
    ADC12MCTL2 = ADC12INCH_11 | ADC12EOS;   // End of sequence at input channel 11 (P4.3)


    initGPIO();
    showVolt();
    P6OUT ^= 0x07;


    while (1)
    {
        ADC12CTL0 |= ADC12ENC | ADC12SC;     // Enable conversion, Start conversion
        while ((ADC12IFGR0 & BIT2)==0);
        showVolt();
        tiltLED();
        _delay_cycles(25000);       // Sets delay after conversion
    }
}


void readADC(void){
    adc_raw[0] = ADC12MEM0;                 // Reads ADC value from A9
    adc_raw[1] = ADC12MEM1;                 // A10
    adc_raw[2] = ADC12MEM2;                 // A11
}

//Converts ADC values across all axes
void convertADC(void){
    readADC();
    unsigned int i;
    for(i = 0; i < 3; i++){
        adc_convert[i] = Vout(adc_raw[i]);
    }
    Vx = adc_convert[0];
    Vy = adc_convert[1];
    Vz = adc_convert[2];
}

//Converts the ADC voltage reading to angle by referencing the accelerometers midpoint
void angle(void){
    convertADC();
    //1.65V is the midpoint of the ADXL335 accelerometer. +- 0.3V change for every 1g
    ax = (Vx - 1.65) / 0.300;   // acceleration in g
    ay = (Vy - 1.65) / 0.300;
    az = (Vz - 1.65) / 0.300;


    angleX = atan2(ax, sqrt(ay*ay + az*az)) * 180.0 / M_PI;     // X-axis
    angleY = atan2(ay, sqrt(ax*ax + az*az)) * 180.0 / M_PI;     // Y-axis
    angleZ = atan2(sqrt(ax*ax + ay*ay), az) * 180.0 / M_PI;     // Z-axis
}


void tiltLED(void){
    angle();
    float quadrant = atan2f(ay, ax) * 180.0 / (float)M_PI;


    float TILT_MIN = 30.0;
    float TILT_MAX = 60.0;


    // If nearly flat or completely still, turn LED off
    if (angleZ < TILT_MIN || angleZ > TILT_MAX) {
        P6OUT = 0x07;
        return;
    }


    if(quadrant < 0.0){
        quadrant += 360.0;              //Eliminates negative angles
    }


    //If 30deg <= tilt <= 60deg, DOWN, LED Green
    if((quadrant >= 315.0f) || (quadrant < 45.0f)) {
        P6OUT = ~BIT1;            //Red + Green
        __delay_cycles(500000);          //half second delay
    }
    //If 30deg <= tilt <= 60deg, RIGHT, LED Yellow
    else if((quadrant >= 45.0f) && (quadrant < 135.0f)) {
        P6OUT = ~(BIT1 | BIT2);
        __delay_cycles(500000);          //half second delay
    }


    //If 30deg <= tilt <= 60deg, UP, LED Blue
    else if((quadrant >= 135.0) && (quadrant < 225.0)) {
        P6OUT = ~BIT0;
        __delay_cycles(500000);          //half second delay
    }


    //If 30deg <= tilt <= 60deg, LEFT, LED Red
    else if((quadrant >= 225.0) && (quadrant < 315.0)) {
        P6OUT = ~BIT2;
        __delay_cycles(500000);          //half second delay
    }
    else{
        P6OUT = 0x07;                 //turns LED off
    }
}


void showVolt(void){
    convertADC();
    int mVolt[3], whole[3], frac[3];
    unsigned int i;
    for(i = 0; i < 3; i++){
        mVolt[i] = adc_convert[i] * 1000;                //Converts to millivolts for integer math
        whole[i] = mVolt[i] / 1000;               //Gets the whole number of value
        frac[i] = mVolt[i] % 1000;                //Gets the remainder for fractional part
    }
    //Print X, Y(Top Row), and Z (Bottom Row) values
    LCD_Command(0x01);                   // Clears the screen
    LCD_Init();
    LCD_Write('X');
    LCD_Write(':');
    LCD_Write('0' + whole[0]);
    LCD_Write('.');
    LCD_Write('0' + (frac[0]/100));            //Hundreds
    LCD_Write('0' + ((frac[0] / 10) % 10));    //Tens
    LCD_Write('0' + (frac[0] % 10));           //Ones
    LCD_Write(' ');


    LCD_Write('Y');
    LCD_Write(':');
    LCD_Write('0' + whole[1]);
    LCD_Write('.');
    LCD_Write('0' + (frac[1]/100));            //Hundreds
    LCD_Write('0' + ((frac[1] / 10) % 10));    //Tens
    LCD_Write('0' + (frac[1] % 10));           //Ones


    LCD_Command(0xC0);                  // Moves to bottom row
    LCD_Write(' ');
    LCD_Write(' ');
    LCD_Write(' ');
    LCD_Write('Z');
    LCD_Write(':');
    LCD_Write('0' + whole[2]);
    LCD_Write('.');
    LCD_Write('0' + (frac[2]/100));            //Hundreds
    LCD_Write('0' + ((frac[2] / 10) % 10));    //Tens
    LCD_Write('0' + (frac[2] % 10));           //Ones


}


void LCD_Command(unsigned char in){
    P3OUT = in;                             //Set P3OUT as argument
    P8OUT &= ~(BIT2 | BIT3);                 //Clear RS and R/W (P8.3 & 8.2)
    P8OUT |= BIT1;                          //Enable Signal (E) (P8.1)
    _delay_cycles(200);             //.2 ms delay
    P8OUT &= ~BIT1;                         //Clear Signal (E)
}


void LCD_Write(unsigned char in){
    P3OUT = in;                             //Set P3OUT as argument
    //Set RS and clear R/W to write to LCD
    P8OUT |= BIT3;
    P8OUT &= ~BIT2;
    P8OUT |= BIT1;                          //Enable signal
    __delay_cycles(200);            //delay
    P8OUT &= ~BIT1;                          //Clear signal
}


void LCD_Init(void){
    P8OUT &= ~0x02;                           //Clear signal
    _delay_cycles(15000);            //15ms delay
    LCD_Command(0x30);                  //wakes up LCD
    _delay_cycles(300);
    LCD_Command(0x30);                  
    _delay_cycles(300);
    LCD_Command(0x30);                  
    _delay_cycles(300);  


    LCD_Command(0x38);                  //function set: 8-bit/2-line
    LCD_Command(0x10);                  //Sets cursor
    LCD_Command(0x0F);                  //Turns on the display
    LCD_Command(0x06);                  //Entry mode set
    LCD_Command(0x01);                  //Clears display
    __delay_cycles(3000);            //3ms delay

}


void initGPIO(void){
    // Configure GPIO
    // Set alternate function for A9, A10, A11
    P4SEL1 |= BIT1 | BIT2 | BIT3;          
    P4SEL0 |= BIT1 | BIT2 | BIT3;

    //LCD (P8.1 - 8.3, P3.0 - 3.7)
    P3OUT &= ~0xFF;                         // Clears output latch for P3.0 - 3.7
    P3DIR |= 0xFF;                          // Sets P3.0 - 3.7 as output
    P8OUT &= ~0x0E;                         //Clears output latch for P8.1 - 8.3
    P8DIR |= 0x0E;                          //Sets P8.1 - 8.3 as output
    //LED (P6.0 - 6.2)
    P6OUT &= ~0x07;
    P6DIR |= 0x07;
}



