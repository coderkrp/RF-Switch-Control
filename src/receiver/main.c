#include "debug.h"
#include "protocol.h"
#include "ch32v00x_iwdg.h"

#define watchdog_feed() IWDG_ReloadCounter()

/* SysTick-based Millisecond Counter */
volatile uint32_t ms_ticks = 0;

void SysTick_Config_1ms(void)
{
    // Configure SysTick to trigger every 1ms (assuming 48MHz system clock)
    SysTick->SR = 0;
    SysTick->CNT = 0;
    SysTick->CMP = 48000 - 1; // 48,000,000 Hz / 1000 = 48000 cycles
    
    // Enable timer, interrupt, auto-reload, HCLK source
    SysTick->CTLR = 0xF;
    
    NVIC_EnableIRQ(SysTick_IRQn);
}

__attribute__((interrupt("WCH-Interrupt-fast")))
void SysTick_Handler(void)
{
    ms_ticks++;
    SysTick->SR = 0; // Clear interrupt flag
}

uint32_t millis(void)
{
    return ms_ticks;
}

/* TIM2 Configuration for 1 microsecond resolution */
void TIM2_Init_1M(void)
{
    // Enable TIM2 clock
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    TIM_TimeBaseInitTypeDef tim_init = {0};
    tim_init.TIM_Period = 0xFFFF; // Auto-reload value (max 16-bit)
    tim_init.TIM_Prescaler = 47;  // 48MHz / 48 = 1MHz count rate
    tim_init.TIM_ClockDivision = TIM_CKD_DIV1;
    tim_init.TIM_CounterMode = TIM_CounterMode_Up;
    
    TIM_TimeBaseInit(TIM2, &tim_init);
    TIM_Cmd(TIM2, ENABLE);
}

/* GPIO Port and Pins Initialization */
void GPIO_Init_Rx(void)
{
    // Enable clocks for GPIOC and GPIOD
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
    
    GPIO_InitTypeDef gpio_init = {0};
    
    // Configure LEDs on PC4-PC7 as Output Push-Pull
    gpio_init.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &gpio_init);
    
    // Configure RF DATA pin on PC1 as Input Floating (actively driven by RF receiver)
    gpio_init.GPIO_Pin = GPIO_Pin_1;
    gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOC, &gpio_init);
    
    // Configure unused GPIOD pins as Input Pull-Up to stabilize them (PD0, PD2-PD6)
    gpio_init.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOD, &gpio_init);
    
    // Turn off all LEDs initially
    GPIO_WriteBit(GPIOC, GPIO_Pin_4, Bit_RESET);
    GPIO_WriteBit(GPIOC, GPIO_Pin_5, Bit_RESET);
    GPIO_WriteBit(GPIOC, GPIO_Pin_6, Bit_RESET);
    GPIO_WriteBit(GPIOC, GPIO_Pin_7, Bit_RESET);
}

typedef enum {
    STATE_SEARCH_PREAMBLE,
    STATE_RECEIVE_DATA
} rx_state_t;

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

int main(void)
{
    // Standard system clock update and timers init
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    
    // Initialize our timers and GPIOs
    SysTick_Config_1ms();
    TIM2_Init_1M();
    GPIO_Init_Rx();
    
    // Initialize Watchdog Timer (IWDG)
    IWDG_Init_Config();
    
    uint16_t last_transition_time = TIM2->CNT;
    uint16_t high_duration = 0;
    uint16_t low_duration = 0;
    uint8_t last_pin_state = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1);
    
    uint32_t rx_shift_reg = 0;
    uint8_t rx_bit_count = 0;
    uint16_t last_bit_time = TIM2->CNT;
    
    rx_state_t rx_state = STATE_SEARCH_PREAMBLE;
    uint32_t last_packet_time = 0;
    static uint32_t valid_packets = 0; // Debug packet counter
    
    while (1)
    {
        // Feed the watchdog at the start of each iteration
        watchdog_feed();

        // 1. Read RF DATA pin (PC1)
        uint8_t pin_state = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1);
        
        // Check for edge transition
        if (pin_state != last_pin_state)
        {
            uint16_t current_time = TIM2->CNT;
            uint16_t duration = (uint16_t)(current_time - last_transition_time);
            last_transition_time = current_time;
            
            if (last_pin_state == Bit_SET)
            {
                // Falling Edge: This duration was the HIGH pulse width
                high_duration = duration;
            }
            else
            {
                // Rising Edge: This duration was the LOW pulse width
                low_duration = duration;
                
                // Now we have a complete pulse cycle (HIGH + LOW) to decode
                // Verify the LOW pulse width is within tolerance
                if (low_duration >= LOW_MIN_US && low_duration <= LOW_MAX_US)
                {
                    int8_t bit = -1;
                    
                    // Classify bit based on the HIGH pulse duration
                    if (high_duration >= HIGH_0_MIN_US && high_duration <= HIGH_0_MAX_US)
                    {
                        bit = 0;
                    }
                    else if (high_duration >= HIGH_1_MIN_US && high_duration <= HIGH_1_MAX_US)
                    {
                        bit = 1;
                    }
                    
                    if (bit >= 0)
                    {
                        // Check if the delay since the last valid bit exceeds the timeout (2.5 ms)
                        uint16_t time_since_last_bit = (uint16_t)(current_time - last_bit_time);
                        if (time_since_last_bit > 2500)
                        {
                            // Reset bit accumulation
                            rx_bit_count = 0;
                            rx_shift_reg = 0;
                            rx_state = STATE_SEARCH_PREAMBLE;
                        }
                        last_bit_time = current_time;
                        
                        rx_shift_reg = (rx_shift_reg << 1) | bit;
                        rx_bit_count++;
                        
                        if (rx_state == STATE_SEARCH_PREAMBLE)
                        {
                            // Check if the lower 8 bits of shift register match the Preamble
                            if ((rx_shift_reg & 0xFF) == RF_PREAMBLE)
                            {
                                rx_state = STATE_RECEIVE_DATA;
                                rx_bit_count = 8; // Preamble (8 bits) already matched
                            }
                        }
                        else if (rx_state == STATE_RECEIVE_DATA)
                        {
                            // We need 32 bits total (8 Preamble, 8 Address, 8 Buttons, 8 Checksum)
                            if (rx_bit_count == 32)
                            {
                                uint8_t preamble = (rx_shift_reg >> 24) & 0xFF;
                                uint8_t address  = (rx_shift_reg >> 16) & 0xFF;
                                uint8_t buttons  = (rx_shift_reg >> 8)  & 0xFF;
                                uint8_t checksum = rx_shift_reg & 0xFF;
                                
                                uint8_t calc_checksum = preamble ^ address ^ buttons;
                                
                                if (calc_checksum == checksum && address == RF_ADDRESS)
                                {
                                    valid_packets++;
                                    // Valid packet received! Update LED outputs on PC4-PC7
                                    GPIO_WriteBit(GPIOC, GPIO_Pin_4, (buttons & 0x01) ? Bit_SET : Bit_RESET);
                                    GPIO_WriteBit(GPIOC, GPIO_Pin_5, (buttons & 0x02) ? Bit_SET : Bit_RESET);
                                    GPIO_WriteBit(GPIOC, GPIO_Pin_6, (buttons & 0x04) ? Bit_SET : Bit_RESET);
                                    GPIO_WriteBit(GPIOC, GPIO_Pin_7, (buttons & 0x08) ? Bit_SET : Bit_RESET);
                                    
                                    last_packet_time = millis();
                                }
                                
                                // Reset state machine for next packet
                                rx_state = STATE_SEARCH_PREAMBLE;
                                rx_bit_count = 0;
                                rx_shift_reg = 0;
                            }
                        }
                    }
                    else
                    {
                        // Invalid HIGH duration -> reset
                        rx_state = STATE_SEARCH_PREAMBLE;
                        rx_bit_count = 0;
                        rx_shift_reg = 0;
                    }
                }
                else
                {
                    // Invalid LOW duration -> reset
                    rx_state = STATE_SEARCH_PREAMBLE;
                    rx_bit_count = 0;
                    rx_shift_reg = 0;
                }
            }
            
            last_pin_state = pin_state;
        }
        
        // 2. Perform periodic 100ms packet timeout check
        if ((millis() - last_packet_time) > 100)
        {
            // Turn off all LEDs if no packet received within 100ms
            GPIO_WriteBit(GPIOC, GPIO_Pin_4, Bit_RESET);
            GPIO_WriteBit(GPIOC, GPIO_Pin_5, Bit_RESET);
            GPIO_WriteBit(GPIOC, GPIO_Pin_6, Bit_RESET);
            GPIO_WriteBit(GPIOC, GPIO_Pin_7, Bit_RESET);
        }
    }
}
