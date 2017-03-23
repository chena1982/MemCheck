// MemCheck.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "ETWTraceSession.h"

int _tmain(int argc, _TCHAR* argv[])
{
	ITraceConsumer * pConsumer = new NodeTraceConsumer();

	GUID nodeGuid;
	LPTSTR szSessionName = L"SampleTracer";

	ZeroMemory(&nodeGuid, sizeof(GUID));
	if (IIDFromString(L"{77754E9B-264B-4D8D-B981-E4135C1ECB0C}", &nodeGuid) != S_OK)
	{
		wprintf(L"Failed to get GUID from string");
		return 1;
	}

	TraceSession* pSession = new TraceSession(szSessionName);
	if (!pSession->Start()) {
		if (pSession->Status() == ERROR_ALREADY_EXISTS) {
			if (!pSession->Stop() || !pSession->Start()) {
				wprintf(L"Error in trace session %d", pSession->Status());
				goto cleanup;
			}
		}
	}

	if (!pSession->EnableProvider(nodeGuid, TRACE_LEVEL_VERBOSE))
	{
		wprintf(L"Error in enabling provider %d", pSession->Status());
		goto cleanup;
	}

	if (!pSession->OpenTrace(pConsumer))
	{
		wprintf(L"Error in opening trace %d", pSession->Status());
		goto cleanup;
	}

	if (!pSession->Process())
	{
		wprintf(L"Error in processing %d", pSession->Status());
		goto cleanup;
	}

cleanup:
	delete pSession;
	delete pConsumer;

	return 0;
}

