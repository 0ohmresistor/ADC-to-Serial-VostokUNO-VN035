#include "retarget_conf.h"

// Эта программа задаёт тактирование для АЦП, затем выполняет оцифровку 0-го канала
// Оцифрованные даныне отправляются в UART

void reportRCUState(); 
void configPLL();
void printPLLConfig();
void adcInit();
void adcInitWithTimer();
void adcInitCyclicWithTimer();
void periphInit();

int main()
{
    configPLL();
    periphInit();
    adcInitCyclicWithTimer();
    printPLLConfig();
    printf("All periph inited\n");

    return 0;
}

// Первичная инициализация
void periphInit()
{
    SystemCoreClockUpdate(); // Библиотечаня функция
    retarget_init(); // printf будет отправлять информацию в UART
}

// Конфигурация коэффициентов тактирования дял ФАПЧ (раздел 4.2, формула 5.1)
// При данных значениях, частота составит 100 МГц, при внутреннем тактовом сигнале 16 МГц
void configPLL()
{
  RCU->PLLCFG_bit.REFSRC = 0; // Внутренний/внешний входной сигнал, т.е. с выводов XI_OSC, XO_OSC
  RCU->PLLCFG_bit.OD = 1; 
  // RCU->PLLCFG_bit.M = 25; // Значение по умолчанию 25
  // RCU->PLLCFG_bit.N = 1; // Значение по умолчанию 25
}

// Инициализация АЦП. Оцифровка будет проводиться циклически через определённые интервалы
void adcInitCyclicWithTimer()
{
    RCU->ADCCFG_bit.CLKSEL = 1; // Выходная частота ФАПЧ в качестве тактирования
    RCU->ADCCFG_bit.DIVN = 1; // делитель входной тактовой чатоты = 4
    RCU->ADCCFG_bit.DIVEN = 1; // включение делителя входной тактовой частоты
    RCU->ADCCFG_bit.CLKEN = 1; // включение тактирования АЦП
    RCU->ADCCFG_bit.RSTDIS = 1; // снятие сброса модуля

    ADC->ACTL_bit.ADCEN = 1; // включение модуля
    ADC->CHCTL[2].CHCTL_bit.GAINTRIM = 5; // определяется из п. 19. документации
    ADC->CHCTL[2].CHCTL_bit.OFFTRIM = (uint32_t)(-5); // аналогично GAINTRIM 
    ADC->EMUX_bit.EM0 = 0xF; // 0xF так как заупуск циклический см. табл. А.1.11
    ADC->SEQ[0].SCCTL_bit.ICNT = 0; // количество запросов для прерывания, если 0, то прерывание по каждому запросу АЦП
    ADC->SEQ[0].SRTMR = 24; // частота срабатывания таймера X(нс)/40(нс) - 1
    ADC->SEQ[0].SRQCTL_bit.RQMAX = 0; // глубина очереди измерений, если не планируется усреднение, то оставить 0
    ADC->SEQ[0].SRQCTL_bit.QAVGVAL = ADC_SEQ_SRQCTL_QAVGVAL_Disable; // выключение усреднения (т.е. коэф. 1)
    ADC->SEQ[0].SRQCTL_bit.QAVGEN = 0; // тоже выключение усреднения
    ADC->SEQ[0].SRQSEL_bit.RQ0 = 2; // канал АЦП для измерения
    ADC->SEQEN_bit.SEQEN0 = 1; // разрешение работы секвеносра

    ADC->IM_bit.SEQIM0 = 1; // разрешение генерации прерываний
    NVIC_EnableIRQ(ADC_SEQ0_IRQn); // включение прерывания от АЦП

    // Запуск
    while(!ADC->ACTL_bit.ADCRDY); // ожидание установки бита готовности АЦП
    ADC->SEQSYNC_bit.SYNC0 = 1; // запуск измерений
    ADC->SEQSYNC_bit.GSYNC = 1; // запуск измерений
}

// Вывод значений регистров конфигурации ФАПЧ в Serial Port
void printPLLConfig()
{
    printf("REFSRC = %d \n", RCU->PLLCFG_bit.REFSRC);
    printf("OD = %d \n", RCU->PLLCFG_bit.OD);
    printf("M = %d \n", RCU->PLLCFG_bit.M);
    printf("N = %d \n", RCU->PLLCFG_bit.N);
    printf("DIVN = %d \n", RCU->ADCCFG_bit.DIVN);
    printf("DIVEN = %d \n",RCU->ADCCFG_bit.DIVEN);
    printf("CLKSEL = %d \n",RCU->ADCCFG_bit.CLKSEL);
}

// Обработчик прерываний от АЦП
void ADC_SEQ0_IRQHandler()
{
    uint32_t result;

    ADC->IC_bit.SEQIC0 = 1; // сбрасываем статус прерывания

    while (ADC->SEQ[0].SFLOAD) 
    {
        result = ADC->SEQ[0].SFIFO; // записываем результат измерений из FIFO
    }

    printf("%d\n", result); // Записываем результат оцифровки в Serial Port
}
