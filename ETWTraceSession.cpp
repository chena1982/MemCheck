#include "stdafx.h"

#include "ETWTraceSession.h"


//code from http://chabster.blogspot.com/2012/10/realtime-etw-consumer-howto.html

DEFINE_GUID( /* 222962AB-6180-4B88-A825-346B75F2A24A */
	HeapTraceGuid,
	0x222962AB,
	0x6180,
	0x4B88,
	0xa8, 0x25, 0x34, 0x6b, 0x75, 0xf2, 0xa2, 0x4a
	);


GUID guid[] = {
	SystemTraceControlGuid,
	HeapTraceGuid
};


TraceSession::TraceSession(LPCTSTR szSessionName) : _szSessionName(_tcsdup(szSessionName))
{
    hSession = NULL;
}

TraceSession::~TraceSession(void)
{
	delete[]_szSessionName;
	delete _pSessionProperties;
}

bool TraceSession::Start(bool isKernel)
{
	if (isKernel)
	{
		//https://msdn.microsoft.com/en-us/library/windows/desktop/aa363691(v=vs.85).aspx
		if (!_pSessionProperties)
		{
			const size_t buffSize = sizeof(EVENT_TRACE_PROPERTIES) + (_tcslen(_szSessionName) + 1) * sizeof(TCHAR);
			_pSessionProperties = reinterpret_cast<EVENT_TRACE_PROPERTIES *>(malloc(buffSize));
			ZeroMemory(_pSessionProperties, buffSize);

			_pSessionProperties->Wnode.BufferSize = buffSize;
			_pSessionProperties->Wnode.ClientContext = 1;
			_pSessionProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
			_pSessionProperties->Wnode.Guid = SystemTraceControlGuid;

			_pSessionProperties->EnableFlags = EVENT_TRACE_FLAG_VIRTUAL_ALLOC;

			_pSessionProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
			_pSessionProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
		}
	}
	else
	{
		if (!_pSessionProperties)
		{
			const size_t buffSize = sizeof(EVENT_TRACE_PROPERTIES) + (_tcslen(_szSessionName) + 1) * sizeof(TCHAR);
			_pSessionProperties = reinterpret_cast<EVENT_TRACE_PROPERTIES *>(malloc(buffSize));
			ZeroMemory(_pSessionProperties, buffSize);
			_pSessionProperties->Wnode.BufferSize = buffSize;
			_pSessionProperties->Wnode.ClientContext = 1;



            _pSessionProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE | PROCESS_TRACE_MODE_EVENT_RECORD;
			_pSessionProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
		}
	}


	// Create the trace session.
	_status = StartTrace(&hSession, _szSessionName, _pSessionProperties);

	return (_status == ERROR_SUCCESS);
}



bool TraceSession::EnableProvider(TraceType type, UCHAR level, ULONGLONG anyKeyword, ULONGLONG allKeyword)
{
	//enable stack
	//https://github.com/Microsoft/perfview/blob/master/src/TraceEvent/TraceEventSession.cs

	//event id
	//https://github.com/mic101/windows/blob/master/WRK-v1.2/public/internal/base/inc/ntwmi.h

	//https://blogs.msdn.microsoft.com/oldnewthing/20040826-00/?p=38043/
	PEVENT_FILTER_EVENT_ID filterId;
	DWORD size = FIELD_OFFSET(EVENT_FILTER_EVENT_ID, Events[1]);
	filterId = (PEVENT_FILTER_EVENT_ID)new BYTE[size];
	filterId->FilterIn = TRUE;
	filterId->Count = 1;
	filterId->Events[0] = 21;


	EVENT_FILTER_DESCRIPTOR filterDesc;
	ZeroMemory(&filterDesc, sizeof(filterDesc));
	filterDesc.Ptr = (ULONGLONG)filterId;
	filterDesc.Size = size;
	filterDesc.Type = EVENT_FILTER_TYPE_STACKWALK;

	ENABLE_TRACE_PARAMETERS traceParam;
	ZeroMemory(&traceParam, sizeof(traceParam));
	traceParam.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
	traceParam.EnableProperty = EVENT_ENABLE_PROPERTY_STACK_TRACE;
	traceParam.SourceId = guid[(int)type];
	//traceParam.EnableFilterDesc = &filterDesc;
	//traceParam.FilterDescCount = 1;
	_status = EnableTraceEx2(hSession, &guid[(int)type], EVENT_CONTROL_CODE_ENABLE_PROVIDER, level, anyKeyword, allKeyword, 0, &traceParam);

	delete [] filterId;

	return (_status == ERROR_SUCCESS);
}

bool TraceSession::OpenTrace(ITraceConsumer *pConsumer)
{
	if (!pConsumer)
		return false;

	ZeroMemory(&_logFile, sizeof(EVENT_TRACE_LOGFILE));
	_logFile.LoggerName = _szSessionName;
	_logFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
	_logFile.EventRecordCallback = &EventRecordCallback;
	_logFile.BufferCallback = &BufferRecordCallback;
	_logFile.Context = pConsumer;
	_logFile.IsKernelTrace = TRUE;

	_hTrace = ::OpenTrace(&_logFile);
	return (_hTrace != 0);
}

bool TraceSession::Process()
{
	_status = ProcessTrace(&_hTrace, 1, NULL, NULL);
	return (_status == ERROR_SUCCESS);
}

bool TraceSession::CloseTrace()
{
	_status = ::CloseTrace(_hTrace);
	return (_status == ERROR_SUCCESS);
}

bool TraceSession::DisableProvider(const GUID& providerId)
{
	_status = EnableTraceEx2(hSession, &providerId, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, NULL);
	return (_status == ERROR_SUCCESS);
}

bool TraceSession::Stop()
{
	_status = ControlTrace(hSession, _szSessionName, _pSessionProperties, EVENT_TRACE_CONTROL_STOP);
    hSession = NULL;

	delete _pSessionProperties;
	_pSessionProperties = NULL;

	return (_status == ERROR_SUCCESS);
}

ULONG TraceSession::Status() const
{
	return _status;
}

LONGLONG TraceSession::PerfFreq() const
{
	return _logFile.LogfileHeader.PerfFreq.QuadPart;
}


VOID WINAPI EventRecordCallback(_In_ PEVENT_RECORD pEventRecord)
{
	reinterpret_cast<ITraceConsumer *>(pEventRecord->UserContext)->OnEventRecord(pEventRecord);
}


ULONG WINAPI BufferRecordCallback(_In_ PEVENT_TRACE_LOGFILE Buffer)
{
	return TRUE;
}



void NodeTraceConsumer::OnEventRecord(PEVENT_RECORD eventPointer)
{

	auto & header = eventPointer->EventHeader;

	if (header.ProviderId == HeapTraceGuid)
	{

	}

	LPWSTR pszPropertyName = NULL;
	PEVENT_HEADER_EXTENDED_DATA_ITEM pExtendedData;

	for (DWORD index = 0; index < eventPointer->ExtendedDataCount; index++)
	{
		wprintf(L"\t--- EVENT_PROPERTY_INFO ---\n");
		pExtendedData = &eventPointer->ExtendedData[index];
		switch (pExtendedData->ExtType)
		{
		case EVENT_HEADER_EXT_TYPE_STACK_TRACE32:
			break;
			case
			EVENT_HEADER_EXT_TYPE_STACK_TRACE64:
				break;
		}
	}
};
