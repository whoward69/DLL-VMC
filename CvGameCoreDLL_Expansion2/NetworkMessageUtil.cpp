#include <CvGameCoreDLLPCH.h>
#include "NetworkMessageUtil.h"

void NetworkMessageUtil::StringShift(char* buffer, std::string& target) {
	int size = target.size();
	memcpy(buffer, target.c_str(), size);
	for (int i = 0; i < size; i++) {
		buffer[i] += SERIALIZED_SHIFT_NUM;
	}
}

void NetworkMessageUtil::CClear(char* buffer, int msgLength) {
	for (int i = 0; i < msgLength; i++) {
		buffer[i] = 0;
	}
}

void NetworkMessageUtil::IClear(int* buffer) {
	for (int i = 0; i < 16; i++) {
		buffer[i] = 0;
	}
}

void NetworkMessageUtil::StringShiftReverse(char* buffer, const char* target, int msgLength) {
	memcpy(buffer, target, msgLength);
	for (int i = 0; i < msgLength; i++) {
		buffer[i] -= SERIALIZED_SHIFT_NUM;
	}
}

char NetworkMessageUtil::ReceiveBuffer[1024];
ArgContainer NetworkMessageUtil::ReceiveArgContainer;
LargeArgContainer NetworkMessageUtil::ReceivrLargeArgContainer;
int NetworkMessageUtil::ArgumentsToPass[MAX_INT32_ARGNUM];


std::list<int> InvokeRecorder::returnValueRecord;
std::map<int, list<int>::iterator> InvokeRecorder::valueMap;
FCriticalSection InvokeRecorder::m_Locker;
void InvokeRecorder::pushReturnValue(int time) {
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

bool InvokeRecorder::getReturnValueExist(int time) {
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
