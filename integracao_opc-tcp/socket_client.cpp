#pragma once
#include "socket_client.h"
// Engenharia de Controle e Automacao - UFMG
// Disciplina: Sistemas Distribuidos para Automacao
//
// Programa-cliente para simular um software de convers�o tcp/OPC-UA
//
// Para a correta compilacao deste programa, nao se esqueca de incluir a
// biblioteca Winsock2 (Ws2_32.lib) no projeto ! (No Visual C++ Express Edition:
// Projects->Properties->Configuration Properties->Linker->Input->Additional Dependencies).
//
//Para a depura��o ocorrer adequadamente com o programa servidor provido pelo
//prof. Luiz T. S. Mendes, o programa deve ser configurado com os par�metros de entrada:
//Projects->Properties->Configuration Properties->Debug->Command arguments->127.0.0.1 3445).

#pragma warning(disable:6031)
#pragma warning(disable:6385)
#pragma warning(disable:4996)

#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <string>
#include <mutex>
#include "GlobalVariables.h"
#include "MessageHandling.h"

using namespace std;

#define SETPOINT_MESSAGE_SIZE 29  // 6+3+6+6+4 caracteres + 4 separadores (MSG de set-point)
#define ACK_MESSAGE_SIZE 10  // 6+3 caracteres + 1 separador

#define ESC			 0x1B

#define ESC 0x1B

#define WHITE   FOREGROUND_RED   | FOREGROUND_GREEN      | FOREGROUND_BLUE  | FOREGROUND_INTENSITY
#define HLGREEN FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define HLRED   FOREGROUND_RED   | FOREGROUND_INTENSITY
#define HLBLUE  FOREGROUND_BLUE  | FOREGROUND_INTENSITY
#define YELLOW  FOREGROUND_RED   | FOREGROUND_GREEN
#define CYAN    FOREGROUND_BLUE  | FOREGROUND_GREEN      | FOREGROUND_INTENSITY

string processDataMessage;
int status, acao, sequenceNumber = 0;
HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
SOCKET      s;

/**************************************************************************/
/* Fun��o para testar o c�digo de erro na comunica��o via sockets         */
/*                                                                        */
/* Par�metros de entrada:                                                 */
/*     status - c�digo devolvido pela fun��o de sockets chamada           */
/*                                                                        */
/*                                                                        */
/* Valor de retorno: 0 se n�o houve erro                                  */
/*                  -1 se o erro for recuper�vel                          */
/*                  -2 se o erro for irrecuper�vel                        */
/**************************************************************************/

int CheckSocketError(int status, HANDLE hOut) {
    int erro;

    if (status == SOCKET_ERROR) {
        SetConsoleTextAttribute(hOut, HLRED);
        erro = WSAGetLastError();
        if (erro == WSAEWOULDBLOCK) {
            printf("Timeout na operacao de RECV! errno = %d - reiniciando...\n\n", erro);
            return(-1); // acarreta rein�cio da espera de mensagens no programa principal
        }
        else if (erro == WSAECONNABORTED) {
            printf("Conexao abortada pelo cliente TCP - reiniciando...\n\n");
            return(-1); // acarreta rein�cio da espera de mensagens no programa principal
        }
        else {
            printf("Erro de conexao! valor = %d\n\n", erro);
            return (-2); // acarreta encerramento do programa principal
        }
    }
    else if (status == 0) {
        //Este caso indica que a conex�o foi encerrada suavemente ("gracefully")
        printf("Conexao com cliente TCP encerrada prematuramente! status = %d\n\n", status);
        return(-1); // acarreta rein�cio da espera de mensagens no programa principal
    }
    else return(0);
}

void checkAndIncreaseSequenceNumber(const char* message) {
    int actualSequenceNumber;
    sscanf(message, "%6d", &actualSequenceNumber);
    if (++sequenceNumber != actualSequenceNumber) {
        SetConsoleTextAttribute(hOut, HLRED);
        printf("Numero sequencial de mensagem incorreto [1]: observado %d ao inves de %d\n",
            actualSequenceNumber, sequenceNumber);
        SetConsoleTextAttribute(hOut, WHITE);
        exit(0);
    }
}

void CALLBACK sendProcessDataMessage(PVOID lpParameter, BOOLEAN TimerOrWaitFired) {

    // sendMessage(getProcessDataMessage()); // TODO: get message from OPC

}

bool sendMessage(string message) {
    checkAndIncreaseSequenceNumber(message.c_str());
    status = send(s, message.c_str(), message.size(), 0);
    return ((acao = CheckSocketError(status, hOut)) != 0);
}

bool receiveProcessComputerACK() {
    char buf[ACK_MESSAGE_SIZE + 1];
    status = recv(s, buf, ACK_MESSAGE_SIZE, 0);
    checkAndIncreaseSequenceNumber(buf);
    if (strncmp(&buf[7], "101", 3) != 0) {
        SetConsoleTextAttribute(hOut, HLRED);
        buf[10] = '\0';
        printf("Mensagem de ACK invalida: recebido %s ao inves de '101'\n\n", &buf[7]);
        exit(0);
    }
    return ((acao = CheckSocketError(status, hOut)) != 0);
}

void socket_client()
{
    const char* ipaddr = "127.0.0.1";
    WSADATA     wsaData;
    SOCKADDR_IN ServerAddr;
    int  status;
    string processDataExampleMessage = "000001$100$1435.0$1480.0$0002.0$0010.0";
    MessageHandling processDataExample(processDataExampleMessage);

    status = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (status != 0) {
        printf("Falha na inicializacao do Winsock 2! Erro  = %d\n", WSAGetLastError());
        WSACleanup();
        exit(0);
    }

    // Cria um novo socket para estabelecer a conex�o.
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        status = WSAGetLastError();
        if (status == WSAENETDOWN)
            printf("Rede ou servidor de sockets inacess�veis!\n");
        else
            printf("Falha na rede: codigo de erro = %d\n", status);
        WSACleanup();
        exit(0);
    }

    // Inicializa a estrutura SOCKADDR_IN que ser� utilizada para
    // a conex�o ao servidor.
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(PORT);
    ServerAddr.sin_addr.s_addr = inet_addr(ipaddr);

    // Estabelece a conex�o com o servidor
    status = connect(s, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr));
    if (status == SOCKET_ERROR) {
        printf("Falha na conexao ao servidor ! Erro  = %d\n", WSAGetLastError());
        WSACleanup();
        exit(0);
    }

    if (sendMessage(processDataExample.toString())) {
        printf("Falha no envio da mensagem ! Erro  = %d\n", WSAGetLastError());
        WSACleanup();
        exit(0);
    }
    if (receiveProcessComputerACK()) {
        printf("Falha ao receber ACK ! Erro  = %d\n", WSAGetLastError());
        WSACleanup();
        exit(0);
    }
}
