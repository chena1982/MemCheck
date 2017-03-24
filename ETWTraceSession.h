#pragma once

class ITraceConsumer
{
public:
	ITraceConsumer() {};
	virtual ~ITraceConsumer() {};
public:
	virtual void OnEventRecord(PEVENT_RECORD eventPointer) = 0;
};


class NodeTraceConsumer : public ITraceConsumer
{
public:
	NodeTraceConsumer() {};
	virtual ~NodeTraceConsumer() {};
public:
	virtual void OnEventRecord(PEVENT_RECORD eventPointer);
};

class TraceSession
{

public:
	TraceSession(LPCTSTR szSessionName);
	~TraceSession();

public:
	bool Start();
	bool StartKernel();

	bool EnableProvider(const GUID& providerId, UCHAR level, ULONGLONG anyKeyword = 0, ULONGLONG allKeyword = 0);
	bool OpenTrace(ITraceConsumer *pConsumer);
	bool Process();
	bool CloseTrace();
	bool DisableProvider(const GUID& providerId);
	bool Stop();

	ULONG Status() const;
	LONGLONG PerfFreq() const;

private:
	LPTSTR _szSessionName;
	ULONG _status;
	EVENT_TRACE_PROPERTIES* _pSessionProperties;
	TRACEHANDLE hSession;
	EVENT_TRACE_LOGFILE _logFile;
	TRACEHANDLE _hTrace;

};

VOID WINAPI EventRecordCallback(_In_ PEVENT_RECORD pEventRecord);
ULONG WINAPI BufferRecordCallback(_In_ PEVENT_TRACE_LOGFILE Buffer);