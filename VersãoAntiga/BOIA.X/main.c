#include <stdio.h>
#include <stdlib.h>
#include "hardware.h"
#include "lcd_4bit.h"

// constantes que informam de qual recipiente ser� lida a informa��o
#define SUPERIOR 0
#define INFERIOR 1
//constantes que define a manipulacao dos estados dos agentes ativos do sistema
#define LIGAR_BOMBA 'B'
#define DESLIGAR_BOMBA 'b'
#define LIGAR_MOTOR 'M'
#define DESLIGAR_MOTOR 'm'
#define LIGAR 'L'
#define DESLIGAR 'l'
// constantes que define em qual pino do PIC16F877A est�o ligados os respectivos hardware
#define RELE_BOMBA PORTDbits.RD2
#define LED_BOMBA PORTDbits.RD3
#define RELE_MOTOR PORTDbits.RD6
#define BOTAO_LIGAR_DESLIGAR PORTCbits.RC0

//Vari�veis Globais de Controle.
int ADCResult = 0;
unsigned char Display[7];
int flag_an = -1;
int recipiente = 0; // variavel responsavel por controlar de qual recipiente estamos lendo os dados

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

//-----------------------------------------------------------------------------

void USARTWriteChar(unsigned char USARTData) {
    while (!PIR1bits.TXIF);
    TXREG = USARTData;
}

//-----------------------------------------------------------------------------

void USARTWriteString(const char *str) {
    // Efetua a transmiss�o da string para a USART.
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

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

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
//-----------------------------------------------------------------------------

void ADCRead(int ch) {

    ADCON0bits.CHS = ch; // Configura��o do Canal 0 (RA0/AN0).
    recipiente = ch; // informo de qual recipiente estamos lendo o valor
    __delay_us(25); //Waits for the acquisition to complete
    ADCON0bits.GO = 1;
    while (ADCON0bits.GO_DONE);

}

/**
 *  funcao responsavel por verificar de acordo com a informa��o
 * lida da porta serial, qual procedimento deve ser realizado
 * @param input recebe como parametro um caractere recebido da serial
 */
void active(char input) {

    if (input == LIGAR_BOMBA) {

        RELE_BOMBA = 0; // ativacao do rele usado no projeto � feita em nivel logico baixo
        LED_BOMBA = 1;
        USARTWriteChar(LIGAR_BOMBA); // escrevo no canal serial

    } else if (input == DESLIGAR_BOMBA) {

        RELE_BOMBA = 1;
        LED_BOMBA = 0;
        USARTWriteChar(DESLIGAR_BOMBA); // escrevo no canal serial

    } else if (input == LIGAR_MOTOR) {

        RELE_MOTOR = 0;
        USARTWriteChar(LIGAR_MOTOR); // escrevo no canal serial

    } else if (input == DESLIGAR_MOTOR) {

        RELE_MOTOR = 1;
        USARTWriteChar(DESLIGAR_MOTOR); // escrevo no canal serial

    } else if (input == LIGAR) {

        BOTAO_LIGAR_DESLIGAR = 1;
        USARTWriteChar(LIGAR); // escrevo no canal serial

    } else if (input == DESLIGAR) {

        BOTAO_LIGAR_DESLIGAR = 0;
        USARTWriteChar(DESLIGAR); // escrevo no canal serial

    }
}

/**
 *  funcao responsavel por preparar o nivel recebido e envia-lo pela porta serial
 * @param send recebe como parametro a informa��o a ser preparada para o envio
 */
void sendString(const char *send) {
    int x = 0;
    char array[40]; // vetor responsavel por armazenar as informa��es iteradas do parametro recebido
    char * envio; // ponteiro que ser� enviado via serial

    while (*send != '\0') { // enquanto n�o alca�armos o final da string
        if (x == 0) { // se estivermos iterado pela primeira vez a string
            if (recipiente == SUPERIOR) { // se estivermos tratando um valor lido do recipiente superior
                array[x] = 'S';
            } else if (recipiente == INFERIOR) { // se estivermos tratando um valor lido do recipiente inferior
                array[x] = 'I';
            }
            x++; // incremento a variavel de controle
        } else {
            array[x] = *send; // adiciono a posicao iterada do array o valor do caractere iterado
            send++;
            x++;
        }
    }

    if (x < 39) { // se n�o tivermos preechidos todas as posicoes do vetor
        array[x] = '\0'; // incremento caractere de fim de curso
    }

    envio = &array; // converto o vetor em ponteiro
    USARTWriteString(envio); // envio o valor para a serial
}

/**
 *  Funcao responsavel por imprimir a barra de progresso junto ao LCD
 * @param preint recebe como parametro o numero de segmentos do lcd que devem ser preechidos
 */
void progressBar(int preint) {
    int i = 0;
    unsigned char *result;
    unsigned char aux[16] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

    // imprime barrinha
    while (preint >= 0) { // enquanto nao for alcando a primeira coluna do LCD
        aux[i] = 0xFF;
        preint--;
        i++;
    }
    result = &aux; // ponteiro recebe array de char
    lcd_escreve_string(result); // escreve no lcd as barras de progresso
    __delay_ms(200);
}

//-----------------------------------------------------------------------------

void interrupt ISR(void) {

    // Verifica��o se a Interrup��o foi causada pela convers�o A/D.
    if (PIR1bits.ADIF) {
        // Converte os dois bytes em um valor inteiro para manipula��o de dados.
        ADCResult = ((ADRESH << 8) + ADRESL);

        //Vari�veis para a fun��o ftoa funcionar corretamente.
        char * buf;
        char * per;
        //char * teste;
        // char * qtdc;
        float input;
        float input2;
        float pre;
        int preint;
        int status;
        int status2;
        //int status3;

        input = ADCResult * 0.0048828125;
        buf = ftoa(input, &status); //Vetor buf armazena a tens�o convertida.


        // Monta o valor de 10 bits para mandar para LCD.
        Display[0] = (ADCResult / 1000) + 48; // Obt�m a milhar do valor.
        Display[1] = ((ADCResult / 100) % 10) + 48; // Obt�m a centena do valor.
        Display[2] = ((ADCResult / 10) % 10) + 48; // Obt�m a dezena do valor.
        Display[3] = (ADCResult % 10) + 48; // Obt�m a unidade do valor.

        lcd_gotoxy(0, 1);
        lcd_escreve_string("Nivel:");
        lcd_gotoxy(7, 1);
        input2 = ((input * 100) / 1);
        per = ftoa(input2, &status2);
        lcd_escreve_string(per);

        sendString(per); // chamada a funcao que ir� escrever o nivel do recipiente na porta serial

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

        progressBar(preint); //chamada a funcao que ir� pintar a barra de progresso

        PIR1bits.ADIF = 0; // Limpa a flag da interrup��o do conversor A/D.
    }
    //Verifica se a interrup��o foi causada pela recep��o de bytes.
    if (PIR1bits.RCIF) {
        // chamada a funcao responsavel por verificar de acordo com o caractere recebido, qual procedimento deve ser realizado
        active(USARTReceiveChar());
    }
}

/**
 *  Funcao responsavel por inicializar os PORTS e Configurar-los como saida ou entrada
 *  para o disposito PIC16F877A
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
    BOTAO_LIGAR_DESLIGAR = 0;
    //TRISCbits.TRISC1 = 1;
    // TRISCbits.TRISC2 = 1;
    // TRISCbits.TRISC3 = 1;

    PORTCbits.RC3 = 0;
    __delay_ms(2000);
    init_lcd_4bit();
    __delay_ms(2000);
    INTCONbits.PEIE = 1; // Habilita Interrup��o de Perif�ricos do Microcontrolador.
    INTCONbits.GIE = 1; // Habilita Interrup��o Global.
    __delay_ms(1000);
    ADCInit();
    lcd_escreve_string("\fLoading PIC...");
    //USARTWriteString("\fLoading PIC...");
    __delay_ms(5000);
    LCDClear();
}

void main(void) {

    //inicialize(); // funcao que ir� configurar a utilizacao dos PORTS
    TRISA = 0b11111111;
    PORTAbits.RA0 = 0;
    TRISDbits.TRISD2 = 0;
    PORTDbits.RD2 = 1;
    TRISDbits.TRISD3 = 0;
    PORTDbits.RD3 = 0;
    //Botoes
    TRISCbits.TRISC0 = 1;
    BOTAO_LIGAR_DESLIGAR = 0;
    //TRISCbits.TRISC1 = 1;
    // TRISCbits.TRISC2 = 1;
    // TRISCbits.TRISC3 = 1;

    PORTCbits.RC3 = 0;
    __delay_ms(2000);
    init_lcd_4bit();
    __delay_ms(2000);
    INTCONbits.PEIE = 1; // Habilita Interrup��o de Perif�ricos do Microcontrolador.
    INTCONbits.GIE = 1; // Habilita Interrup��o Global.
    __delay_ms(1000);
    ADCInit();
    lcd_escreve_string("\fLoading PIC...");
    //USARTWriteString("\fLoading PIC...");
    __delay_ms(5000);
    LCDClear();
    
    if (BOTAO_LIGAR_DESLIGAR == 1) {
        while (1) {
            USARTWriteString("loop");
            //ADCRead(SUPERIOR); // realizo a leitura do nivel d'agua do recipiente superior
            //__delay_ms(300);
            //PORTDbits.RD2 = 1;
            //PORTDbits.RD3 = 0;
        }
    }
}


