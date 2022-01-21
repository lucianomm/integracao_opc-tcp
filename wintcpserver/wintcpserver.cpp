/*
 * Sistemas Distribuidos para Automacao
 * Engenharia de Controle e Automacao
 * UFMG
 *
 * Trabalho Pratico sobre OPC e Sockets - 2021/2
 * ---------------------------------------------
 * Prof. Luiz T. S. Mendes
 *
 *
 * Programa de simulacao do Computador de Processo
 *
 * Este programa sera' o mesmo utilizado pelo professor para testar as solucoes
 * enviadas pelos alunos.
 *
 * Para a correta compilacao deste programa no WINDOWS, nao se esqueca de incluir
 * a biblioteca Winsock2 (Ws2_32.lib) no projeto ! (No Visual Studio 2010:
 * Projects->Properties->Configuration Properties->Linker->Input->Additional Dependencies).
 *
 * ======================================================================================
 * ATENCAO: APESAR DE TER SIDO TESTADO PELO PROFESSOR, N�O H� GARANTIAS DE QUE ESTE
 * PROGRAMA ESTEJA LIVRE DE ERROS. SE VOC� ENCONTRAR ALGUM ERRO NO PROGRAMA, COMUNIQUE O
 * MESMO AO PROFESSOR INDICANDO, SEMPRE QUE POSS�VEL, UMA FORMA DE CORRIGI-LO.
 * ======================================================================================
 */

#define _CRT_SECURE_NO_WARNINGS /* Para evitar warnings sobre fun�oes seguras de manipulacao de strings*/
#define _WINSOCK_DEPRECATED_NO_WARNINGS /* para evitar warnings de fun��es WINSOCK depreciadas */

// Para evitar warnings irritantes do Visual Studio
#pragma warning(disable:6031)
#pragma warning(disable:6385)

#include <winsock2.h>
#include <stdio.h>
#include <conio.h>

//#include <sys/types.h>	/* basic system data types */
//#include <sys/socket.h>	/* basic socket definitions */
//#include <netinet/in.h>
//#include <string.h>
//#include <stdio.h>
//#include <time.h>
//#include <errno.h>
//#include <unistd.h>

#define	LISTENQ	 1024	/* 2nd argument to listen() */
#define SA struct sockaddr

#define TAMMSGDADOS  38  // 6+3+6+6+6+6 caracteres + 5 separadores
#define TAMMSGREQ    10  // 6+3 caracteres + 1 separador
#define TAMMSGSP     29  // 6+3+6+6+4 caracteres + 4 separadores
#define TAMMSGACK    10  // 6+3 caracteres + 1 separador
#define TAMMSGACKSP  10  // 6+3 caracteres + 1 separador
#define ESC			 0x1B

#define ESC 0x1B

#define WHITE   FOREGROUND_RED   | FOREGROUND_GREEN      | FOREGROUND_BLUE  | FOREGROUND_INTENSITY
#define HLGREEN FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define HLRED   FOREGROUND_RED   | FOREGROUND_INTENSITY
#define HLBLUE  FOREGROUND_BLUE  | FOREGROUND_INTENSITY
#define YELLOW  FOREGROUND_RED   | FOREGROUND_GREEN
#define CYAN    FOREGROUND_BLUE  | FOREGROUND_GREEN      | FOREGROUND_INTENSITY

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
      printf ("Timeout na operacao de RECV! errno = %d - reiniciando...\n\n", erro);
      return(-1); // acarreta rein�cio da espera de mensagens no programa principal
    }
    else if (erro == WSAECONNABORTED) {
 	  printf("Conexao abortada pelo cliente TCP - reiniciando...\n\n");
	  return(-1); // acarreta rein�cio da espera de mensagens no programa principal
	}
    else {
      printf ("Erro de conexao! valor = %d\n\n", erro);
      return (-2); // acarreta encerramento do programa principal
    }
  }
  else if (status == 0) {
	//Este caso indica que a conex�o foi encerrada suavemente ("gracefully")
	printf ("Conexao com cliente TCP encerrada prematuramente! status = %d\n\n", status);
	return(-1); // acarreta rein�cio da espera de mensagens no programa principal
  }
  else return(0);
}

/***********************************************/
/* Fun��o para encerrar a conexao de sockets   */
/*                                             */
/* Par�metros de entrada:  socket de conex�o   */
/* Valor de retorno:       nenhum              */
/*                                             */
/***********************************************/

void CloseConnection(SOCKET connfd) {
	closesocket(connfd);
	WSACleanup();
}
/**************************************************************************/
/* Corpo do Programa                                                      */
/**************************************************************************/

int main(int argc, char **argv) {

  WSADATA     wsaData;
  SOCKET      listenfd, connfd;
  SOCKADDR_IN ServerAddr;
  SOCKADDR    MySockAddr;
  int         MySockAddrLenght;
  MySockAddrLenght = sizeof(MySockAddr);
  int optval;

  SYSTEMTIME SystemTime;

  char msgdados[TAMMSGDADOS+1];
  char msgreq[TAMMSGREQ+1];
  char msgsp[TAMMSGSP+1];
  char msgsp1[TAMMSGSP+1] = "NNNNNN$103$0010.0$1450.0$0002";
  char msgsp2[TAMMSGSP+1] = "NNNNNN$103$0009.5$1387.0$0001";
  char msgack[TAMMSGACK+1] = "NNNNNN$101";
  char msgacksp[TAMMSGACKSP+1];
  char buf[100];

  int status, port, vez = 0;
  int tecla = 0, acao;
  int nseql, nseqr;
  HANDLE hOut;

  /* Verifica parametros de linha de comando */
  if ((argc != 2) || (argc == 2 && strcmp(argv[1],"-h") == 0)){
    printf("Use: wintcpserver <port>\n");
    _exit(0);
  }
  else if (argc == 2) port=atoi(argv[1]);

  // Obt�m um handle para a sa�da da console
  hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut == INVALID_HANDLE_VALUE)
	  printf("Erro ao obter handle para a sa�da da console\n");
  SetConsoleTextAttribute(hOut, WHITE);

  /* Inicializa Winsock vers�o 2.2 */
  status = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (status != 0) {
      printf("Falha na inicializacao do Winsock 2! Erro  = %d\n", WSAGetLastError());
      WSACleanup();
      exit(0);
  }

  /* Cria socket */
  printf ("Criando socket ...\n");
  listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listenfd == INVALID_SOCKET){
	status=WSAGetLastError();
	if ( status == WSAENETDOWN)
      printf("Rede ou servidor de sockets inacess�veis!\n");
	else
	  printf("Falha na rede: codigo de erro = %d\n", status);
	  WSACleanup();
	  exit(0);
  }

  /* Permite a possibilidade de reuso deste socket, de forma que,   */
  /* se uma inst�ncia anterior deste programa tiver sido encerrada  */
  /* com CTRL-C por exemplo, n�o ocorrera' o erro "10048" ("address */
  /* already in use") na operacao de BIND                           */
  optval = 1;
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval));

  /* Inicializa estrutura sockaddr_in */
  printf ("Inicializando estruturas ...\n");
  memset(&ServerAddr, 0, sizeof(ServerAddr));
  ServerAddr.sin_family = AF_INET;
  ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  ServerAddr.sin_port = htons(port);    

  /* Vincula o socket ao endereco e porta especificados */
  printf ("Binding ...\n");
  status = bind(listenfd, (SOCKADDR *) &ServerAddr, sizeof(ServerAddr)) ;
  if (status == SOCKET_ERROR){
    printf("Falha na execucao do BIND! Erro  = %d\n", WSAGetLastError());
    WSACleanup();
    exit(0);
  }

  /* Coloca o socket em estado de escuta */
  printf ("Listening ...\n");
  status = listen(listenfd, LISTENQ);
  if (status == SOCKET_ERROR){
    printf("Falha na execucao do LISTEN! Erro  = %d\n", WSAGetLastError());
    WSACleanup();
    exit(0);
  }

  /* LOOP EXTERNO - AGUARDA CONEXOES */

  while (true) {

	nseql = 0; // Reinicia cnumera��o das mensagens

    /* Aguarda conexao do cliente */
    GetSystemTime(&SystemTime);
    printf ("Aguardando conexoes... data/hora local = %02d-%02d-%04d %02d:%02d:%02d\n",
             SystemTime.wDay, SystemTime.wMonth, SystemTime.wYear,
             SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
    connfd = accept(listenfd, &MySockAddr, &MySockAddrLenght);
    if (connfd == INVALID_SOCKET){
      printf("Falha na execucao do ACCEPT! Erro  = %d\n", WSAGetLastError());
      WSACleanup();
      exit(0);
    }
    else printf("Conexao efetuada!\n");

    /* LOOP DE TROCA DE MENSAGENS */
    for (;;) {
   
	  /* AGUARDA MENSAGEM */

	  /* Como a mensagem a receber pode ter tamanhos diferentes, a estrat�gia �  */
	  /* ler a mensagem de menor tamanho e verificar o campo de "codigo" da      */
	  /* mensagem recebida. Se este indicar que a mensagem refere-se � de maior  */
	  /* tamanho, ent�o uma segunda chamada a recv() � feita para completar o    */
	  /* buffer de mensagens.                                                    */

	  SetConsoleTextAttribute(hOut, WHITE);
	  printf ("Aguardando mensagem...\n");
      memset(buf, 0, sizeof(buf));
	  status = recv(connfd, buf, TAMMSGREQ, 0);
	  if ((acao = CheckSocketError(status, hOut)) != 0) break;
	  sscanf(buf, "%6d", &nseqr);
	  if (++nseql != nseqr) {
		  SetConsoleTextAttribute(hOut, HLRED);
		  printf("Numero sequencial de mensagem incorreto [1]: recebido %d ao inves de %d\n",
			    nseqr, nseql);
		  CloseConnection(connfd);
          SetConsoleTextAttribute(hOut, WHITE);
		  exit(0);
	  }

      /* VERIFICA O CAMPO DE "C�DIGO" DA MENSAGEM RECEBIDA */
	  if (strncmp(&buf[7],"100", 3) == 0) {

		/* MENSAGEM DE DADOS DA APLICA��O DE INTEGRA��O */
        strncpy(msgdados, buf, TAMMSGREQ+1);
		/* L� o restante da mensagem */
		status = recv(connfd, buf, TAMMSGDADOS - TAMMSGREQ, 0);
		if ((acao = CheckSocketError(status, hOut)) != 0) break;
		strncpy(&msgdados[TAMMSGREQ], buf, TAMMSGDADOS - TAMMSGREQ + 1);
		SetConsoleTextAttribute(hOut, HLGREEN);
	    printf ("\nMensagem recebida do Sist. de Desgaseificacao a vacuo:\n%s\n",
                msgdados);

		/* DEVOLVE MSG DE ACK */
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%06d", ++nseql);
        memcpy(msgack, buf, 6);
        status = send(connfd, msgack, TAMMSGACK, 0);
		if ((acao = CheckSocketError(status, hOut)) != 0) break;
		SetConsoleTextAttribute(hOut, YELLOW);
	    printf ("Mensagem de ACK enviada ao Sist. de Desgaseificacao a vacuo:\n%s\n\n",
                msgack);
	  }

	  else if (strncmp(&buf[7],"102", 3) == 0) {

		/* MENSAGEM DE SOLICITA��O DE SET-POINTS */
        strncpy(msgreq, buf, TAMMSGREQ+1);
		SetConsoleTextAttribute(hOut, HLBLUE);
	    printf ("\nMensagem de solicitacao de set-points recebida:\n%s\n", msgreq);

		/* DEVOLVE MENSAGEM COM OS SET-POINTS PARA A DESGASEIFICA��O A V�CUO  */
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%06d", ++nseql);
		if (vez == 0) {
          strcpy(msgsp, msgsp1);
        vez = 1;
        }
        else {
          strcpy(msgsp, msgsp2);
          vez = 0;
		}
        memcpy(&msgsp[0], buf, 6);
        status = send(connfd, msgsp, TAMMSGSP, 0);
		if ((acao = CheckSocketError(status, hOut)) != 0) break;
		SetConsoleTextAttribute(hOut, CYAN);
		printf ("Enviando set-points ao Sist. de Desgaseificacao a vacuo:\n%s\n", msgsp);

		/* AGUARDA MENSAGEM DE ACK DO SISTEMA DE DESGASEIFICA��O A V�CUO*/
		memset(buf, 0, sizeof(buf));
		status = recv(connfd, buf, TAMMSGACK, 0);
		if ((acao = CheckSocketError(status, hOut)) != 0) break;
		sscanf(buf, "%6d", &nseqr);
		if (++nseql != nseqr) {
			SetConsoleTextAttribute(hOut, HLRED);
			printf("Numero sequencial de mensagem incorreto [2]: recebido %d ao inves de %d\n",
				nseqr, nseql);
			CloseConnection(connfd);
            SetConsoleTextAttribute(hOut, WHITE);
			exit(0);
		}
		if (strncmp(&buf[7], "104", 3) == 0) {
			strncpy(msgacksp, buf, TAMMSGACKSP+1);
			SetConsoleTextAttribute(hOut, YELLOW);
		    printf("Mensagem de ACK recebida do Sist. de Desgaseificacao a vacuo:\n%s\n\n",
                    msgacksp);
			}
		else {
			SetConsoleTextAttribute(hOut, HLRED);
            buf[10] = '\0';
            printf("Mensagem de ACK invalida: recebido %s ao inves de '104'\n\n", &buf[7]);
			break;
		}
	  }

	  else {
        /* MENSAGEM INV�LIDA */
		SetConsoleTextAttribute(hOut, HLRED);
		buf[10] = '\0';
        printf ("MENSAGEM RECEBIDA COM CODIGO INVALIDO: %s\n\n", &buf[7]);
        break;
	  }
	  /* Testa se usuario digitou ESC e, em caso afirmativo,encerra o programa */
	  if (_kbhit() !=0)
	    if ((tecla = _getch()) == ESC) break;
    } // END Loop secundario
	if (acao == -1)	closesocket(connfd);
	else if ((acao == -2) || (tecla == ESC)) break;
  }// END Loop primario

  SetConsoleTextAttribute(hOut, WHITE);
  printf ("Encerrando o programa ...");
  CloseConnection(connfd);
  exit(0);
}
