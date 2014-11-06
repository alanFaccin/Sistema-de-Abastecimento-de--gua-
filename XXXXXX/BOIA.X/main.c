#include <stdio.h>
#include <stdlib.h>
//#include <hardware.h>
//#include <lcd_4bit.h>
#include "hardware.h"
#include "lcd_4bit.h"

#define BOTAO_LIGAR_DESLIGAR PORTCbits.RC3
#define RELEBOMBA   PORTDbits.RD2
#define LEDBOMBA    PORTDbits.RD3
#define RELEMOTOR   PORTDbits.RD4
#define SUPERIOR 0
#define INFERIOR 1

const char LIGAR_BOMBA = 'B';
const char DESLIGAR_BOMBA = 'b';
const char LIGAR_MOTOR = 'M';
const char DESLIGAR_MOTOR = 'm';
const char LIGAR = 'L';
const char DESLIGAR = 'l';

int ADCResult = 0; // variavel responsavel por armazenar o valor lido dos registradores ADCC
//int flag_an = -1; //
int flag_Start = 0; // variavel responsavel por controlar se deve ser iniciado a execução do sofrware
int recipiente = 0; // variavel responsavel por armazenar de qual recipiente está sendo lido a leitura do nivel d'agua
int countSuperior = 0; // variavel responsavel por armazenar o numero de amostras lidas para o recipiente superior
int countInferior = 0; // variavel responsavel por armazenar o numero de amostras lidas para o recipiente inferior
int nivelSuperior = 0; // variavel responsavel por armazenar os valores relativos ao nivel d'agua lido no recipiente superior
int nivelInferior = 0; // variavel responsavel por armazenar os valores relativos ao nivel d'agua lido no recipiente inferior

/**
 *  funcao responsavel por configurar a inicializar a comunicação serial do pic
 * @param BaudRate informa qual será a taxa de transferencia
 * @param Mode recebe o modo de transmissao
 */
void USARTInit(long BaudRate, int Mode) {
    int BR = 0;

    // Cálculo do valor para o registrador SPBRG para uma determinada velocidade em bps.
    if (Mode == 0) //Cálculo para baixa velocidade.
    {
        BR = (_XTAL_FREQ / (64 * BaudRate)) - 1;
        SPBRG = BR;
    } else //Cálculo para baixa velocidade.
    {
        BR = (_XTAL_FREQ / (16 * BaudRate)) - 1;
        SPBRG = BR;
    }

    // Configuração do Registrador TXSTA.
    TXSTAbits.CSRC = 1; // Seleção MASTER/SLAVE para o Modo Síncrono.
    TXSTAbits.TX9 = 0; // Transmição de Dados em 8 Bits.
    TXSTAbits.TXEN = 1; // Habilita a Transmição de Dados.
    TXSTAbits.SYNC = 0; // Modo de Comunicação Assíncrono.
    TXSTAbits.BRGH = Mode; // Baud Rate em alta ou baixa velocidade.
    TXSTAbits.TRMT = 1; // Situação do Registrador Interno de Transmissão (TSR).
    TXSTAbits.TX9D = 0; // Valor a Ser Transmitido como 9º bit (Paridade/Endereçamento).

    // Configuração do Registrador RCSTA.
    RCSTAbits.SPEN = 1; // Habilita o Sistema USART.
    RCSTAbits.RX9 = 0; // Recepção de Dados em 8 Bits.
    RCSTAbits.SREN = 0; // Desabilita Recepção Unitária (Somente Modo Síncrono em MASTER).
    RCSTAbits.CREN = 1; // Habilita a Recepção Contínua de Dados.
    RCSTAbits.ADDEN = 0; // Desabilita o Sistema de Endereçamento.
    RCSTAbits.FERR = 0; // Erro de Stop Bit.
    RCSTAbits.OERR = 0; // Erro de Muitos Bytes Recebidos sem Leitura.
    RCSTAbits.RX9D = 0; // Valor a Ser Recebido como 9º bit (Paridade/Endereçamento).

    // Configuração da Interrupção USART.
    PIE1bits.RCIE = 1; // Habilita a Interrupção Serial.
    PIR1bits.RCIF = 0; // Habilita a Interrupção Serial de Recepção.
}

/**
 *  Funcao responsavel por escrever um caractere que será enviado no canal serial do PIC16F877A
 * @param USARTData recebe o caracttere a ser escrito
 */
void USARTWriteChar(unsigned char USARTData) {
    while (!PIR1bits.TXIF);
    TXREG = USARTData;
}

/**
 * Funcao responsavel por escrever uma String no canal serial do pic
 * @param str recebe como parametro um ponteiro, que nada mais é do que uma String de caracter a ser enviada
 */
void USARTWriteString(const char *str) {
    // Efetua a transmissão da string para a USART.
    while (*str != '\0') {
        // Envio da string byte a byte.
        USARTWriteChar(*str);
        str++;
    }
}

/**
 * Funcao responsavel por ler um caractere recebido no canal serial
 * @return um caractere lido
 */
unsigned char USARTReceiveChar(void) {

    unsigned char USARTData;

    if (!OERR) // Erro de Muitos Bytes Recebidos sem Nenhuma Leitura.
    {
        USARTData = RCREG; // Recebe o byte da USART e atribui a variável USARTData.
        PIR1bits.RCIF = 0; // Limpa a Flag da Interrupção de Recepção.
    } else {
        USARTWriteString("\n\r ------- ESTOURO DE PILHA ------- \n\r ");

        RCSTAbits.CREN = 0; // Desabilita a Recepção Contínua de Dados Momentaneamente.
        USARTData = RCREG; // Recebe o byte da USART e atribui a variável USARTData.
        RCSTAbits.CREN = 1; // Habilita a Recepção Contínua de Dados Novamente.
        PIR1bits.RCIF = 0; // Limpa a Flag da Interrupção de Recepção.
    }

    return (USARTData);
}

/**
 * Funcao responsavel por ativar ou desativar o uso da bomba d'água e o motor
 * @autor Diovani Bernardid a Motta
 * @param comando
 */
void Ativar(char comando) {
    if (comando == LIGAR_BOMBA) {
        RELEBOMBA = 0;
        LEDBOMBA = 1;
        USARTWriteChar(LIGAR_BOMBA);
    } else if (comando == DESLIGAR_BOMBA) {
        RELEBOMBA = 1;
        LEDBOMBA = 0;
        USARTWriteChar(DESLIGAR_BOMBA);
    } else if (comando == LIGAR_MOTOR) {
        RELEMOTOR = 1;
        USARTWriteChar(LIGAR_MOTOR);
    } else if (comando == DESLIGAR_MOTOR) {
        RELEMOTOR = 0;
        USARTWriteChar(DESLIGAR_MOTOR);
    } else if (comando == LIGAR) {
        flag_Start = 1;
        USARTWriteChar(LIGAR);
    } else if (comando == DESLIGAR) {
        flag_Start = 0;
        USARTWriteChar(DESLIGAR);
    }
}

/**
 *  Funcao responsavel por preparar o valor convertido da leitura em um valor que será enviado para a serial
 * @param preint o valor que sera enviado via serial
 */
void sendSerial(char* nivelRecipiente) {
    int x = 0;
    char origem [40];
    const char * envio;
    //enquanto nao for alcancado o fim da string
    while (*nivelRecipiente != '\0') {
        if ((x == 0)) {// se for a primeira posicao do vetor
            if (recipiente == SUPERIOR) { // se estivermos lendo um valor do recipiente superior
                origem[x] = 'S'; // adiciono a flag de controle S para informar que o valor lido é do do recipiente superior
            } else {
                origem[x] = 'I'; // adiciono a flag de controle I para informar que o valor lido é do do recipiente inferior
            }
            x++; // incremento a variavel de cotrole
        } else {
            origem[x] = *nivelRecipiente; // adiciono o caractere interado a posicao do vetor
            *nivelRecipiente++; // incremento o caractere do ponteiro
            x++; // incremento a variavel de controle
        }
    }
    envio = &origem; //converto o vetor em um ponteiro
    USARTWriteString(envio); // envio para o supervisorio
}

/**
 * Funcao responsavel por imprimir na barra de progresso o nivel de água do recipiente
 * @param preint recebe como parametro o o numero de colunas que o mesmo deve pintar.Intervalo de 0 a 16
 */
void progressBar(int preint) {
    int i = 0;
    unsigned char *result;
    unsigned char aux[16] = {' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    // imprime barrinha
    while (preint >= 0) {
        aux[i] = 0xFF;
        preint--;
        i++;
    }
    result = &aux;
    lcd_escreve_string(result);
    __delay_ms(200);

}

/**
 * Funcao responsavel por atualizar o estado dos ativos do circuito, sendo que essa
 * funcao tem como objetivo ligar ou desligar um mecanismo de acordo com o nivel d'agua
 */
void refresh(void) {
    if (recipiente == SUPERIOR) {
        if (ADCResult < 80) {
            RELEBOMBA = 0;
            LEDBOMBA = 1;
            Ativar(LIGAR_BOMBA);
        }

        if (ADCResult > 200) {
            RELEBOMBA = 1;
            LEDBOMBA = 0;
            Ativar(DESLIGAR_BOMBA);
        }
    } else if (recipiente == INFERIOR) {

        if (ADCResult < 80) {
            RELEMOTOR = 0;
            Ativar(LIGAR_MOTOR);
        }

        if (ADCResult > 200) {
            RELEMOTOR = 1;
            Ativar(DESLIGAR_MOTOR);
        }
    }
}

/**
 *  Funcao responsavel por configurar e inicializar o conversor Analogico-Digital do pic
 */
void ADCInit() {
    //Configuração do Registrador ADCON1 para a Conversão A/D.
    ADCON1bits.ADFM = 1; // Configuração dos Bits mais significativos a esquerda.
    ADCON1bits.PCFG3 = 0; // Configuração dos pinos analógicos e tensões de referência.
    ADCON1bits.PCFG2 = 0; // Configuração dos pinos analógicos e tensões de referência.
    ADCON1bits.PCFG1 = 0; // Configuração dos pinos analógicos e tensões de referência.
    ADCON1bits.PCFG0 = 0; // Configuração dos pinos analógicos e tensões de referência.
    //Configuração do Registrador ADCON0 para a Conversão A/D.
    ADCON0bits.ADCS1 = 1; // Frequência de Trabalho (Fosc/32 - 1.6us).
    ADCON0bits.ADCS0 = 0; // Frequência de Trabalho (Fosc/32 - 1.6us).
    ADCON0bits.ADON = 1; // Ativa o Sistema de Conversão A/D.
    //Configuração dos Registradores PIE1 e PIR1 para a Conversão A/D.
    PIE1bits.ADIE = 1; // Interrupção do conversor A/D Habilitada.
    PIR1bits.ADIF = 0; // Limpa a Flag da Interrupção da Conversão A/D.
}

/**
 *  Funcao responsavel por realizar a leitura de um canal analogico do pic
 * @param ch recebe como parametro o canal analogico a seril lido
 */
void ADCRead(int ch) {

    ADCON0bits.CHS = ch; // Configuração do Canal 0 (RA0/AN0).
    recipiente = ch;
    __delay_us(25); //Waits for the acquisition to complete
    ADCON0bits.GO = 1;
    while (ADCON0bits.GO_DONE);
}

/**
 * Funcao responsavel por tratar as interrupções geradas pelos perifericos do pic
 */
void interrupt ISR(void) {
    // Verificação se a Interrupção foi causada pela conversão A/D.
    if (PIR1bits.ADIF) {
        // Converte os dois bytes em um valor inteiro para manipulação de dados.
        ADCResult = ((ADRESH << 8) + ADRESL);

        char * buf, * per;
        float input, input2;
        float pre;
        int preint, status;

        input = ADCResult * 0.0048828125;
        buf = ftoa(input, &status); //Vetor buf armazena a tensão convertida.

        lcd_gotoxy(0, 1);
        lcd_escreve_string("Nivel:");
        lcd_gotoxy(7, 1);

        input2 = ((input * 100) / 1);
        per = ftoa(input2, &status);

        lcd_escreve_string(per);
        lcd_gotoxy(16, 1);
        lcd_escreve_string("%");
        lcd_gotoxy(0, 0);

        pre = input2 * 0.16;
        preint = (int) pre;

        sendSerial(per);
        progressBar(preint);
        refresh();

        PIR1bits.ADIF = 0; // Limpa a flag da interrupção do conversor A/D.
    }
    //Verifica se a interrupção foi causada pela recepção de bytes.
    if (PIR1bits.RCIF) {
        USARTWriteChar(USARTReceiveChar());
        Ativar(USARTReceiveChar());
    }
}

/**
 *  Funcao responsavel por inicializar o sistema caso seja pressionado botão
 * para ligar/desligar o circuito
 */
void ativar(void) {
    if (BOTAO_LIGAR_DESLIGAR == 1) {
        __delay_ms(300);
        if (flag_Start == 0) {
            Ativar(LIGAR);
            flag_Start = 1;
        } else {
            flag_Start = 0;
            Ativar(DESLIGAR);
        }
    }

    if (flag_Start == 1) {
        ADCRead(SUPERIOR);
        __delay_ms(300);
        ADCRead(INFERIOR);
        __delay_ms(300);
    }
}

/**
 * Funcao responsavel por inicializar e configurar os PORTS usados pelo PIC,
 * comunicacao Serial e conversor ADC
 */
void inicialize(void) {

    USARTInit(57600, 1); // inicializacao da comunicacao serial

    TRISA = 0b11111111;
    TRISDbits.TRISD3 = 0;
    TRISDbits.TRISD2 = 0;
    TRISCbits.TRISC0 = 1;
    TRISCbits.TRISC1 = 1;
    TRISCbits.TRISC2 = 1;
    TRISCbits.TRISC3 = 1;

    PORTAbits.RA0 = 0;
    RELEBOMBA = 1;
    LEDBOMBA = 0;
    BOTAO_LIGAR_DESLIGAR = 0;
    __delay_ms(2000);

    init_lcd_4bit(); // inicializacao do LCD
    __delay_ms(2000);
    INTCONbits.PEIE = 1; // Habilita Interrupção de Periféricos do Microcontrolador.
    INTCONbits.GIE = 1; // Habilita Interrupção Global.
    __delay_ms(1000);
    ADCInit(); //inicializacao convertor ADC
    lcd_escreve_string("\fLoading PIC..."); //escrita no LCD
    __delay_ms(5000);
    LCDClear();
}

void main(void) {

    inicialize(); // funcao responsavel por inicializar os ports do pic

    while (1) {
        ativar(); // funcao responsavel por inicializar o supervisorio
    }
}

