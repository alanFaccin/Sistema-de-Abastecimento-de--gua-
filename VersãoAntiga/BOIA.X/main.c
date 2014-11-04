#include <stdio.h>
#include <stdlib.h>
#include "hardware.h"
#include "lcd_4bit.h"


//Variáveis Globais de Controle.
int ADCResult = 0;
//unsigned short ADCResult = 0;
unsigned char Display[7];
int flag_an = -1;

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

void USARTWriteChar(unsigned char USARTData) {
    while (!PIR1bits.TXIF);
    TXREG = USARTData;
}

//-----------------------------------------------------------------------------

void USARTWriteString(const char *str) {
    // Efetua a transmissão da string para a USART.
    while (*str != '\0') {
        // Envio da string byte a byte.
        USARTWriteChar(*str);
        str++;
    }
}

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

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
    //ADCON0bits.CHS2 = 0; // Configuração do Canal 0 (RA0/AN0).
    //ADCON0bits.CHS1 = 0; // Configuração do Canal 0 (RA0/AN0).
    //ADCON0bits.CHS0 = 0; // Configuração do Canal 0 (RA0/AN0).
    ADCON0bits.ADON = 1; // Ativa o Sistema de Conversão A/D.

    //Configuração dos Registradores PIE1 e PIR1 para a Conversão A/D.
    PIE1bits.ADIE = 1; // Interrupção do conversor A/D Habilitada.
    PIR1bits.ADIF = 0; // Limpa a Flag da Interrupção da Conversão A/D.
}
//-----------------------------------------------------------------------------

void ADCRead(int ch) {



    ADCON0bits.CHS = ch; // Configuração do Canal 0 (RA0/AN0).
    //ADCON0bits.CHS1 = 0; // Configuração do Canal 0 (RA0/AN0).
    //ADCON0bits.CHS0 = 0; // Configuração do Canal 0 (RA0/AN0).
    __delay_us(25); //Waits for the acquisition to complete
    ADCON0bits.GO = 1;
    while (ADCON0bits.GO_DONE);

    //ADCResult = (ADRESH<<8) + ADRESL ;   //Merging the MSB and LSB

}

//-----------------------------------------------------------------------------

void interrupt ISR(void) {


    // Verificação se a Interrupção foi causada pela conversão A/D.
    if (PIR1bits.ADIF) {
        // Converte os dois bytes em um valor inteiro para manipulação de dados.
        //ADCResult = ((ADRESH << 8) + ADRESL) * 0.0048828125;
        ADCResult = ((ADRESH << 8) + ADRESL);

        //Variáveis para a função ftoa funcionar corretamente.
        char * buf;
        char * per;
        char * teste;
        char * qtdc;
        float input;
        float input2;
        float pre;
        int preint;
        int status;
        int status2;
        int status3;
        int i = 0;
        unsigned char *result;
        unsigned char aux[16] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

        input = ADCResult * 0.0048828125;
        buf = ftoa(input, &status); //Vetor buf armazena a tensão convertida.


        // Monta o valor de 10 bits para mandar para LCD.
        Display[0] = (ADCResult / 1000) + 48; // Obtém a milhar do valor.
        Display[1] = ((ADCResult / 100) % 10) + 48; // Obtém a centena do valor.
        Display[2] = ((ADCResult / 10) % 10) + 48; // Obtém a dezena do valor.
        Display[3] = (ADCResult % 10) + 48; // Obtém a unidade do valor.

        // Envia o valor formatado para o LCD.
        // coluna e linha

        lcd_gotoxy(0, 1);
        lcd_escreve_string("Nivel:");
        lcd_gotoxy(7, 1);
        input2 = ((input * 100) / 1);
        per = ftoa(input2, &status2);
        lcd_escreve_string(per);
        USARTWriteString(per);
        lcd_gotoxy(16, 1);
        lcd_escreve_string("%");
        lcd_gotoxy(0, 0);
        pre = input2 * 0.16;
        preint = (int) pre;
        //Aciona o rele que starta a bomba quando o nivel esta baixo
        if (ADCResult < 200) {
            PORTDbits.RD2 = 0;
            PORTDbits.RD3 = 1;
        }

        if (ADCResult > 200) {
            PORTDbits.RD2 = 1;
            PORTDbits.RD3 = 0;
        }
        // imprime barrinha

        while (preint >= 0) {
            aux[i] = 0xFF;
            preint--;
            i++;
        }
        result = &aux;
        //        teste = ftoa(preint, &status2);
        lcd_escreve_string(result);
        //lcd_gotoxy(0, 0);


        //if (ADCResult >= 769) {
        //  LCDCursor(0, 9);
        //  LCDWriteString("100%");
        // }
        // if (ADCResult >= 513 && ADCResult <= 768) {
        //    LCDCursor(0,9);
        //   LCDWriteString("75%");
        // }

        // if (ADCResult >= 257 && ADCResult <= 512) {
        //    LCDCursor(0,9);
        //    LCDWriteString("5%");
        // }

        // if (ADCResult >= 0 && ADCResult <= 256) {
        //   LCDCursor(0,9);
        //  LCDWriteString("25%");
        // }

        __delay_ms(200);

        PIR1bits.ADIF = 0; // Limpa a flag da interrupção do conversor A/D.
    }
    //Verifica se a interrupção foi causada pela recepção de bytes.
    if (PIR1bits.RCIF) {
        USARTWriteString("\n\r Entrou na funcao de Interrupcao da USART");
        USARTWriteString("\n\r Caracter Digitado :");
        USARTWriteChar(USARTReceiveChar());

    }
}

void main(void) {
    TRISA = 0b11111111;
    PORTAbits.RA0 = 0;
    TRISDbits.TRISD2 = 0;
    PORTDbits.RD2 = 1;
    TRISDbits.TRISD3 = 0;
    PORTDbits.RD3 = 0;
    //Botoes
    TRISCbits.TRISC0 = 1;
    TRISCbits.TRISC1 = 1;
    TRISCbits.TRISC2 = 1;
    TRISCbits.TRISC3 = 1;
    PORTCbits.RC3 = 0;
    __delay_ms(2000);
    init_lcd_4bit();
    __delay_ms(2000);
    INTCONbits.PEIE = 1; // Habilita Interrupção de Periféricos do Microcontrolador.
    INTCONbits.GIE = 1; // Habilita Interrupção Global.
    __delay_ms(1000);
    ADCInit();
    lcd_escreve_string("\fLoading PIC...");
    USARTWriteString("\fLoading PIC...");
    __delay_ms(5000);
    LCDClear();

    while (1) {
        ADCRead(0);
        __delay_ms(300);
        if (PORTCbits.RC3 == 1) {
            while (PORTCbits.RC3 == 1) {
                PORTDbits.RD2 = 1;
                PORTDbits.RD3 = 0;
            }
        }
    }
}

