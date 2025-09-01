#include "../system/include/cmsis/stm32f4xx.h"

#include <string.h>

class MotorFather;

class M1;

extern MotorFather *motors_[16];

void GPIO_init()
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    GPIOA->MODER &= ~GPIO_MODER_MODER2;
    GPIOA->MODER |= GPIO_MODER_MODER2_1;
    GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR2;
    GPIOA->AFR[0] |= (0x7 << 8);

    GPIOA->MODER &= ~GPIO_MODER_MODER3;
    GPIOA->MODER |= GPIO_MODER_MODER3_1;
    GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR3;
    GPIOA->AFR[0] |= (0x7 << 12);

    GPIOA->MODER &= ~GPIO_MODER_MODER9;
    GPIOA->MODER |= GPIO_MODER_MODER9_1;
    GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR9;
    GPIOA->AFR[1] |= (0x7 << 4);

    GPIOA->MODER &= ~GPIO_MODER_MODER10;
    GPIOA->MODER |= GPIO_MODER_MODER10_1;
    GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR10;
    GPIOA->AFR[1] |= (0x7 << 8);
}

void USART_init()
{
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    USART1->CR1 = 0; // Сбрасываем все биты
    USART1->CR2 = 0; // 1 стоп-бит
    USART1->CR3 = 0;

    uint32_t f_CK = SystemCoreClock; // Тактовая частота USART1 (16 МГц, HSI)
    uint32_t baud = 115200;
    uint32_t usartdiv = (f_CK + (baud / 2)) / (16 * baud); // Округление
    USART1->BRR = (usartdiv / 16) << 4 | (usartdiv % 16);  // BRR = Mantissa | Fraction

    USART1->CR1 |= (USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE);
    USART1->CR3 |= (USART_CR3_DMAR | USART_CR3_DMAT);
    USART1->CR1 |= USART_CR1_UE;

    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    USART2->BRR = 0x8B; // 115200
    USART2->CR1 |= (USART_CR1_TE);
    USART2->CR3 |= (USART_CR3_DMAT);
    USART2->CR1 |= USART_CR1_UE;
}

uint8_t usart1_mrk = 0x00;
uint8_t usart1_rx_array[256];
uint8_t usart1_tx_array[256];

uint8_t usart2_mrk = 0xFF;

extern "C" void __attribute__((interrupt, used)) DMA1_Stream6_IRQHandler(void) // TX
{
    if ((DMA1->HISR & DMA_HISR_TCIF6) == DMA_HISR_TCIF6)
    {
        usart2_mrk = 0xFF;
        DMA1_Stream6->CR &= ~DMA_SxCR_EN;
        while ((DMA1_Stream6->CR & DMA_SxCR_EN) == DMA_SxCR_EN)
            ;
        DMA1->HIFCR |= DMA_HIFCR_CTCIF6;
    }
}

extern "C" void __attribute__((interrupt, used)) DMA2_Stream7_IRQHandler(void) // TX
{
    if ((DMA2->HISR & DMA_HISR_TCIF7) == DMA_HISR_TCIF7)
    {
        usart1_mrk = 0xA0;
        DMA2_Stream7->CR &= ~DMA_SxCR_EN;
        while ((DMA2_Stream7->CR & DMA_SxCR_EN) == DMA_SxCR_EN)
            ;
        DMA2->HIFCR |= DMA_HIFCR_CTCIF7;
    }
}

extern "C" void __attribute__((interrupt, used)) DMA2_Stream5_IRQHandler(void) // RX
{
    if ((DMA2->HISR & DMA_HISR_TCIF5) == DMA_HISR_TCIF5)
    {
        usart1_mrk = 0x0A;
        DMA2_Stream5->CR &= ~DMA_SxCR_EN;
        while ((DMA2_Stream5->CR & DMA_SxCR_EN) == DMA_SxCR_EN)
            ;
        DMA2->HIFCR |= DMA_HIFCR_CTCIF5;
    }
}

void DMA_init()
{
    // DMA1_Stream6 this USART2_TX
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    DMA1_Stream6->CR &= ~DMA_SxCR_EN;
    while ((DMA1_Stream6->CR & DMA_SxCR_EN) == DMA_SxCR_EN)
        ;

    // - (0x4 << 25) - 4-ый канал
    // - DMA_SxCR_MINC - увеличенный объем памяти
    // - DMA_SxCR_TCIE - прерывания по приему/передачи
    // - DMA_SxCR_CIRC (for rx) - циклическая работа
    DMA1_Stream6->CR |= ((0x4 << 25) | DMA_SxCR_MINC | DMA_SxCR_TCIE);
    DMA1_Stream6->CR &= ~(DMA_SxCR_MSIZE | DMA_SxCR_PSIZE);
    DMA1_Stream6->CR |= (0x01 << 6); // Из памяти в перефирию
    DMA1_Stream6->NDTR = 12;
    DMA1_Stream6->PAR = (uint32_t)(&USART2->DR);
    // DMA1_Stream6->M0AR = (uint32_t) &usart2_tx_array[0];
    NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    NVIC_SetPriority(DMA1_Stream6_IRQn, 7);

    // DMA2_Stream7 this USART1_TX
    // DMA2_Stream5 this USART1_RX
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    DMA2_Stream7->CR &= ~DMA_SxCR_EN;
    while ((DMA2_Stream7->CR & DMA_SxCR_EN) == DMA_SxCR_EN)
        ;

    DMA2_Stream5->CR &= ~DMA_SxCR_EN;
    while ((DMA2_Stream5->CR & DMA_SxCR_EN) == DMA_SxCR_EN)
        ;

    // - (0x4 << 25) - 4-ый канал
    // - DMA_SxCR_MINC - увеличенный объем памяти
    // - DMA_SxCR_TCIE - прерывания по приему/передачи
    // - DMA_SxCR_CIRC (for rx) - циклическая работа
    DMA2_Stream7->CR |= ((0x4 << 25) | DMA_SxCR_MINC | DMA_SxCR_TCIE);
    DMA2_Stream5->CR |= ((0x4 << 25) | DMA_SxCR_MINC | DMA_SxCR_TCIE | DMA_SxCR_CIRC);
    DMA2_Stream7->CR &= ~(DMA_SxCR_MSIZE | DMA_SxCR_PSIZE);
    DMA2_Stream5->CR &= ~(DMA_SxCR_MSIZE | DMA_SxCR_PSIZE);
    DMA2_Stream7->CR |= (0x01 << 6); // Из памяти в перефирию
    DMA2_Stream5->CR &= ~(3UL << 6); // Из переферии в память
    DMA2_Stream7->NDTR = 256;
    DMA2_Stream5->NDTR = 256;
    DMA2_Stream7->PAR = (uint32_t)(&USART1->DR);
    DMA2_Stream5->PAR = (uint32_t)(&USART1->DR);
    DMA2_Stream7->M0AR = (uint32_t)&usart1_tx_array[0];
    DMA2_Stream5->M0AR = (uint32_t)&usart1_rx_array[0];
    NVIC_EnableIRQ(DMA2_Stream7_IRQn);
    NVIC_SetPriority(DMA2_Stream7_IRQn, 6);
    NVIC_EnableIRQ(DMA2_Stream5_IRQn);
    NVIC_SetPriority(DMA2_Stream5_IRQn, 5);

    DMA2_Stream5->CR |= DMA_SxCR_EN;
    while ((DMA2_Stream5->CR & DMA_SxCR_EN) != DMA_SxCR_EN)
        ;
}

void send2driver(const uint8_t *frame)
{
    while (usart2_mrk == 0x00)
        ;
    usart2_mrk = 0x00;
    DMA1_Stream6->M0AR = (uint32_t)frame;
    DMA1_Stream6->NDTR = 12;
    DMA1_Stream6->CR |= DMA_SxCR_EN;
}

void SystemClock_HSI_Config(void)
{
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY))
        ;

    RCC->CFGR &= ~RCC_CFGR_HPRE;
    RCC->CFGR &= ~RCC_CFGR_PPRE1;
    RCC->CFGR &= ~RCC_CFGR_PPRE2;

    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_HSI;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI)
        ;
    SystemCoreClockUpdate();
}

class MotorFather
{
public:
    MotorFather() = default;

    virtual void init() = 0;
    virtual void set(const uint8_t *frame) = 0;
    virtual void launch() = 0;
    virtual void setLaunch(const uint8_t *frame) = 0;
    virtual void reset()
    {
        res = false;
    }

protected:
    bool res = false;
};

uint8_t m1_status = 0x00;
class M1 : public MotorFather
{
public:
    void init()
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;

        GPIOD->MODER &= ~(GPIO_MODER_MODER12 | GPIO_MODER_MODER13 | GPIO_MODER_MODER15);      // Сбрасываем биты
        GPIOD->MODER |= (GPIO_MODER_MODER12_0 | GPIO_MODER_MODER13_0 | GPIO_MODER_MODER15_0); // Устанавливаем как выход (01)

        GPIOD->OTYPER &= ~(GPIO_OTYPER_OT_12 | GPIO_OTYPER_OT_13 | GPIO_OTYPER_OT_15); // Push-pull (0)

        GPIOD->OSPEEDR |= (GPIO_OSPEEDER_OSPEEDR12 | GPIO_OSPEEDER_OSPEEDR13 | GPIO_OSPEEDER_OSPEEDR15); // Максимальная скорость (11)

        GPIOD->PUPDR &= ~(GPIO_PUPDR_PUPDR12 | GPIO_PUPDR_PUPDR13 | GPIO_PUPDR_PUPDR15);      // Сбрасываем биты
        GPIOD->PUPDR |= (GPIO_PUPDR_PUPDR12_1 | GPIO_PUPDR_PUPDR13_1 | GPIO_PUPDR_PUPDR15_1); // Pull-down (10)

        GPIOD->MODER &= ~GPIO_MODER_MODER11; // Вход (00)
        RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
        SYSCFG->EXTICR[2] &= ~SYSCFG_EXTICR3_EXTI11;   // Сбрасываем
        SYSCFG->EXTICR[2] |= SYSCFG_EXTICR3_EXTI11_PD; // Выбираем PD11
        EXTI->FTSR |= EXTI_FTSR_TR11;                  // Прерывание по заднему фронту
        // EXTI->RTSR |= EXTI_RTSR_TR11;                  // Прерывание по переднему фронту
        EXTI->IMR |= EXTI_IMR_MR11;

        NVIC_SetPriority(EXTI15_10_IRQn, 1); // Приоритет 0 (высший)
        NVIC_EnableIRQ(EXTI15_10_IRQn);      // Включаем прерывание

        GPIOD->ODR |= GPIO_ODR_OD13;
    }

    void set(const uint8_t *frame)
    {
        if (res)
            return;

        GPIOD->ODR |= (GPIO_ODR_OD12 | GPIO_ODR_OD15);
        send2driver(frame);
        while (m1_status == 0x00)
            ;
        GPIOD->ODR &= ~(GPIO_ODR_OD15);
    }

    void launch()
    {
        if (res)
            return;

        while (m1_status == 0x00)
            ;
        m1_status = 0x00;
        GPIOD->ODR &= ~(GPIO_ODR_OD12);
        res = true;
    }

    void setLaunch(const uint8_t *frame)
    {
        set(frame);
        launch();
    }
};

MotorFather *motors_[16] = {
    nullptr,  // 0
    new M1(), // 1
    nullptr,  // 2
    nullptr,  // 3
    nullptr,  // 4
    nullptr,  // 5
    nullptr,  // 6
    nullptr,  // 7
    nullptr,  // 8
    nullptr,  // 9
    nullptr,  // 10
    nullptr,  // 11
    nullptr,  // 12
    nullptr,  // 13
    nullptr,  // 14
    nullptr   // 15
};

extern "C" void __attribute__((interrupt, used)) EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR11)
    {
        m1_status = 0xFF;
        EXTI->PR = EXTI_PR_PR11; // Сбрасываем флаг прерывания
    }
}

constexpr int PacketSize = 16;
constexpr int TotalPackets = 16;

void clearBits(const int i, const int packet, uint8_t *data)
{
    if (packet != i)
    {
        uint8_t *buffer = (data + (i * PacketSize));
        *(buffer + 0) = 0x00;
        *(buffer + 1) = 0x00;
        *(buffer + 2) = 0x00;
        *(buffer + 3) = 0x00;
    }
}

// Настроить сразу все указанные двигатели
void configureAllEngines(const uint16_t motors, const int packet, uint8_t *data)
{
    for (int i = 0; i < 16; i++)
    {
        if (motors & (1 << (15 - i)))
        {
            clearBits(i, packet, data);

            if (motors_[i] != nullptr)
                motors_[i]->set((data + (i * PacketSize) + 4));
        }
    }

    for (int i = 0; i < 16; i++)
    {
        if ((motors & (1 << (15 - i))) and (motors_[i] != nullptr))
            motors_[i]->launch();
    }
}

// Двигатели нужно запустить одновременно, но с индивидуальными настройками
// Находим все включенные двигатели
void startSimultaneously(const uint16_t motors, const int packet, uint8_t *data)
{
    for (int i = 0; i < 16; i++)
    {
        if (motors & (1 << (15 - i)))
        {
            clearBits(i, packet, data);

            if (motors_[i] != nullptr)
                motors_[i]->set((data + (i * PacketSize) + 4));
        }
    }

    for (int i = 0; i < 16; i++)
    {
        if ((motors & (1 << (15 - i))) and (motors_[i] != nullptr))
            motors_[i]->launch();
    }
}

// Запустить двигатели немедленно
void startImmediately(const uint16_t motors, const int packet, uint8_t *data)
{
    for (int i = 0; i < 16; i++)
    {
        if (motors & (1 << (15 - i)))
        {
            clearBits(i, packet, data);

            if (motors_[i] != nullptr)
                motors_[i]->setLaunch((data + (i * PacketSize) + 4));
        }
    }
}

void loop()
{
    uint8_t *data = &usart1_rx_array[0];

    for (int i = 0; i < 16; i++)
    {
        if (motors_[i] != nullptr)
            motors_[i]->reset();
    }

    // Каждая пачка состоит из 16 байт (4 байта конфигурации + 12 байт настроек)
    for (int packet = 0; packet < TotalPackets; ++packet)
    {
        const uint8_t *packetData = data + (packet * PacketSize);

        // Читаем конфигурационные биты (первые 4 байта)
        uint16_t motors = (((uint16_t)packetData[0] << 8) | packetData[1]);
        uint8_t flags = packetData[2];

        // Если все флаги (биты 1,2,3) равны 0 - пропускаем пачку
        if ((flags & 0xE0) == 0)
        {
            GPIOD->ODR ^= GPIO_ODR_OD14;
            continue;
        }

        bool configureAll = ((flags & 0x20) != 0);
        bool simultaneousWithSettings = ((flags & 0x40) != 0);

        if (configureAll)
            configureAllEngines(motors, packet, data);
        else if (simultaneousWithSettings)
            startSimultaneously(motors, packet, data);
        else
            startImmediately(motors, packet, data);
    }
}

void swap(uint8_t &A, uint8_t &B)
{
    uint8_t tmp = A;
    A = B;
    B = tmp;
}

int main(void)
{
    SystemClock_HSI_Config();
    GPIO_init();
    USART_init();
    DMA_init();

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    GPIOD->MODER |= GPIO_MODER_MODER14_0;

    for (int i = 0; i < TotalPackets; ++i)
    {
        if (motors_[i] != nullptr)
            motors_[i]->init();
    }

    GPIOD->ODR |= GPIO_ODR_OD14;

    while (1)
    {
        if (usart1_mrk)
        {
            usart1_mrk = 0x00;
            GPIOD->ODR &= ~GPIO_ODR_OD14;
            DMA2_Stream5->CR &= ~DMA_SxCR_EN;
            while ((DMA2_Stream5->CR & DMA_SxCR_EN) == DMA_SxCR_EN)
                ;
            DMA2_Stream5->CR |= (DMA_SxCR_CIRC);

            for (int i = 0; i < 256; i += 4)
            {
                swap(usart1_rx_array[i], usart1_rx_array[i + 3]);
                swap(usart1_rx_array[i + 1], usart1_rx_array[i + 2]);
            }
            loop();

            DMA2_Stream5->NDTR = 256;
            DMA2_Stream5->CR |= DMA_SxCR_EN;

            // DMA2_Stream7->NDTR = 256;
            // DMA2_Stream7->CR |= DMA_SxCR_EN;
        }
    }
}
