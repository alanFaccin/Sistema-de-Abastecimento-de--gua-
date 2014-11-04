#include <stdio.h>
#include <stdlib.h>
#include "hardware.h"
#include "lcd_4bit.h"

#define RELEBOMBA   PORTDbits.RD2
#define LEDBOMBA    PORTDbits.RD3
#define RELEMOTOR   PORTDbits.RD4
#define RESET       PORTCbits.RC3
#define START       PORTCbits.RC2
#define LIGAR_BOMBA 'B'
#define DESLIGAR_BOMBA 'b'
#define LIGAR_MOTOR 'M'
#define DESLIGAR_MOTOR 'm'


//Vari�veis Globais de Controle.
int flag_Leitura = 0;
int Buffer_Leitura = 0;
const int SUPERIOR = 0;
const int INFERIOR = 1;
int ADCResult = 0;
int flag_an = -1;

/**
 * Funcao responsavel por configurar a taxa de transferencia e o modo de transferencia da serial
 * @param BaudRate recebe a taxa de transferencia que ser� usada na comunicacao
 * @param Mode o modo de transmissao
 */
void USARTInit(long BaudRate, int Mode) {
    int BR = 0;

    // C�lculo do valor para o registrador SPBRG para uma determinada velocidade em bps.
    if (Mode == 0) //C�lculo para baixa velocidade.
    {
        BR = (_XTAL_FREQ / (64 * BaudRate)) - 1;
        SPBRG = BR;
    } else //C�lculo para baixa velocidade.
    {
        BR = (_XTAL_FREQ / (16 * BaudRate)) - 1;
        SPBRG = BR;
    }

    // Configura��o do Registrador TXSTA.
    TXSTAbits.CSRC = 1; // Sele��o MASTER/SLAVE para o Modo S�ncrono.
    TXSTAbits.TX9 = 0; // Transmi��o de Dados em 8 Bits.
    TXSTAbits.TXEN = 1; // Habilita a Transmi��o de Dados.
    TXSTAbits.SYNC = 0; // Modo de Comunica��o Ass�ncrono.
    TXSTAbits.BRGH = Mode; // Baud Rate em alta ou baixa velocidade.
    TXSTAbits.TRMT = 1; // Situa��o do Registrador Interno de Transmiss�o (TSR).
    TXSTAbits.TX9D = 0; // Valor a Ser Transmitido como 9� bit (Paridade/Endere�amento).

    // Configura��o do Registrador RCSTA.
    RCSTAbits.SPEN = 1; // Habilita o Sistema USART.
    RCSTAbits.RX9 = 0; // Recep��o de Dados em 8 Bits.
    RCSTAbits.SREN = 0; // Desabilita Recep��o Unit�ria (Somente Modo S�ncrono em MASTER).
    RCSTAbits.CREN = 1; // Habilita a Recep��o Cont�nua de Dados.
    RCSTAbits.ADDEN = 0; // Desabilita o Sistema de Endere�amento.
    RCSTAbits.FERR = 0; // Erro de Stop Bit.
    RCSTAbits.OERR = 0; // Erro de Muitos Bytes Recebidos sem Leitura.
    RCSTAbits.RX9D = 0; // Valor a Ser Recebido como 9� bit (Paridade/Endere�amento).

    // Configura��o da Interrup��o USART.
    PIE1bits.RCIE = 1; // Habilita a Interrup��o Serial.
    PIR1bits.RCIF = 0; // Habilita a Interrup��o Serial de Recep��o.
}

/**
 *  Funcao responsavel por escrever um caractere no canal serial
 * @param USARTData recebe o caractere a ser enviado
 */
void USARTWriteChar(unsigned char USARTData) {
    while (!PIR1bits.TXIF);
    TXREG = USARTData;
}

/**
 * Funcao responsavel por escrever uma string na comunica��o serial
 * @param str recebe como parametro a string a ser impressa
 */
void USARTWriteString(const char *str) {
    // Efetua a transmiss�o da string para a USART.
    while (*str != '\0') {
        // Envio da string byte a byte.
        USARTWriteChar(*str);
        str++;
    }
}

/**
 *  Funcao responsavel por ler um caractere recebido pela comunicao serial
 * @return um caractere lido do canal serial
 */
unsigned char USARTReceiveChar(void) {
    unsigned char USARTData;

    if (!OERR) // Erro de Muitos Bytes Recebidos sem Nenhuma Leitura.
    {
        USARTData = RCREG; // Recebe o byte da USART e atribui a vari�vel USARTData.
        PIR1bits.RCIF = 0; // Limpa a Flag da Interrup��o de Recep��o.
    } else {
        USARTWriteString("\n\r ------- ESTOURO DE PILHA ------- \n\r ");

        RCSTAbits.CREN = 0; // Desabilita a Recep��o Cont�nua de Dados Momentaneamente.
        USARTData = RCREG; // Recebe o byte da USART e atribui a vari�vel USARTData.
        RCSTAbits.CREN = 1; // Habilita a Recep��o Cont�nua de Dados Novamente.
        PIR1bits.RCIF = 0; // Limpa a Flag da Interrup��o de Recep��o.
    }
    return (USARTData);
}

/**
 * Funcao responsavel por inicalizar o convero AD do PIC 16F877A
 */
void ADCInit() {
    //Configura��o do Registrador ADCON1 para a Convers�o A/D.
    ADCON1bits.ADFM = 1; // Configura��o dos Bits mais significativos a esquerda.
    ADCON1bits.PCFG3 = 0; // Configura��o dos pinos anal�gicos e tens�es de refer�ncia.
    ADCON1bits.PCFG2 = 0; // Configura��o dos pinos anal�gicos e tens�es de refer�ncia.
    ADCON1bits.PCFG1 = 0; // Configura��o dos pinos anal�gicos e tens�es de refer�ncia.
    ADCON1bits.PCFG0 = 0; // Configura��o dos pinos anal�gicos e tens�es de refer�ncia.

    //Configura��o do Registrador ADCON0 para a Convers�o A/D.
    ADCON0bits.ADCS1 = 1; // Frequ�ncia de Trabalho (Fosc/32 - 1.6us).
    ADCON0bits.ADCS0 = 0; // Frequ�ncia de Trabalho (Fosc/32 - 1.6us).
    ADCON0bits.ADON = 1; // Ativa o Sistema de Convers�o A/D.

    //Configura��o dos Registradores PIE1 e PIR1 para a Convers�o A/D.
    PIE1bits.ADIE = 1; // Interrup��o do conversor A/D Habilitada.
    PIR1bits.ADIF = 0; // Limpa a Flag da Interrup��o da Convers�o A/D.
}

/**
 * Funcao responsavel por ler um canal analogico
 * @param ch
 */
void ADCRead(int ch) {
    ADCON0bits.CHS = ch; // Configura��o do Canal 0 (RA0/AN0).
    __delay_us(25); //Waits for the acquisition to complete
    ADCON0bits.GO = 1;
    while (ADCON0bits.GO_DONE);
}

/**
 * Funcao responsavel por ativar ou desativar o uso da bomba d'�gua e o motor
 * @autor Diovani Bernardid a Motta
 * @param comando
 */
void Ativar(char comando) {
    if (comando == LIGAR_BOMBA) {
        RELEBOMBA = 0;
        LEDBOMBA = 1;
        USARTWriteString(LIGAR_BOMBA);
    } else if (comando == DESLIGAR_BOMBA) {
        RELEBOMBA = 1;
        LEDBOMBA = 0;
        USARTWriteString(DESLIGAR_BOMBA);
    } else if (comando == LIGAR_MOTOR) {
        RELEMOTOR = 1;
        USARTWriteString(LIGAR_MOTOR);
    } else if (comando == DESLIGAR_MOTOR) {
        RELEMOTOR = 0;
        USARTWriteString(DESLIGAR_MOTOR);
    }
}

/**
 *  Funcao responsavel por verificar o nivel de agua no recipiente superior 
 */
void Bomba(void) {
    //Aciona o rele que starta a bomba quando o nivel esta baixo
    if (ADCResult < 40) {
        Ativar(LIGAR_BOMBA);
    }

    if (ADCResult > 160) {
        Ativar(DESLIGAR_BOMBA);
    }
}

/**
 * Funcao responsavel por exibir no lcd o progresso do enchimento do recipiente superior
 * @param preint recebe o percentual lido do convertor ad
 */
void ProgressBar(int preint) {
    int i = 0;
    unsigned char *result;
    unsigned char aux[16] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

    // imprime barrinha
    while (preint >= 0) {
        aux[i] = 0xFF;
        preint--;
        i++;
    }
    result = &aux;
    // escreve o resultado no LCD
    lcd_escreve_string(result);
    __delay_ms(200);
}

/**
 *  Funcao responsavel por preparar o valor convertido da leitura em um valor que ser� enviado para a serial
 * @param preint o valor que sera enviado via serial
 * @param recipiente  informa de qual recipiente foi lido o valor
 */
void sendSerial(int preint, int recipiente) {
    int status = 0, x = 0;
    char origem [40];
    const char * envio;
    const char * nivelRecipiente;
    nivelRecipiente = ftoa(preint, &status); // converto o valor que sera enviado para o supervisorio
    //enquanto nao for alcancado o fim da string
    while (*nivelRecipiente != '\n') {
        if ((x == 0)) {// se for a primeira posicao do vetor
            if (recipiente == SUPERIOR) { // se estivermos lendo um valor do recipiente superior
                origem[x] = 'S'; // adiciono a flag de controle S para informar que o valor lido � do do recipiente superior
            } else {
                origem[x] = 'I'; // adiciono a flag de controle I para informar que o valor lido � do do recipiente inferior
            }
            x++; // incremento a variavel de cotrole
        } else {
            origem[x] = *nivelRecipiente; // adiciono o caractere interado a posicao do vetor
            *nivelRecipiente++; // incremento o caractere do ponteiro
            x++; // incremento a variavel de controle 
        }
    }
    envio = &origem; //converto o vetor em um ponteiro
    USARTWriteString(*envio); // envio para o supervisorio
}

/**
 *  Funcao responsavel por gerenciar as interrucoes causadas pelos perifericos do PICF877A
 * */
void interrupt ISR(void) {
    // Verifica��o se a Interrup��o foi causada pela convers�o A/D.
    if (PIR1bits.ADIF) {

        char * percentual;
        float input;
        float input2;
        float pre;
        int preint;
        int status;

        if (flag_Leitura < 5) {
            flag_Leitura++;
            // Converte os dois bytes em um valor inteiro para manipula��o de dados.
            ADCResult = ((ADRESH << 8) + ADRESL);
            Buffer_Leitura += ADCResult;
        } else {

            flag_Leitura=0;
            input = (Buffer_Leitura * 0.0048828125)/5;
            // Envia o valor formatado para o LCD.
            // coluna e linha
            lcd_gotoxy(0, 1);
            lcd_escreve_string("Nivel:");
            USARTWriteString("Nivel:");
            lcd_gotoxy(7, 1);
            input2 = ((input * 100) / 0.8);
            percentual = ftoa(input2, &status);
            lcd_escreve_string(percentual);
            USARTWriteString(percentual);
            lcd_gotoxy(16, 1);
            lcd_escreve_string("%");
            USARTWriteString("%");
            lcd_gotoxy(0, 0);
            pre = input2 * 0.16;
            preint = (int) pre;
            Bomba(); // fun�ao responsavel por verificar o nivel de �gua no recipiente superior
            ProgressBar(preint); // chamada a funcao responsavel por exibir o progresso do preenchimento
            sendSerial(preint, SUPERIOR);
        }
        PIR1bits.ADIF = 0; // Limpa a flag da interrup��o do conversor A/D.
    }

    //Verifica se a interrup��o foi causada pela recep��o de bytes.
    if (PIR1bits.RCIF) {
        char recebido = USARTReceiveChar(); // caractere recebido do por interrup��o da USART
        // repassa o caracter lido para a funcao responsavel por ativar ou nao a bomba ou o motor
        Ativar(recebido);
        //USARTWriteString("\n\r Entrou na funcao de Interrupcao da USART");
        //USARTWriteString("\n\r Caracter Digitado :");
        //USARTWriteChar(recebido);

    }
}

/**
 *  Funcao responsavel por verificar se o botao contido na placa, responsavel por resetar o sistema est� pressionado
 */
void reset(void) {
    if (RESET == 1) {
        while (RESET == 1) {
            RELEBOMBA = 1;
            LEDBOMBA = 0;
        }
    }
}

/**
 *  Funcao ir� configurar os ports usados pelo PIC16F8&77A
 */
void inicialize(void) {
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
    PORTCbits.RC2 = 0;
    __delay_ms(2000);
    init_lcd_4bit();
    __delay_ms(2000);
    INTCONbits.PEIE = 1; // Habilita Interrup��o de Perif�ricos do Microcontrolador.
    INTCONbits.GIE = 1; // Habilita Interrup��o Global.
    __delay_ms(1000);
    ADCInit();
    lcd_escreve_string("\fLoading PIC...");
    USARTWriteString("\fLoading PIC...");
    __delay_ms(5000);
    LCDClear();
}

void main(void) {
    inicialize(); // funcao que ir� inicializar as configur�es do port
    if (START == 1) {
        USARTWriteString("Kaliane");
        while (1) {//loop inUSARTWriteString("Kaliane");finito
            ADCRead(1);
            __delay_ms(300);
            reset(); // funcao responsavel por verificar o o botao reset esta pressionado
        }
    }
}

