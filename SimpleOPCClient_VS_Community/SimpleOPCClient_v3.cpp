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
#include "SOCDataCallback.h"                                                                                                                                                                                                                                                                                                                                                                                                                                                      e "SOCWrapperFunctions.h"
#include <winsock2.h>
#include <stdio.h>
#include <string>

#include <windows.h>
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
/* DEFINE AREA */

#define OPC_SERVER_NAME			L"Matrikon.OPC.Simulation.1"
#define	VT						VT_R4

#define TAM_MSG_DADOS			38
#define TAM_MSG_CONFIRMACAO		10
#define TAM_MSG_SOLICITACAO		10
#define TAM_MSG_SETPOINT		29
#define TAM_MSG_ACK				10

#define ESC						0x1B

#define WHITE   FOREGROUND_RED   | FOREGROUND_GREEN      | FOREGROUND_BLUE  | FOREGROUND_INTENSITY
#define HLGREEN FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define HLRED   FOREGROUND_RED   | FOREGROUND_INTENSITY
#define HLBLUE  FOREGROUND_BLUE  | FOREGROUND_INTENSITY
#define YELLOW  FOREGROUND_RED   | FOREGROUND_GREEN
#define CYAN    FOREGROUND_BLUE  | FOREGROUND_GREEN      | FOREGROUND_INTENSITY

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

int N_SEQ = 0; // Numero referente a mensagem sequencial

VARIANT* sLeitura;
OPCHANDLE* sHandleLeitura;

char* mensagem = new char;

IOPCServer* pIOPCServer = NULL;   //pointer to IOPServer interface
IOPCItemMgt* pIOPCItemMgt = NULL; //pointer to IOPCItemMgt interface

OPCHANDLE hITEM_ID_SP_PRES_ARGONIO;
OPCHANDLE hITEM_ID_SP_TEMP_CAMARA;
OPCHANDLE hITEM_ID_SP_PRES_CAMARA;

char	msg_dados[TAM_MSG_DADOS + 1] = "NNNNNN$100$NNNN.N$NNNN.N$NNNN.N$NNNN.N",
		msg_confirmacao[TAM_MSG_CONFIRMACAO + 1] = "NNNNNN$101",
		msg_solicitacao[TAM_MSG_SOLICITACAO + 1] = "NNNNNN$102",
		msg_setpoint[TAM_MSG_SETPOINT + 1] = "NNNNNN$103$NNNN.N$NNNN.N$NNNN",
		msg_ack[TAM_MSG_ACK + 1] = "NNNNNN$104";

HANDLE hMutexStatus;
HANDLE hMutexSetpoint;

HANDLE hEvent;

//////////////////////////////////////////////////////////////////////
// Read the value of an item on an OPC server. 

void main(void)
{	
	SetConsoleTitle("ACIARIA");

	DWORD ret;
	int tipoEvento;
	float SP_PRES_ARGONIO,
		  SP_TEMP_CAMARA;
	int   SP_PRES_CAMARA;

	hMutexStatus = CreateMutex(NULL, FALSE, "MutexStatus");
	GetLastError();

	hMutexSetpoint = CreateMutex(NULL, FALSE, "MutexSetpoint");
	GetLastError();

	hEvent = CreateEvent(NULL, FALSE, FALSE, "Evento_s");
	GetLastError();

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

		ret = WaitForSingleObject(hEvent, 1);
		GetLastError();

		tipoEvento = ret - WAIT_OBJECT_0;

		if (tipoEvento == 0) {
			SP_PRES_ARGONIO  =	(msg_setpoint[11] - '0') * pow(10, 3) +
								(msg_setpoint[12] - '0') * pow(10, 2) +
								(msg_setpoint[13] - '0') * pow(10, 1) +
								(msg_setpoint[14] - '0') * pow(10, 0) +
								(msg_setpoint[16] - '0') * pow(10, -1);
			SP_TEMP_CAMARA   =	(msg_setpoint[18] - '0') * pow(10, 3) +
								(msg_setpoint[19] - '0') * pow(10, 2) +
								(msg_setpoint[20] - '0') * pow(10, 1) +
								(msg_setpoint[21] - '0') * pow(10, 0) +
								(msg_setpoint[23] - '0') * pow(10, -1);
			SP_PRES_CAMARA   =	(msg_setpoint[25] - '0') * pow(10, 3) +
								(msg_setpoint[26] - '0') * pow(10, 2) +
								(msg_setpoint[27] - '0') * pow(10, 1) +
								(msg_setpoint[28] - '0') * pow(10, 0);

			aux.vt = VT_R8;
			aux.iVal = SP_PRES_ARGONIO;
			WriteItem(pIOPCItemMgt, hITEM_ID_SP_PRES_ARGONIO, aux);
			printf("Set-point de pressao de injecao de gas argonio: %06.1f\n", SP_PRES_ARGONIO);

			aux.vt = VT_R4;
			aux.iVal = SP_TEMP_CAMARA;
			WriteItem(pIOPCItemMgt, hITEM_ID_SP_TEMP_CAMARA, aux);
			printf("Set-point de temperatura na camara a vacuo: %06.1f\n", SP_TEMP_CAMARA);
			
			aux.vt = VT_I4;
			aux.iVal = SP_PRES_CAMARA;
			WriteItem(pIOPCItemMgt, hITEM_ID_SP_PRES_CAMARA, aux);
			printf("Set-point de pressao na camara a vacuo: %04d\n", SP_PRES_CAMARA);

			tipoEvento = -1;
			ret = 1;
		}
		
		ret = WaitForSingleObject(hMutexStatus, 1);
		GetLastError();

		tipoEvento = ret - WAIT_OBJECT_0;

		if (tipoEvento == 0) {
			for (int i = 11; i <= TAM_MSG_DADOS; i++) {
				msg_dados[i] = mensagem[i-11];
			}

			tipoEvento = -1;
			ret = 1;
		}

		ret = ReleaseMutex(hMutexStatus);
		GetLastError();
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

	CloseHandle(hMutexStatus);
	CloseHandle(hMutexSetpoint);
	CloseHandle(hEvent);
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
