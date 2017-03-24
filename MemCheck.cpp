// MemCheck.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "ETWTraceSession.h"

int _tmain(int argc, _TCHAR* argv[])
{
	ITraceConsumer * pConsumer = new NodeTraceConsumer();

	LPTSTR szSessionName = KERNEL_LOGGER_NAME;


	TraceSession* pSession = new TraceSession(szSessionName);
	if (!pSession->StartKernel()) {
		if (pSession->Status() == ERROR_ALREADY_EXISTS) {
			if (!pSession->Stop() || !pSession->StartKernel()) {
				wprintf(L"Error in trace session %d", pSession->Status());
				goto cleanup;
			}
		}
	}

	/*if (!pSession->EnableProvider(SystemTraceControlGuid, TRACE_LEVEL_VERBOSE, 0x4000, 0))
	{
		wprintf(L"Error in enabling provider %d", pSession->Status());
		goto cleanup;
	}*/

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

