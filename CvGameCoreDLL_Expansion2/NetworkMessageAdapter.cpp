#include <CvGameCoreDLLPCH.h>
#include "NetworkMessageAdapter.h"

void NetworkMessageAdapter::StringShift(char* buffer, std::string& target) {
	int size = target.size();
	memcpy(buffer, target.c_str(), size);
	for (int i = 0; i < size; i++) {
		buffer[i] += SERIALIZED_SHIFT_NUM;
	}
}

void NetworkMessageAdapter::Clear(char* buffer, int msgLength) {
	for (int i = 0; i < msgLength; i++) {
		buffer[i] = 0;
	}
}

void NetworkMessageAdapter::StringShiftReverse(char* buffer, const char* target, int msgLength) {
	memcpy(buffer, target, msgLength);
	for (int i = 0; i < msgLength; i++) {
		buffer[i] -= SERIALIZED_SHIFT_NUM;
	}
}

void NetworkMessageAdapter::SetArguments(ArgContainer& args, const std::string& name,
	int arg1,
	int arg2,
	int arg3,
	int arg4,
	int arg5,
	int arg6,
	int arg7,
	int arg8,
	int arg9
) {
	args.set_functiontocall(name);
	args.set_args1(arg1);
	args.set_args2(arg2);
	args.set_args3(arg3);
	args.set_args4(arg4);
	args.set_args5(arg5);
	args.set_args6(arg6);
	args.set_args7(arg7);
	args.set_args8(arg8);
	args.set_args9(arg9);
}

char NetworkMessageAdapter::ReceiveBuffer[1024];
ArgContainer NetworkMessageAdapter::ReceiveArgContainer;
LargeArgContainer NetworkMessageAdapter::ReceivrLargeArgContainer;
