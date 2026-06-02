#include "debug.h"
#include "protocol.h"
#include "ch32v00x_iwdg.h"

#define watchdog_feed() IWDG_ReloadCounter()

static void IWDG_Init_Config(void)
{
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler(IWDG_Prescaler_32);
    IWDG_SetReload(1250); // ~1 second timeout
    IWDG_ReloadCounter();
    IWDG_Enable();
}

static void rf_tx_send_byte(uint8_t byte)
{
    for (int i = 7; i >= 0; i--)
    {
        uint8_t bit = (byte >> i) & 1;
        GPIO_WriteBit(GPIOC, GPIO_Pin_1, bit ? Bit_SET : Bit_RESET);
        Delay_Us(bit ? T_LONG_US : T_SHORT_US);
        GPIO_WriteBit(GPIOC, GPIO_Pin_1, Bit_RESET);
        Delay_Us(T_SHORT_US);
    }
}

static void rf_tx_send_packet(uint8_t address, uint8_t buttons)
{
    uint8_t checksum = RF_PREAMBLE ^ address ^ buttons;
    rf_tx_send_byte(RF_TRAINING);
    rf_tx_send_byte(RF_PREAMBLE);
    rf_tx_send_byte(address);
    rf_tx_send_byte(buttons);
    rf_tx_send_byte(checksum);
}

int main(void)
{
    SystemCoreClockUpdate();
    Delay_Init();
    
    // Enable GPIO clock for GPIOC
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    // Initialize Watchdog Timer (IWDG)
    IWDG_Init_Config();
    
    // Configure buttons (PC4-PC7) as Input with Pull-Up
    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOC, &gpio_init);
    
    // Configure RF DATA pin (PC1) as Output Push-Pull
    gpio_init.GPIO_Pin = GPIO_Pin_1;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &gpio_init);
    
    GPIO_WriteBit(GPIOC, GPIO_Pin_1, Bit_RESET);
    
    uint8_t prev_buttons = 0;
    
    while (1)
    {
        watchdog_feed();
        
        // Read button state from PC4-PC7 (Active Low)
        uint8_t pin_state = (GPIO_ReadInputData(GPIOC) >> 4) & 0x0F;
        uint8_t buttons = (~pin_state) & 0x0F;
        
        if (buttons != 0)
        {
            // Button is pressed, transmit packet
            rf_tx_send_packet(RF_ADDRESS, buttons);
            Delay_Ms(25);
        }
        else if (prev_buttons != 0)
        {
            // Just released: transmit 3 release packets
            for (int i = 0; i < 3; i++)
            {
                rf_tx_send_packet(RF_ADDRESS, 0x00);
                Delay_Ms(25);
            }
        }
        else
        {
            // Idle: yield slightly to prevent continuous high frequency loop cycles
            Delay_Ms(10);
        }
        
        prev_buttons = buttons;
    }
}
