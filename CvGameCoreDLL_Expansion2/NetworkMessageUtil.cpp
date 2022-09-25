#include <CvGameCoreDLLPCH.h>
#include "NetworkMessageUtil.h"

void NetworkMessageUtil::IClear(int* buffer) {
	for (int i = 0; i < 16; i++) {
		buffer[i] = 0;
	}
}


LargeArgContainer NetworkMessageUtil::ReceiveLargeArgContainer;
int NetworkMessageUtil::ArgumentsToPass[MAX_INT32_ARGNUM];


std::list<int> InvokeRecorder::returnValueRecord;
std::map<int, list<int>::iterator> InvokeRecorder::valueMap;
FCriticalSection InvokeRecorder::m_Locker;
void InvokeRecorder::pushTimeValue(int time) {
	while (!m_Locker.Try()) {
		Sleep(1);
	}
	m_Locker.Enter();
	if (returnValueRecord.size() >= MaxSize) {
		valueMap.erase(returnValueRecord.front());
		returnValueRecord.pop_front();
	}
	returnValueRecord.push_back(time);
	valueMap.insert(std::pair<int, list<int>::iterator>(time, --returnValueRecord.end()));
	m_Locker.Leave();
}

bool InvokeRecorder::getTimeValueExist(int time) {
	while (!m_Locker.Try()) {
		Sleep(1);
	}
	m_Locker.Enter();
	std::map<int, list<int>::iterator>::iterator iter = valueMap.find(time);
	bool rtn = false;
	if (iter != valueMap.end()) {
		rtn = true;
		returnValueRecord.erase((*iter).second);
		valueMap.erase(iter);
	}
	m_Locker.Leave();
	return rtn;
}

namespace ReturnValueUtil {
	InvokeRecorder container;
}
