#pragma once


class TraceSession;
class ITraceConsumer;

class SessionInfo
{
	TraceSession * session;
	ITraceConsumer * consumer;
};

class TraceSessionManager
{
public:
	TraceSessionManager();
	~TraceSessionManager();

public:
private:

};