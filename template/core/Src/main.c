//RWLJEN001 Jenna Rowley
//EEE2050F Assignment

//********************************************************************
// INCLUDE FILES
//====================================================================
#include "stm32f0xx.h"
#include "lcd_stm32f0.h"
#include "stdbool.h"
//====================================================================
// GLOBAL VARIABLES
//====================================================================
//flags
bool startFlag = false;
bool lapFlag = false;
bool stopFlag = false;
bool resetFlag = true;

//timing variables
uint8_t minutes = 0;
uint8_t seconds = 0;
uint8_t hundredths = 0;

//timing variables for laps
uint8_t minutesLap = 0;
uint8_t secondsLap = 0;
uint8_t hundredthsLap = 0;

//bool to check if running or lap
bool running = false;
//====================================================================
// FUNCTION DECLARATIONS
//====================================================================
void initGPIO();
void initTIM14();

void TIM14_IRQHandler();

void display(void);

void checkPB(void);

void convert2BCDASCII(const uint8_t min, const uint8_t sec, const uint8_t hund, char* resultPtr);
//====================================================================
// MAIN FUNCTION
//====================================================================
int main (void)
{
    //clear LEDs
    GPIOB->ODR = 0x00;

    //initialise LCD, GPIO and TIM14
    init_LCD();
    initGPIO();
    initTIM14();
    
    //check switches and update display
    while (1){
        checkPB();
        display();
    }

}							// End of main
//====================================================================
// FUNCTION DEFINITIONS
//====================================================================
void initGPIO(){
    //enable clock signal to GPIOA and GPIOB
    RCC -> AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;

    //configure pins PA0, PA1, PA2 and P3 to input mode --> switches
    GPIOA->MODER &= ~(GPIO_MODER_MODER0 | GPIO_MODER_MODER1 | GPIO_MODER_MODER2 | GPIO_MODER_MODER3);

    //clear pull-up resistors for switches
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR0 | GPIO_PUPDR_PUPDR1 | GPIO_PUPDR_PUPDR2 | GPIO_PUPDR_PUPDR3);
    
    //enable pull-up resistors for switches
    GPIOA->PUPDR |= (GPIO_PUPDR_PUPDR0_0 | GPIO_PUPDR_PUPDR1_0 | GPIO_PUPDR_PUPDR2_0 | GPIO_PUPDR_PUPDR3_0);

    //clear pins PB0-3 --> LEDs
    GPIOB->MODER &= ~(GPIO_MODER_MODER0 | GPIO_MODER_MODER1 | GPIO_MODER_MODER2 | GPIO_MODER_MODER3);

    //set pins PB0-3 to output mode --> LEDs
    GPIOB->MODER |= (GPIO_MODER_MODER0_0 | GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0);
}

void initTIM14(){
    //enable clock signal to TIM14
    RCC -> APB1ENR |= RCC_APB1ENR_TIM14EN;

    //set PSC and ARR parameters
    TIM14 -> PSC = 7999;
    TIM14 -> ARR = 9;

    //enable update event interrupts
    TIM14 -> DIER |= TIM_DIER_UIE;

    //stop timer and set to upcounting mode (default)
    TIM14->CR1 &= ~TIM_CR1_CEN;

    //unmask global Timer 14 update event NVIC interrupts
    NVIC_EnableIRQ(TIM14_IRQn);
}

void TIM14_IRQHandler(){
    //clear interrupt flag
    TIM14 -> SR &= ~TIM_SR_UIF;

    //only increment if timer is running
    if(running){
        //increment hundredths
        hundredths++;

        //if 100 hundreths, carry over to seconds(increment) and reset hundreths
        if(hundredths >= 100){
            hundredths = 0;
            seconds++;

            //if 60 seconds, carry over to minutes(increment) and reset seconds
            if(seconds >= 60){
                seconds = 0;
                minutes++;
            }
        }
    }

}

void display(void){
    if(startFlag == false && lapFlag == false && stopFlag == false && resetFlag == true){
        //clear output and print out starting of stop watch
        lcd_command(CLEAR);
        lcd_putstring("Stop Watch");
        lcd_command(LINE_TWO);
        lcd_putstring("Press sw0...");

        //switch LED 3 on
        GPIOB -> ODR = 0b00001000;
    }

    if(startFlag == true && lapFlag == false && stopFlag == false && resetFlag == false){
        //call function to get correct output
        char buffer[8];
        convert2BCDASCII(minutes, seconds, hundredths, buffer);

        //write output to LCD
        lcd_command(CLEAR);
        lcd_putstring("Time");
        lcd_command(LINE_TWO);
        lcd_putstring(buffer);

        //switch on LED 0
        GPIOB -> ODR = 0b00000001;
    }
    if(startFlag == true && lapFlag == true && stopFlag == false && resetFlag == false){
        //call function to get correct output
        char buffer[8];
        convert2BCDASCII(minutesLap, secondsLap, hundredthsLap, buffer);

        //write output to LCD
        lcd_command(CLEAR);
        lcd_putstring("Time");
        lcd_command(LINE_TWO);
        lcd_putstring(buffer);

        //switch on LED 1
        GPIOB -> ODR = 0b00000010;
    }
    if(startFlag == true && lapFlag == false && stopFlag == true && resetFlag == false){
        //call function to get correct output
        char buffer[8];
        convert2BCDASCII(minutes, seconds, hundredths, buffer);

        //write output to LCD
        lcd_command(CLEAR);
        lcd_putstring("Time");
        lcd_command(LINE_TWO);
        lcd_putstring(buffer);

        //switch on LED 2
        GPIOB -> ODR = 0b00000100;
    }

}

void checkPB(void){
    //sw0 pressed
    if((GPIOA -> IDR & GPIO_IDR_0) == 0){
        startFlag = true;
        lapFlag = false;
        stopFlag = false;
        resetFlag = false;

        //timer is running
        running = true;

        //start counter for timer 14
        TIM14 -> CR1 |= TIM_CR1_CEN;
    }
    //sw1 pressed
    else if((GPIOA -> IDR & GPIO_IDR_1) == 0){
        startFlag = true;
        lapFlag = true;
        stopFlag = false;
        resetFlag = false;

        //keep timer running
        running = true;

        //record lap times
        minutesLap = minutes;
        secondsLap = seconds;
        hundredthsLap = hundredths;
    }
    //sw2 pressed
    else if((GPIOA -> IDR & GPIO_IDR_2) == 0){
        startFlag = true;
        lapFlag = false;
        stopFlag = true;
        resetFlag = false;

        //stop timer
        running = false;
    }
    //sw3 pressed
    else if((GPIOA -> IDR & GPIO_IDR_3) == 0){
        startFlag = false;
        lapFlag = false;
        stopFlag = false;
        resetFlag = true;

        minutes = 0;
        seconds = 0;
        hundredths = 0;

        minutesLap = 0
        secondsLap = 0;
        hundredthsLap = 0

        //stop timer
        running = false;
    }

}

void convert2BCDASCII(const uint8_t min, const uint8_t sec, const uint8_t hund, char* resultPtr){
    //convert min, sec and hund to BCD
    uint8_t minBCD  = ((min  / 10) << 4) | (min  % 10);
    uint8_t secBCD  = ((sec  / 10) << 4) | (sec  % 10);
    uint8_t hundBCD = ((hund / 10) << 4) | (hund % 10);

    //convert minBCD to ASCII and add to ptr --> tens then units
    resultPtr[0] = ((minBCD >> 4) & 0x0F) + '0';
    resultPtr[1] = (minBCD & 0x0F) + '0';        

    // : separator into ptr
    resultPtr[2] = ':';

    //convert secBCD to ASCII and add to ptr --> tens then units
    resultPtr[3] = ((secBCD >> 4) & 0x0F) + '0';
    resultPtr[4] = (secBCD & 0x0F) + '0';

    // . separator into ptr
    resultPtr[5] = '.';

    //convert hundBCD to ASCII and add to ptr --> tens then units
    resultPtr[6] = ((hundBCD >> 4) & 0x0F) + '0';
    resultPtr[7] = (hundBCD & 0x0F) + '0';

    //null terminator for end of ptr
    resultPtr[8] = '\0';
}
//********************************************************************
// END OF PROGRAM
//********************************************************************
