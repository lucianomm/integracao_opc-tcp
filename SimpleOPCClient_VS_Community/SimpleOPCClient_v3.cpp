// Simple OPC Client
//
// This is a modified version of the "Simple OPC Client" originally
// developed by Philippe Gras (CERN) for demonstrating the basic techniques
// involved in the development of an OPC DA client.
//
// The modifications are the introduction of two C++ classes to allow the
// the client to ask for callback notifications from the OPC server, and
// the corresponding introduction of a message comsumption loop in the
// main program to allow the client to process those notifications. The
// C++ classes implement the OPC DA 1.0 IAdviseSink and the OPC DA 2.0
// IOPCDataCallback client interfaces, and in turn were adapted from the
// KEPWARE´s  OPC client sample code. A few wrapper functions to initiate
// and to cancel the notifications were also developed.
//
// The original Simple OPC Client code can still be found (as of this date)
// in
//        http://pgras.home.cern.ch/pgras/OPCClientTutorial/
//
//
// Luiz T. S. Mendes - DELT/UFMG - 15 Sept 2011
// luizt at cpdee.ufmg.br
//

/* ======================================================= */
/* INCLUDE AREA */

#include <atlbase.h>    // required for using the "_T" macro
#include <iostream>
#include <ObjIdl.h>

#include "opcda.h"
#include "opcerror.h"
#include "SimpleOPCClient_v3.h"
#include "SOCAdviseSink.h"
#include "SOCDataCallback.h"                                                                                                                                                                                                                                                                                                                                                                                                                                                      
#include "SOCWrapperFunctions.h"
#include <winsock2.h>
#include <stdio.h>
#include <string>
#include "MessageHandling.h"

#include <windows.h>
#include <thread>
#include <string.h>
#include <stdlib.h>
#include <conio.h>		
#include <ws2tcpip.h>
#include <math.h>                                               
#include <mutex>

#include "opcda.h"
#include "opcerror.h"
#include "SimpleOPCClient_v3.h"
#include "SOCDataCallback.h"
#include "SOCWrapperFunctions.h"

/* ======================================================= */
/* NAMESPACE AREA */

using namespace std;

/* ======================================================= */
/* PRAGMA AREA */
#pragma warning(disable:6031)
#pragma warning(disable:6385)
#pragma warning(disable:4996)
#pragma once


/* ======================================================= */
/* PROTOTIPE AREA */

int CheckSocketError(int status, HANDLE hOut);
void checkAndIncreaseSequenceNumber(const char* message);
bool sendMessage(string message);
bool receiveProcessComputerACK();

/* ======================================================= */
/* DEFINE AREA */

#define OPC_SERVER_NAME			L"Matrikon.OPC.Simulation.1"
#define	VT						VT_R4

#define SETPOINT_MESSAGE_SIZE	29
#define ACK_MESSAGE_SIZE		10

#define ESC						0x1B

#define WHITE   FOREGROUND_RED   | FOREGROUND_GREEN      | FOREGROUND_BLUE  | FOREGROUND_INTENSITY
#define HLGREEN FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define HLRED   FOREGROUND_RED   | FOREGROUND_INTENSITY
#define HLBLUE  FOREGROUND_BLUE  | FOREGROUND_INTENSITY
#define YELLOW  FOREGROUND_RED   | FOREGROUND_GREEN
#define CYAN    FOREGROUND_BLUE  | FOREGROUND_GREEN      | FOREGROUND_INTENSITY

#define ANSI_COLOR_RED			"\x1b[31m"
#define ANSI_COLOR_GREEN		"\x1b[32m"
#define ANSI_COLOR_YELLOW		"\x1b[33m"
#define ANSI_COLOR_BLUE			"\x1b[34m"

#define PORT 3842

//#define REMOTE_SERVER_NAME L"your_path"

// Global variables

// The OPC DA Spec requires that some constants be registered in order to use
// them. The one below refers to the OPC DA 1.0 IDataObject interface.
UINT OPC_DATA_TIME = RegisterClipboardFormat (_T("OPCSTMFORMATDATATIME"));

/* ======================================================= */
/* GLOBAL VARIABLE */

wchar_t ITEM_ID_TEMP_PANELA[] = L"Random.Real4"; // Temperatura na panela de aco (K)
wchar_t ITEM_ID_TEMP_CAMARA[] = L"Saw-toothed Waves.Real4"; // Temperatura na camara a vacuo (K)
wchar_t ITEM_ID_PRES_ARGONIO[] = L"Triangle Waves.Real4"; // Pressao de injecao de gas argonio (mmHg)
wchar_t ITEM_ID_PRES_CAMARA[] = L"Square Waves.Real4"; // Pressao na camara a vacuo (mmHg) 

wchar_t ITEM_ID_SP_PRES_ARGONIO[] = L"Bucket Brigade.Real8"; // Set-point de temperatura de injecao de gas argonio
wchar_t ITEM_ID_SP_TEMP_CAMARA[] = L"Bucket Brigade.Real4"; // Set-point de temperatura na camara a vacuo
wchar_t ITEM_ID_SP_PRES_CAMARA[] = L"Bucket Brigade.Int4"; // Set-point de pressao na camara a vacuo

int status, acao, sequenceNumber = 1; 

VARIANT* sLeitura;
OPCHANDLE* sHandleLeitura;

IOPCServer* pIOPCServer = NULL;   //pointer to IOPServer interface
IOPCItemMgt* pIOPCItemMgt = NULL; //pointer to IOPCItemMgt interface

OPCHANDLE hITEM_ID_SP_PRES_ARGONIO;
OPCHANDLE hITEM_ID_SP_TEMP_CAMARA;
OPCHANDLE hITEM_ID_SP_PRES_CAMARA;

char	msg_setpoint[SETPOINT_MESSAGE_SIZE + 1] = "000000$103$0000.0$0000.0$0000";

// Variável de dados do processo
MessageHandling processDataMessageOPC;

// Variável para compartilhamento de setpoints
MessageHandling setpointsMessageOPC(msg_setpoint);

// socket utilizado para comunicação tcp/ip
SOCKET      s;

HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
// Mutex para envio e recebimento de mensagens
mutex hMutexSendAndReceiveMessage;
// Mutex para alterar dados de processo
mutex mutexProcessData;
// Semáforo para permitir aplicar setpoints
HANDLE shouldSetVariables;
// Evento da tecla ESC
HANDLE hEscEvent;

//////////////////////////////////////////////////////////////////////
// Read the value of an item on an OPC server. 

void opcClient()
{	
	int id = 1;
	//DWORD dwSocketsClient;
	//HANDLE hSocketsClient = CreateThread(NULL, 0, socket_client, (LPVOID)id, 0, &dwSocketsClient);
	//if (dwSocketsClient){
	//	printf("Thread %d criada com sucesso -> Id = %0d \n", id, dwSocketsClient);
	//}
	DWORD ret;
	float SP_PRES_ARGONIO,
		  SP_TEMP_CAMARA;
	int   SP_PRES_CAMARA;

	float	aux1,
			aux2,
			aux3,
			aux4;
	char    c_aux1[8],
			c_aux2[8],
			c_aux3[8],
			c_aux4[8];
	char    c_aux1_p[8] = "NNNN.N",
			c_aux2_p[8] = "NNNN.N",
			c_aux3_p[8] = "NNNN.N",
			c_aux4_p[8] = "NNNN.N";
	char	c_aux100[5] = "100";
	char    c_auxNumero[8] = "NNNNNN";

	OPCHANDLE hServerGroup; // server handle to the group
	
	OPCHANDLE hServer_ITEM_ID_TEMP_PANELA; // Temperatura na panela de aco (K)
	OPCHANDLE hServer_ITEM_ID_TEMP_CAMARA; // Temperatura na camara a vacuo (K)
	OPCHANDLE hServer_ITEM_ID_PRES_ARGONIO; // Pressao de injecao de gas argonio (mmHg)
	OPCHANDLE hServer_ITEM_ID_PRES_CAMARA; // Pressao na camara a vacuo (mmHg) 

	char buf[100];

	// Have to be done before using microsoft COM library:
	printf("Initializing the COM environment...\n");
	CoInitialize(NULL);

	// Let's instantiante the IOPCServer interface and get a pointer of it:
	printf("Instantiating the MATRIKON OPC Server for Simulation...\n");
	pIOPCServer = InstantiateServer(OPC_SERVER_NAME);
	
	// Add the OPC group the OPC server and get an handle to the IOPCItemMgt
	//interface:
	printf("Adding a group in the INACTIVE state for the moment...\n");
	AddTheGroup(pIOPCServer, pIOPCItemMgt, hServerGroup);

	// Add the OPC item. First we have to convert from wchar_t* to char*
	// in order to print the item name in the console.
    size_t m;
	wcstombs_s(&m, buf, 100, ITEM_ID_TEMP_PANELA, _TRUNCATE);
	printf("Adding the item %s to the group...\n", buf);
	wcstombs_s(&m, buf, 100, ITEM_ID_TEMP_CAMARA, _TRUNCATE);
	printf("Adding the item %s to the group...\n", buf);
	wcstombs_s(&m, buf, 100, ITEM_ID_PRES_ARGONIO, _TRUNCATE);
	printf("Adding the item %s to the group...\n", buf);
	wcstombs_s(&m, buf, 100, ITEM_ID_PRES_CAMARA, _TRUNCATE);
	printf("Adding the item %s to the group...\n", buf);
	wcstombs_s(&m, buf, 100, ITEM_ID_SP_PRES_ARGONIO, _TRUNCATE);
	printf("Adding the item %s to the group...\n", buf);
	wcstombs_s(&m, buf, 100, ITEM_ID_SP_TEMP_CAMARA, _TRUNCATE);
	printf("Adding the item %s to the group...\n", buf);
	wcstombs_s(&m, buf, 100, ITEM_ID_SP_PRES_CAMARA, _TRUNCATE);
	printf("Adding the item %s to the group...\n", buf);

	AddTheItem(pIOPCItemMgt, hServer_ITEM_ID_TEMP_PANELA,
							 hServer_ITEM_ID_TEMP_CAMARA,
							 hServer_ITEM_ID_PRES_ARGONIO,
							 hServer_ITEM_ID_PRES_CAMARA,
							 hITEM_ID_SP_PRES_ARGONIO,
							 hITEM_ID_SP_TEMP_CAMARA,
							 hITEM_ID_SP_PRES_CAMARA);

	int bRet;
	MSG msg;
	
	// Establish a callback asynchronous read by means of the IOPCDaraCallback
	// (OPC DA 2.0) method. We first instantiate a new SOCDataCallback object and
	// adjusts its reference count, and then call a wrapper function to
	// setup the callback.
	IConnectionPoint* pIConnectionPoint = NULL; //pointer to IConnectionPoint Interface
	DWORD dwCookie = 0;
	SOCDataCallback* pSOCDataCallback = new SOCDataCallback();
	pSOCDataCallback->AddRef();

	printf("Setting up the IConnectionPoint callback connection...\n");
	SetDataCallback(pIOPCItemMgt, pSOCDataCallback, pIConnectionPoint, &dwCookie);

	// Change the group to the ACTIVE state so that we can receive the
	// server´s callback notification
	printf("Changing the group state to ACTIVE...\n");
    SetGroupActive(pIOPCItemMgt); 
	
	// Enter again a message pump in order to process the server´s callback
	// notifications, for the same reason explained before.
	printf("Waiting for IOPCDataCallback notifications...\n\n");

	VARIANT aux;
	::VariantInit(&aux);

	while (true) {
		bRet = GetMessage(&msg, NULL, 0, 0);
		if (!bRet) {
			printf("Failed to get windows message! Error code = %d\n", GetLastError());
			exit(0);
		}

		TranslateMessage(&msg); // This call is not really needed ...
		DispatchMessage(&msg);  // ... but this one is!

		sLeitura = pSOCDataCallback->sendValues();
		sHandleLeitura = pSOCDataCallback->sendHandles();

		for (int i = 0; i < 4; i++) {
			VarToStr(sLeitura[i], buf);
		}

		char mensagem[30];
	
		VarToStr(sLeitura[0], buf);
		aux1 = atof(buf);
		if (aux1 < 0) aux1 = -1 * aux1;
		if (aux1 > 10000) aux1 = aux1 / 10;
		sprintf(c_aux1_p, "%07.2f", aux1);
		if (c_aux1_p[0] == '-') c_aux1_p[0] = '0';
		sprintf(c_aux1, "%c%c%c%c%c%c", c_aux1_p[0], c_aux1_p[1], c_aux1_p[2], c_aux1_p[3], c_aux1_p[4], c_aux1_p[5]);

		VarToStr(sLeitura[1], buf);
		aux2 = atof(buf);
		if (aux2 < 0) aux2 = -1 * aux2;
		if (aux2 > 10000) aux2 = aux2 / 10;
		sprintf(c_aux2_p, "%07.2f", aux2);
		if (c_aux2_p[0] == '-') c_aux2_p[0] = '0';
		sprintf(c_aux2, "%c%c%c%c%c%c", c_aux2_p[0], c_aux2_p[1], c_aux2_p[2], c_aux2_p[3], c_aux2_p[4], c_aux2_p[5]);
		VarToStr(sLeitura[2], buf);

		aux3 = atof(buf);
		if (aux3 < 0) aux3 = -1 * aux3;
		if (aux3 > 10000) aux3 = aux3 / 10;
		sprintf(c_aux3_p, "%07.2f", aux3);
		if (c_aux3_p[0] == '-') c_aux3_p[0] = '0';
		sprintf(c_aux3, "%c%c%c%c%c%c", c_aux3_p[0], c_aux3_p[1], c_aux3_p[2], c_aux3_p[3], c_aux3_p[4], c_aux3_p[5]);
		VarToStr(sLeitura[3], buf);

		aux4 = atof(buf);
		if (aux4 < 0) aux4 = -1 * aux4;
		if (aux4 > 10000) aux4 = aux4 / 10;
		sprintf(c_aux4_p, "%07.2f", aux4);
		if (c_aux4_p[0] == '-') c_aux4_p[0] = '0';
		sprintf(c_aux4, "%c%c%c%c%c%c", c_aux4_p[0], c_aux4_p[1], c_aux4_p[2], c_aux4_p[3], c_aux4_p[4], c_aux4_p[5]);
		
		printf(ANSI_COLOR_GREEN "Mensagem lida por callback do server OPC:");
		printf("%s$%s$%s$%s$%s$%s\n\n ", c_auxNumero, c_aux100, c_aux1, c_aux2, c_aux3, c_aux4);

		ret = WaitForSingleObject(shouldSetVariables, 10);

		if(!(ret - WAIT_OBJECT_0)) {
			SP_PRES_ARGONIO = setpointsMessageOPC.getGasInjectionPressureSP();
			SP_TEMP_CAMARA = setpointsMessageOPC.getVaccumChamberTemperatureSP();
			SP_PRES_CAMARA   =	setpointsMessageOPC.getVaccumChamberPressureSP();
			
			aux.vt = VT_R8;
			aux.dblVal = SP_PRES_ARGONIO;
			WriteItem(pIOPCItemMgt, hITEM_ID_SP_PRES_ARGONIO, aux);
			printf(ANSI_COLOR_YELLOW "Set-point de pressao de injecao de gas argonio: %06.1f\n", SP_PRES_ARGONIO);


			aux.vt = VT_R4;
			aux.fltVal = SP_TEMP_CAMARA;
			WriteItem(pIOPCItemMgt, hITEM_ID_SP_TEMP_CAMARA, aux);
			printf(ANSI_COLOR_YELLOW "Set-point de temperatura na camara a vacuo: %06.1f\n", SP_TEMP_CAMARA);
			
			aux.vt = VT_I4;
			aux.intVal = SP_PRES_CAMARA;
			WriteItem(pIOPCItemMgt, hITEM_ID_SP_PRES_CAMARA, aux);
			printf(ANSI_COLOR_YELLOW "Set-point de pressao na camara a vacuo: %04d\n\n", SP_PRES_CAMARA);
		}
		mutexProcessData.lock();
		GetLastError();
		processDataMessageOPC.setProcessDataMessage(aux1, aux2, aux3, aux4);
		mutexProcessData.unlock();
		GetLastError();

		ret = WaitForSingleObject(hEscEvent, 0);
		if (!(ret - WAIT_OBJECT_0)) break;
	}

	// Cancel the callback and release its reference
	printf("Cancelling the IOPCDataCallback notifications...\n");
    CancelDataCallback(pIConnectionPoint, dwCookie);
	//pIConnectionPoint->Release();
	pSOCDataCallback->Release();

	// Remove the OPC item:
	printf("Removing the OPC item...\n");
	RemoveItem(pIOPCItemMgt, hServer_ITEM_ID_TEMP_PANELA,
							 hServer_ITEM_ID_TEMP_CAMARA,
							 hServer_ITEM_ID_PRES_ARGONIO,
							 hServer_ITEM_ID_PRES_CAMARA,
							 hITEM_ID_SP_PRES_ARGONIO,
							 hITEM_ID_SP_TEMP_CAMARA,
							 hITEM_ID_SP_PRES_CAMARA);
	
	// Remove the OPC group:
	printf("Removing the OPC group object...\n");
    pIOPCItemMgt->Release();
	RemoveGroup(pIOPCServer, hServerGroup);

	// release the interface references:
	printf("Removing the OPC server object...\n");
	pIOPCServer->Release();

	//close the COM library:
	printf ("Releasing the COM environment...\n");
	CoUninitialize();

	return;
}

////////////////////////////////////////////////////////////////////
// Instantiate the IOPCServer interface of the OPCServer
// having the name ServerName. Return a pointer to this interface
//
IOPCServer* InstantiateServer(wchar_t ServerName[])
{
	CLSID CLSID_OPCServer;
	HRESULT hr;

	// get the CLSID from the OPC Server Name:
	hr = CLSIDFromString(ServerName, &CLSID_OPCServer);
	_ASSERT(!FAILED(hr));


	//queue of the class instances to create
	LONG cmq = 1; // nbr of class instance to create.
	MULTI_QI queue[1] =
		{{&IID_IOPCServer,
		NULL,
		0}};

	//Server info:
	//COSERVERINFO CoServerInfo =
    //{
	//	/*dwReserved1*/ 0,
	//	/*pwszName*/ REMOTE_SERVER_NAME,
	//	/*COAUTHINFO*/  NULL,
	//	/*dwReserved2*/ 0
    //}; 

	// create an instance of the IOPCServer
	hr = CoCreateInstanceEx(CLSID_OPCServer, NULL, CLSCTX_SERVER,
		/*&CoServerInfo*/NULL, cmq, queue);
	_ASSERT(!hr);

	// return a pointer to the IOPCServer interface:
	return(IOPCServer*) queue[0].pItf;
}


/////////////////////////////////////////////////////////////////////
// Add group "Group1" to the Server whose IOPCServer interface
// is pointed by pIOPCServer. 
// Returns a pointer to the IOPCItemMgt interface of the added group
// and a server opc handle to the added group.
//
void AddTheGroup(IOPCServer* pIOPCServer, IOPCItemMgt* &pIOPCItemMgt, 
				 OPCHANDLE& hServerGroup)
{
	DWORD dwUpdateRate = 0;
	OPCHANDLE hClientGroup = 0;

	// Add an OPC group and get a pointer to the IUnknown I/F:
    HRESULT hr = pIOPCServer->AddGroup(/*szName*/ L"Group1",
		/*bActive*/ FALSE,
		/*dwRequestedUpdateRate*/ 1000,
		/*hClientGroup*/ hClientGroup,
		/*pTimeBias*/ 0,
		/*pPercentDeadband*/ 0,
		/*dwLCID*/0,
		/*phServerGroup*/&hServerGroup,
		&dwUpdateRate,
		/*riid*/ IID_IOPCItemMgt,
		/*ppUnk*/ (IUnknown**) &pIOPCItemMgt);
	_ASSERT(!FAILED(hr));
}



//////////////////////////////////////////////////////////////////
// Add the Item ITEM_ID to the group whose IOPCItemMgt interface
// is pointed by pIOPCItemMgt pointer. Return a server opc handle
// to the item.
 
void AddTheItem(IOPCItemMgt* pIOPCItemMgt, OPCHANDLE& hServer_ITEM_ID_TEMP_PANELA,
										   OPCHANDLE& hServer_ITEM_ID_TEMP_CAMARA,
	                                       OPCHANDLE& hServer_ITEM_ID_PRES_ARGONIO,
	                                       OPCHANDLE& hServer_ITEM_ID_PRES_CAMARA,
	                                       OPCHANDLE& hITEM_ID_SP_PRES_ARGONIO,
	                                       OPCHANDLE& hITEM_ID_SP_TEMP_CAMARA,
	                                       OPCHANDLE& hITEM_ID_SP_PRES_CAMARA)
{
	HRESULT hr;

	// Array of items to add:
	OPCITEMDEF ItemArray[7] =
	{
		{
			/*szAccessPath*/ L"",
			/*szItemID*/ ITEM_ID_TEMP_PANELA,
			/*bActive*/ TRUE,
			/*hClient*/ 0,
			/*dwBlobSize*/ 0,
			/*pBlob*/ NULL,
			/*vtRequestedDataType*/ VT,
			/*wReserved*/0
		},
	
		{
			/*szAccessPath*/ L"",
			/*szItemID*/ ITEM_ID_TEMP_CAMARA,
			/*bActive*/ TRUE,
			/*hClient*/ 1,
			/*dwBlobSize*/ 0,
			/*pBlob*/ NULL,
			/*vtRequestedDataType*/ VT,
			/*wReserved*/0
		},

		{
			/*szAccessPath*/ L"",
			/*szItemID*/ ITEM_ID_PRES_ARGONIO,
			/*bActive*/ TRUE,
			/*hClient*/ 2,
			/*dwBlobSize*/ 0,
			/*pBlob*/ NULL,
			/*vtRequestedDataType*/ VT,
			/*wReserved*/0
		},

		{
			/*szAccessPath*/ L"",
			/*szItemID*/ ITEM_ID_PRES_CAMARA,
			/*bActive*/ TRUE,
			/*hClient*/ 3,
			/*dwBlobSize*/ 0,
			/*pBlob*/ NULL,
			/*vtRequestedDataType*/ VT,
			/*wReserved*/0
		},

		{
			/*szAccessPath*/ L"",
			/*szItemID*/ ITEM_ID_SP_PRES_ARGONIO,
			/*bActive*/ TRUE,
			/*hClient*/ 4,
			/*dwBlobSize*/ 0,
			/*pBlob*/ NULL,
			/*vtRequestedDataType*/ VT,
			/*wReserved*/0
		},
		
		{
			/*szAccessPath*/ L"",
			/*szItemID*/ ITEM_ID_SP_TEMP_CAMARA,
			/*bActive*/ TRUE,
			/*hClient*/ 5,
			/*dwBlobSize*/ 0,
			/*pBlob*/ NULL,
			/*vtRequestedDataType*/ VT,
			/*wReserved*/0
		},

		{
			/*szAccessPath*/ L"",
			/*szItemID*/ ITEM_ID_SP_PRES_CAMARA,
			/*bActive*/ TRUE,
			/*hClient*/ 6,
			/*dwBlobSize*/ 0,
			/*pBlob*/ NULL,
			/*vtRequestedDataType*/ VT,
			/*wReserved*/0
		},

	};

	//Add Result:
	OPCITEMRESULT* pAddResult=NULL;
	HRESULT* pErrors = NULL;

	// Add an Item to the previous Group:
	hr = pIOPCItemMgt->AddItems(7, ItemArray, &pAddResult, &pErrors);
	if (hr != S_OK){
		printf("Failed call to AddItems function. Error code = %x\n", hr);
		exit(0);
	}

	// Server handle for the added item:
	hServer_ITEM_ID_TEMP_PANELA = pAddResult[0].hServer;
	hServer_ITEM_ID_TEMP_CAMARA = pAddResult[1].hServer;
	hServer_ITEM_ID_PRES_ARGONIO = pAddResult[2].hServer;
	hServer_ITEM_ID_PRES_CAMARA = pAddResult[3].hServer;
	hITEM_ID_SP_PRES_ARGONIO = pAddResult[4].hServer;
	hITEM_ID_SP_TEMP_CAMARA = pAddResult[5].hServer;
	hITEM_ID_SP_PRES_CAMARA = pAddResult[6].hServer;

	// release memory allocated by the server:
	CoTaskMemFree(pAddResult->pBlob);

	CoTaskMemFree(pAddResult);
	pAddResult = NULL;

	CoTaskMemFree(pErrors);
	pErrors = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Read from device the value of the item having the "hServerItem" server 
// handle and belonging to the group whose one interface is pointed by
// pGroupIUnknown. The value is put in varValue. 
//
void WriteItem(IUnknown* pGroupIUnknown, OPCHANDLE hServerItem, VARIANT& varValue)
{
	//get a pointer to the IOPCSyncIOInterface:
	IOPCSyncIO* pIOPCSyncIO;
	pGroupIUnknown->QueryInterface(__uuidof(pIOPCSyncIO), (void**) &pIOPCSyncIO);

	// write the item value from the device:
	
	HRESULT* pErrors = NULL;
	
	HRESULT hr = pIOPCSyncIO->Write(1, &hServerItem, &varValue, &pErrors);
	if (hr != S_OK) {
		printf("Failed to send message %x.\n", hr);
		exit(0);
	}

	//Release memeory allocated by the OPC server:
	CoTaskMemFree(pErrors);
	pErrors = NULL;

	// release the reference to the IOPCSyncIO interface:
	pIOPCSyncIO->Release();
}

///////////////////////////////////////////////////////////////////////////
// Remove the item whose server handle is hServerItem from the group
// whose IOPCItemMgt interface is pointed by pIOPCItemMgt
//
void RemoveItem(IOPCItemMgt* pIOPCItemMgt, OPCHANDLE hServer_ITEM_ID_TEMP_PANELA,
										   OPCHANDLE hServer_ITEM_ID_TEMP_CAMARA,
	                                       OPCHANDLE hServer_ITEM_ID_PRES_ARGONIO,
	                                       OPCHANDLE hServer_ITEM_ID_PRES_CAMARA,
	                                       OPCHANDLE hITEM_ID_SP_PRES_ARGONIO,
	                                       OPCHANDLE hITEM_ID_SP_TEMP_CAMARA,
	                                       OPCHANDLE hITEM_ID_SP_PRES_CAMARA)
{

	
	// server handle of items to remove:
	OPCHANDLE hServerArray[7];
	hServerArray[0] = hServer_ITEM_ID_TEMP_PANELA;
	hServerArray[1] = hServer_ITEM_ID_TEMP_CAMARA;
	hServerArray[2] = hServer_ITEM_ID_PRES_ARGONIO;
	hServerArray[3] = hServer_ITEM_ID_PRES_CAMARA;
	hServerArray[4] = hITEM_ID_SP_PRES_ARGONIO;
	hServerArray[5] = hITEM_ID_SP_TEMP_CAMARA;
	hServerArray[6] = hITEM_ID_SP_PRES_CAMARA;
	
	//Remove the item:
	HRESULT* pErrors; // to store error code(s)
	HRESULT hr = pIOPCItemMgt->RemoveItems(7, hServerArray, &pErrors);
	_ASSERT(!hr);

	//release memory allocated by the server:
	CoTaskMemFree(pErrors);
	pErrors = NULL;
}

////////////////////////////////////////////////////////////////////////
// Remove the Group whose server handle is hServerGroup from the server
// whose IOPCServer interface is pointed by pIOPCServer
//
void RemoveGroup (IOPCServer* pIOPCServer, OPCHANDLE hServerGroup)
{
	// Remove the group:
	HRESULT hr = pIOPCServer->RemoveGroup(hServerGroup, FALSE);
	if (hr != S_OK){
		if (hr == OPC_S_INUSE)
			printf ("Failed to remove OPC group: object still has references to it.\n");
		else printf ("Failed to remove OPC group. Error code = %x\n", hr);
		exit(0);
	}
}

/**************************************************************************/
/* Função para testar o código de erro na comunicação via sockets         */
/*                                                                        */
/* Parâmetros de entrada:                                                 */
/*     status - código devolvido pela função de sockets chamada           */
/*                                                                        */
/*                                                                        */
/* Valor de retorno: 0 se não houve erro                                  */
/*                  -1 se o erro for recuperável                          */
/*                  -2 se o erro for irrecuperável                        */
/**************************************************************************/

int CheckSocketError(int status, HANDLE hOut) {
	int erro;

	if (status == SOCKET_ERROR) {
		SetConsoleTextAttribute(hOut, HLRED);
		erro = WSAGetLastError();
		if (erro == WSAEWOULDBLOCK) {
			printf("Timeout na operacao de RECV! errno = %d - reiniciando...\n\n", erro);
			return(-1); // acarreta reinício da espera de mensagens no programa principal
		}
		else if (erro == WSAECONNABORTED) {
			printf("Conexao abortada pelo cliente TCP - reiniciando...\n\n");
			return(-1); // acarreta reinício da espera de mensagens no programa principal
		}
		else {
			printf("Erro de conexao! valor = %d\n\n", erro);
			return (-2); // acarreta encerramento do programa principal
		}
	}
	else if (status == 0) {
		//Este caso indica que a conexão foi encerrada suavemente ("gracefully")
		printf("Conexao com cliente TCP encerrada prematuramente! status = %d\n\n", status);
		return(-1); // acarreta reinício da espera de mensagens no programa principal
	}
	else return(0);
}

void checkAndIncreaseSequenceNumber(const char* message) {
	int actualSequenceNumber;
	sscanf(message, "%6d", &actualSequenceNumber);
	if (sequenceNumber++ != actualSequenceNumber) {
		SetConsoleTextAttribute(hOut, HLRED);
		printf("Numero sequencial de mensagem incorreto [1]: observado %d ao inves de %d\n",
			actualSequenceNumber, sequenceNumber);
		SetConsoleTextAttribute(hOut, WHITE);
		exit(0);
	}
}

bool sendMessage(string message) {
	checkAndIncreaseSequenceNumber(message.c_str());
	status = send(s, message.c_str(), message.size(), 0);
	return ((acao = CheckSocketError(status, hOut)) != 0);
}

bool receiveProcessComputerACK() {
	char buf[ACK_MESSAGE_SIZE + 1];
	status = recv(s, buf, ACK_MESSAGE_SIZE, 0);
	if ((acao = CheckSocketError(status, hOut)) != 0) {
		return true;
	}
	checkAndIncreaseSequenceNumber(buf);
	if (strncmp(&buf[7], "101", 3) != 0) {
		SetConsoleTextAttribute(hOut, HLRED);
		buf[10] = '\0';
		printf("Mensagem de ACK invalida: recebido %s ao inves de '101'\n\n", &buf[7]);
		exit(0);
	}
	return ((acao = CheckSocketError(status, hOut)) != 0);
}

void receiveSetpoints() {
	status = recv(s, msg_setpoint, SETPOINT_MESSAGE_SIZE, 0);
	if ((acao = CheckSocketError(status, hOut)) != 0) {
		return;
	}
	checkAndIncreaseSequenceNumber(msg_setpoint);
	if (strncmp(&msg_setpoint[7], "103", 3) != 0) {
		SetConsoleTextAttribute(hOut, HLRED);
		msg_setpoint[10] = '\0';
		printf("Mensagem de Setpoints invalida: recebido %s ao inves de '103'\n\n", &msg_setpoint[7]);
		exit(0);
	}
	setpointsMessageOPC.UpdateMessageFromString(msg_setpoint);
	ReleaseSemaphore(shouldSetVariables, 1, NULL); // Letting OPC know it can set variables
}

void setpointsRequestAndReceive() {
	if (sendMessage(SetPointRequestMessage(sequenceNumber))) {
		printf("Erro ao enviar requisição de Set Points");
		return;
	}
	receiveSetpoints();
	if (sendMessage(SetPointAckMessage(sequenceNumber))) {
		printf("Erro ao enviar ACK de Set Points");
		return;
	}
}

void CALLBACK sendProcessDataMessage(PVOID lpParameter, BOOLEAN TimerOrWaitFired) {
	hMutexSendAndReceiveMessage.lock();
	GetLastError();
	mutexProcessData.lock();
	GetLastError();
	processDataMessageOPC.setSequenceNumber(sequenceNumber);
	if (sendMessage(processDataMessageOPC.toString())) {
		printf("Erro ao enviar dado de processo");
		mutexProcessData.unlock();
		hMutexSendAndReceiveMessage.unlock();
		return;
	}
	if (receiveProcessComputerACK()) {
		printf("Erro de socket ao receber ACK");
		mutexProcessData.unlock();
		hMutexSendAndReceiveMessage.unlock();
		return;
	}
	mutexProcessData.unlock();
	hMutexSendAndReceiveMessage.unlock();
}

void ConnectToServer(SOCKADDR_IN ServerAddr) {
	status = connect(s, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr));
	if (status == SOCKET_ERROR) {
		printf("Falha na conexao ao servidor ! Erro  = %d\n", WSAGetLastError());
		WSACleanup();
		exit(0);
	}
}

void socketClient(void) {

	/*************************************************
		SETTING UP SOCKET CLIENT
	*************************************************/

	const char* ipaddr = "127.0.0.1";
	WSADATA     wsaData;
	SOCKADDR_IN ServerAddr;

	status = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (status != 0) {
		printf("Falha na inicializacao do Winsock 2! Erro  = %d\n", WSAGetLastError());
		WSACleanup();
		exit(0);
	}

	// Cria um novo socket para estabelecer a conexão.
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) {
		status = WSAGetLastError();
		if (status == WSAENETDOWN)
			printf("Rede ou servidor de sockets inacessíveis!\n");
		else
			printf("Falha na rede: codigo de erro = %d\n", status);
		WSACleanup();
		exit(0);
	}

	// Inicializa a estrutura SOCKADDR_IN que será utilizada para
	// a conexão ao servidor.
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(PORT);
	ServerAddr.sin_addr.s_addr = inet_addr(ipaddr);

	// Estabelece a conexão com o servidor
	ConnectToServer(ServerAddr);

	/*************************************************
	CREATING TIMERS
	*************************************************/

	HANDLE hTimer = NULL;
	HANDLE hTimerQueue = NULL;

	// Create the timer queue.
	hTimerQueue = CreateTimerQueue();
	if (NULL == hTimerQueue)
	{
		printf("CreateTimerQueue failed (%d)\n", GetLastError());
		exit(0);
	}

	// Sends process data message every 2 seconds
	if (!CreateTimerQueueTimer(&hTimer, hTimerQueue,
		(WAITORTIMERCALLBACK)sendProcessDataMessage, NULL, 0, 2000, 0))
	{
		printf("CreateTimerQueueTimer failed (%d)\n", GetLastError());
		exit(0);
	}

	/*************************************************
	Loop for reading keyboard keys
	*************************************************/
	DWORD ret;
	while (1) {
		if (acao == -1) {
			closesocket(s); // close and reconnect
			ConnectToServer(ServerAddr);
		}
		else if ((acao == -2)) break;
		ret = WaitForSingleObject(hEscEvent, 0);
		if (!(ret - WAIT_OBJECT_0)) break;
	}
}

/*
	- Create mutexes
	- Start OPC client thread
	- Start socket client thread
*/

void keyboardRead() {
	char keyPressed = 0;
	while (keyPressed != ESC) { // TODO: while ESC not typed
		keyPressed = _getch();
		if (keyPressed == 'S' || keyPressed == 's') {
			hMutexSendAndReceiveMessage.lock();
			GetLastError();
			setpointsRequestAndReceive();
			hMutexSendAndReceiveMessage.unlock();
		}
	}
	SetEvent(hEscEvent);
}

	/*************************************************
		- Starts semaphores and events
		- Starts threads
		- Wait for threads to finish
	*************************************************/
void main(void) {
	/*************************************************
		Starting semaphores and events
	*************************************************/

	shouldSetVariables = CreateSemaphore(
		NULL,
		0,
		1,
		NULL
	);

	if (shouldSetVariables == NULL)
	{
		printf("CreateMutex error: %d\n", GetLastError());
		return;
	}

	hEscEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	thread opc_thread(opcClient);
	thread socket(socketClient);
	thread keyboard(keyboardRead);
	keyboard.join();
	opc_thread.join();
	socket.join();
}