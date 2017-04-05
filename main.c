#include "stm32f030.h"

#include "util.h"

void timer17SetupToggleOutput(unsigned short prescale, unsigned short timeout) {
    REG_L(GPIOA_BASE, GPIO_MODER) &= ~(3 << (7 * 2));
    REG_L(GPIOA_BASE, GPIO_MODER) |= (2 << (7 * 2)); // PA6 alternate function (TIM17_CH1)
    REG_L(GPIOA_BASE, GPIO_AFRL) &= ~(0xF << (7 * 4));
    REG_L(GPIOA_BASE, GPIO_AFRL) |= (5 << (7 * 4)); // PA6 alternate function 5
    REG_L(RCC_BASE, RCC_APB2ENR) |= (1 << 18); // timer 16 enabled
    REG_S(TIM17_BASE, TIM_PSC) = prescale - 1; // divide by prescaler
    REG_S(TIM17_BASE, TIM_ARR) = timeout - 1;
    REG_S(TIM17_BASE, TIM_CCMR1) = 0x30; // toggle output compare
    REG_S(TIM17_BASE, TIM_CCER) = 1; // output compare enable
    REG_S(TIM17_BASE, TIM_CNT) = 0;
    REG_S(TIM17_BASE, TIM_CCR1) = timeout - 1;
    REG_S(TIM17_BASE, TIM_BDTR) = 0xC000; // disable output override
    REG_S(TIM17_BASE, TIM_CR1) = 1; // start
}

void timer3SetupCountInput(unsigned short prescale) {
    
    REG_L(GPIOA_BASE, GPIO_MODER) &= ~(3 << (6 * 2));
    REG_L(GPIOA_BASE, GPIO_MODER) |= (2 << (6 * 2)); // PA4 alternate function (TIM3_CH1)
    REG_L(GPIOA_BASE, GPIO_AFRL) &= ~(0xF << (6 * 4));
    REG_L(GPIOA_BASE, GPIO_AFRL) |= (1 << (6 * 4)); // PA4 alternate function 1
    
    REG_L(RCC_BASE, RCC_APB1ENR) |= (1 << 1); // timer 3 enabled
    REG_S(TIM3_BASE, TIM_CCMR1) = 1; // input pin enabled
    REG_S(TIM3_BASE, TIM_SMCR) = 0x57; // clock from input pin
    REG_S(TIM3_BASE, TIM_PSC) = prescale - 1; // divide by prescaler
    REG_S(TIM3_BASE, TIM_ARR) = 0xFFFF;
    REG_S(TIM3_BASE, TIM_CNT) = 0;
    REG_S(TIM3_BASE, TIM_CR1) = 1; // start
}

void timer3Restart(unsigned short prescale) {
    REG_S(TIM3_BASE, TIM_CR1) = 0;
    REG_S(TIM3_BASE, TIM_PSC) = prescale - 1;
    REG_S(TIM3_BASE, TIM_CNT) = 0;
    REG_S(TIM3_BASE, TIM_EGR) = 1;
    REG_S(TIM3_BASE, TIM_CR1) = 1;
}

void pinModeOutputA(int i) {
    REG_L(GPIOA_BASE, GPIO_MODER) |= (1 << (i * 2));
}

void pinOutputA(int i, char v) {
    if (v == 0) {
        i += 16;
    }
    REG_L(GPIOA_BASE, GPIO_BSRR) = (1 << i);
}

void pinModeInputA(int i) {
    REG_L(GPIOA_BASE, GPIO_MODER) &= ~(3 << (i * 2));
}

char pinInputA(int i) {
    return (char) ((REG_L(GPIOA_BASE, GPIO_IDR) >> i) & 1);
}

void twoWireClk(char v) {
    pinOutputA(1, v);
}

void twoWireData(char v) {
    pinOutputA(2, v);
}

void twoWireAsOutput() {
    pinModeOutputA(2);
}

void twoWireAsInput() {
    pinModeInputA(2);
}

char twoWireRead() {
    return pinInputA(2);
}

void twoWireDelay() {
    int n = 500; while (n--);
}

void twoWireStart() {
    twoWireClk(1);
    twoWireDelay();
    twoWireData(1);
    twoWireDelay();
    twoWireData(0);
    twoWireDelay();
    twoWireClk(0);
    twoWireDelay();
}

void twoWireStop() {
    twoWireData(0);
    twoWireDelay();
    twoWireClk(1);
    twoWireDelay();
    twoWireData(1);
    twoWireDelay();
    twoWireRead();
    twoWireDelay();
}

int twoWireWriteByte(unsigned char b) {
    int i;
    for (i = 0; i < 8; i++) {
        twoWireData((b & 0x80) ? 1 : 0);
        b <<= 1;
        twoWireDelay();
        twoWireClk(1);
        twoWireDelay();
        twoWireClk(0);
    }
    twoWireAsInput();
    twoWireDelay();
    twoWireClk(1);
    twoWireDelay();
    i = twoWireRead();
    twoWireClk(0);
    twoWireDelay();
    twoWireAsOutput();
    return i;
}

unsigned char twoWireReadByte() {
    int i;
    unsigned char b = 0;
    twoWireAsInput();
    for (i = 0; i < 8; i++) {
        b <<= 1;
        twoWireDelay();
        twoWireClk(1);
        twoWireDelay();
        b |= twoWireRead();
        twoWireClk(0);
    }
    twoWireAsOutput();
    twoWireData(1);
    twoWireDelay();
    twoWireClk(1);
    twoWireDelay();
    twoWireClk(0);
    return b;
}

int cameraReadByte(unsigned char addr) {
    unsigned char res;
    twoWireStart();
    twoWireWriteByte(0x42);
    twoWireWriteByte(addr);
    twoWireStop();
    twoWireStart();
    twoWireWriteByte(0x43);
    res = twoWireReadByte();
    twoWireStop();
    return res;
}

int cameraWriteByte(unsigned char addr, unsigned char v) {
    unsigned char res;
    int a = 0;
    twoWireStart();
    a |= twoWireWriteByte(0x42);
    a |= (twoWireWriteByte(addr) << 1);
    a |= (twoWireWriteByte(v) << 2);
    twoWireStop();
}

int main() {
    setupPll(48);
    int n, i;
    REG_L(RCC_BASE, RCC_AHBENR) |= (1 << 17);
    
    pinModeOutputA(0);
    pinModeOutputA(1);
    pinOutputA(1, 1);
    twoWireAsOutput();
    REG_L(GPIOA_BASE, GPIO_PUPDR) |= (1 << 2 * 2);
    twoWireDelay();
    
    uartEnable(48000000 / 921600);
    uartSends("Started...\n");
    
    
    timer17SetupToggleOutput(1, 3);
    int mul = 1;
    timer3SetupCountInput(mul);
    
    i = 0;
    
    cameraWriteByte(0x11, 0x00);
    cameraWriteByte(0x12, 0x08);
    for (n = 0; n < 0x20; n++) {
        uartSendHex(n, 2);
        uartSends(": ");
        uartSendHex(cameraReadByte(n), 2);
        if (n % 4 == 3) {
            uartSends("\n");
        } else {
            uartSends("   ");
        }
    }
    
    while(1) {
        pinOutputA(0, 1);
        n=1364000; while(--n);
        pinOutputA(0, 0);
        n=3000000; while(--n);
        n = REG_S(TIM3_BASE, TIM_CNT);
        uartSendDec(n);
        uartSend('*');
        uartSendDec(mul);
        uartSends("\n");
        i += 1;
        mul *= 10;
        if (mul > 1000) {
            mul = 1;
        }
        timer3Restart(mul);
    }    
}

