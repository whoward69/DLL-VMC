#include <CvGameCoreDLLPCH.h>
#include "NetworkMessageAdapter.h"

void NetworkMessageAdapter::StringShift(char* buffer, std::string& target) {
	int size = target.size();
	memcpy(buffer, target.c_str(), size);
	for (int i = 0; i < size; i++) {
		buffer[i] += SERIALIZED_SHIFT_NUM;
	}
}

void NetworkMessageAdapter::StringShiftReverse(char* buffer, const char* target, int msgLength) {
	memcpy(buffer, target, msgLength);
	for (int i = 0; i < msgLength; i++) {
		buffer[i] -= SERIALIZED_SHIFT_NUM;
	}
}
