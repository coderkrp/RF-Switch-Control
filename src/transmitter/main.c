#include "debug.h"
#include "protocol.h"
#include "ch32v00x_iwdg.h"

#define watchdog_feed() IWDG_ReloadCounter()

/* Helper to configure all unused pins as Input Pull-Up to save power */
static void configure_unused_pins_pullup(void)
{
    GPIO_InitTypeDef gpio_init = {0};
    
    // Enable clocks for GPIO ports
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);
    
    // GPIOA unused pins: PA1, PA2
    gpio_init.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &gpio_init);
    
    // GPIOC unused pins: PC0, PC2, PC3 (PC1 is our RF TX pin, PC4-PC7 are buttons)
    gpio_init.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_3;
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOC, &gpio_init);
    
    // GPIOD unused pins: PD0, PD2-PD6 (PD1 is SWIO, PD7 is NRST)
    gpio_init.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOD, &gpio_init);
}

/* Initialize Independent Watchdog Timer */
static void IWDG_Init_Config(void)
{
    // Enable write access to IWDG_PR and IWDG_RLR registers
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    
    // Set IWDG prescaler to 32 (clock: LSI ~40kHz, 40kHz / 32 = 1.25kHz = 0.8ms per tick)
    IWDG_SetPrescaler(IWDG_Prescaler_32);
    
    // Set reload value to 1250 for a ~1-second timeout (1250 * 0.8ms = 1000ms)
    IWDG_SetReload(1250);
    
    // Reload IWDG counter before enabling
    IWDG_ReloadCounter();
    
    // Enable IWDG
    IWDG_Enable();
}

/* Transmit a single byte using software pulse-width modulation */
static void rf_tx_send_byte(uint8_t byte)
{
    for (int i = 7; i >= 0; i--)
    {
        uint8_t bit = (byte >> i) & 1;
        if (bit == 1)
        {
            // Logic 1: 1000us HIGH, 500us LOW
            GPIO_WriteBit(GPIOC, GPIO_Pin_1, Bit_SET);
            Delay_Us(T_LONG_US);
            GPIO_WriteBit(GPIOC, GPIO_Pin_1, Bit_RESET);
            Delay_Us(T_SHORT_US);
        }
        else
        {
            // Logic 0: 500us HIGH, 500us LOW
            GPIO_WriteBit(GPIOC, GPIO_Pin_1, Bit_SET);
            Delay_Us(T_SHORT_US);
            GPIO_WriteBit(GPIOC, GPIO_Pin_1, Bit_RESET);
            Delay_Us(T_SHORT_US);
        }
    }
}

/* Format and transmit an RF packet */
static void rf_tx_send_packet(uint8_t address, uint8_t buttons)
{
    uint8_t checksum = RF_PREAMBLE ^ address ^ buttons;
    
    // Send training byte (0x55) to settle receiver AGC
    rf_tx_send_byte(RF_TRAINING);
    
    // Send packet payload
    rf_tx_send_byte(RF_PREAMBLE);
    rf_tx_send_byte(address);
    rf_tx_send_byte(buttons);
    rf_tx_send_byte(checksum);
}

int main(void)
{
    // Safety delay to prevent debug lock-out:
    // If we woke up from Standby mode (signaled by the RCC Low-Power Reset flag),
    // skip the delay to minimize button press latency.
    // If not (e.g. power-on reset or programmer reset), delay 2 seconds to allow the debugger to connect.
    if (RCC_GetFlagStatus(RCC_FLAG_LPWRRST) == RESET)
    {
        Delay_Init();
        Delay_Ms(2000);
    }
    else
    {
        // System woke up from Standby, clear reset flags
        RCC_ClearFlag();
        Delay_Init();
    }
    
    // Enable Power interface clock
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    
    // Initialize Watchdog Timer (IWDG)
    IWDG_Init_Config();
    
    // Configure all unused pins as IPU for power savings
    configure_unused_pins_pullup();
    
    // Configure button pins (PC4 - PC7) as Input with Pull-Up
    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOC, &gpio_init);
    
    // Configure RF DATA pin (PC1) as Output Push-Pull
    gpio_init.GPIO_Pin = GPIO_Pin_1;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &gpio_init);
    
    // Ensure DATA line is low at start
    GPIO_WriteBit(GPIOC, GPIO_Pin_1, Bit_RESET);
    
    // Wait for the button pin state to settle (debounce initial press)
    Delay_Ms(5);
    
    while (1)
    {
        // Feed the watchdog at the start of each iteration
        watchdog_feed();

        // Buttons are active low. Read GPIOC pins 4-7, shift right by 4, and invert.
        uint8_t pin_state = (GPIO_ReadInputData(GPIOC) >> 4) & 0x0F;
        uint8_t buttons = (~pin_state) & 0x0F;
        
        if (buttons != 0)
        {
            // At least one button is pressed, transmit state
            rf_tx_send_packet(RF_ADDRESS, buttons);
            
            // Interval delay between transmissions (25ms nominal)
            Delay_Ms(25);
        }
        else
        {
            if (buttons == 0) { watchdog_feed(); }
            // All buttons are released:
            // Send release packet (bitmap = 0) 3 times to ensure the receiver clears outputs
            for (int i = 0; i < 3; i++)
            {
                rf_tx_send_packet(RF_ADDRESS, 0x00);
                Delay_Ms(25);
            }
            
            // Ensure DATA pin is low before entering Standby
            GPIO_WriteBit(GPIOC, GPIO_Pin_1, Bit_RESET);
            
            // Configure EXTI line 4 to 7 for wakeup events on falling edge (button press)
            GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource4);
            GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource5);
            GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource6);
            GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource7);
            
            EXTI_InitTypeDef exti_init = {0};
            exti_init.EXTI_Line = EXTI_Line4 | EXTI_Line5 | EXTI_Line6 | EXTI_Line7;
            exti_init.EXTI_Mode = EXTI_Mode_Event;
            exti_init.EXTI_Trigger = EXTI_Trigger_Falling;
            exti_init.EXTI_LineCmd = ENABLE;
            EXTI_Init(&exti_init);
            
            // Enter standby mode using Wait For Event (WFE)
            PWR_EnterSTANDBYMode(PWR_STANDBYEntry_WFE);
            
            // Standby mode stops clocks and resets on wake, so this point is never reached.
            while (1);
        }
    }
}
