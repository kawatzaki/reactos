#ifndef __INCLUDE_INTERNAL_PORT_H
#define __INCLUDE_INTERNAL_PORT_H

#include <napi/lpc.h>

typedef struct _EPORT_LISTENER
{
  HANDLE ListenerPid;
  LIST_ENTRY ListenerListEntry;
} EPORT_LISTENER, *PEPORT_LISTENER;

typedef struct _EPORT
{
  KSPIN_LOCK	Lock;
  KSEMAPHORE    Semaphore;
  
  ULONG		State;
  
  struct _EPORT	* OtherPort;
  
  ULONG		QueueLength;
  LIST_ENTRY	QueueListHead;
  
  ULONG		ConnectQueueLength;
  LIST_ENTRY	ConnectQueueListHead;
  
  ULONG		MaxDataLength;
  ULONG		MaxConnectInfoLength;

  /*
   * List of processes that can receive connection requests on this port.
   */
  LIST_ENTRY ListenerListHead;
} EPORT, * PEPORT;


typedef struct _EPORT_TERMINATION_REQUEST
{
	LIST_ENTRY	ThreadListEntry;
	PEPORT		Port;	
} EPORT_TERMINATION_REQUEST, *PEPORT_TERMINATION_REQUEST;


typedef struct _EPORT_CONNECT_REQUEST_MESSAGE
{
  LPC_MESSAGE_HEADER MessageHeader;
  PEPROCESS ConnectingProcess;
  struct _SECTION_OBJECT* SendSectionObject;
  LARGE_INTEGER SendSectionOffset;
  ULONG SendViewSize;
  ULONG ConnectDataLength;
  UCHAR ConnectData[0];
} EPORT_CONNECT_REQUEST_MESSAGE, *PEPORT_CONNECT_REQUEST_MESSAGE;

typedef struct _EPORT_CONNECT_REPLY_MESSAGE
{
  LPC_MESSAGE_HEADER MessageHeader;
  PVOID SendServerViewBase;
  ULONG ReceiveClientViewSize;
  PVOID ReceiveClientViewBase;
  ULONG MaximumMessageSize;
  ULONG ConnectDataLength;
  UCHAR ConnectData[0];
} EPORT_CONNECT_REPLY_MESSAGE, *PEPORT_CONNECT_REPLY_MESSAGE;

NTSTATUS STDCALL
LpcRequestPort (PEPORT		Port,
		PLPC_MESSAGE	LpcMessage);
NTSTATUS
STDCALL
LpcSendTerminationPort (PEPORT	Port,
			TIME	CreationTime);


/* EPORT.State */

#define EPORT_INACTIVE                (0)
#define EPORT_WAIT_FOR_CONNECT        (1)
#define EPORT_WAIT_FOR_ACCEPT         (2)
#define EPORT_WAIT_FOR_COMPLETE_SRV   (3)
#define EPORT_WAIT_FOR_COMPLETE_CLT   (4)
#define EPORT_CONNECTED_CLIENT        (5)
#define EPORT_CONNECTED_SERVER        (6)
#define EPORT_DISCONNECTED            (7)

typedef struct _QUEUEDMESSAGE
{
  PEPORT		Sender;
  LIST_ENTRY	QueueListEntry;
  LPC_MESSAGE	Message;
  UCHAR		MessageData [MAX_MESSAGE_DATA];
} QUEUEDMESSAGE,  *PQUEUEDMESSAGE;

/* Code in ntoskrnl/lpc/close.h */

VOID STDCALL
NiClosePort (PVOID	ObjectBody,
	     ULONG	HandleCount);
VOID STDCALL
NiDeletePort (IN	PVOID	ObjectBody);

/* Code in ntoskrnl/lpc/queue.c */

VOID STDCALL
EiEnqueueConnectMessagePort (IN OUT	PEPORT		Port,
			     IN	PQUEUEDMESSAGE	Message);
VOID STDCALL
EiEnqueueMessagePort (IN OUT	PEPORT		Port,
		      IN	PQUEUEDMESSAGE	Message);
VOID STDCALL
EiEnqueueMessageAtHeadPort (IN OUT	PEPORT		Port,
			    IN	PQUEUEDMESSAGE	Message);
PQUEUEDMESSAGE
STDCALL
EiDequeueConnectMessagePort (IN OUT	PEPORT	Port);
PQUEUEDMESSAGE STDCALL
EiDequeueMessagePort (IN OUT	PEPORT	Port);

/* Code in ntoskrnl/lpc/create.c */

NTSTATUS STDCALL
NiCreatePort (PVOID			ObjectBody,
	      PVOID			Parent,
	      PWSTR			RemainingPath,
	      POBJECT_ATTRIBUTES	ObjectAttributes);

/* Code in ntoskrnl/lpc/port.c */

NTSTATUS STDCALL
NiInitializePort (IN OUT	PEPORT	Port);
NTSTATUS
NiInitPort (VOID);

extern POBJECT_TYPE	ExPortType;
extern ULONG		EiNextLpcMessageId;

/* Code in ntoskrnl/lpc/reply.c */

NTSTATUS STDCALL
EiReplyOrRequestPort (IN	PEPORT		Port, 
		      IN	PLPC_MESSAGE	LpcReply, 
		      IN	ULONG		MessageType,
		      IN	PEPORT		Sender);


#endif /* __INCLUDE_INTERNAL_PORT_H */
