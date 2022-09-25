#include "CvGameCoreDLLPCH.h"
#include "FunctionsRef.h"

#include "CvLuaGame.h"
#include "CvLuaMap.h"
#include "CvLuaCity.h"
#include "CvLuaUnit.h"
#include "CvLuaPlayer.h"
#include "CvLuaPlot.h"
#include "CvLuaTeamTech.h"

namespace FunctionPointers {
	InstanceFunctionReflector instanceFunctions;
	StaticFunctionReflector staticFunctions;
}

NoSuchMethodException::NoSuchMethodException(const std::string& method_name){
	this->message = "Method " + method_name + "does not exist in dictionry.";
}
NoSuchMethodException::~NoSuchMethodException(){}
const char* NoSuchMethodException::what()const throw(){
	return this->message.c_str();
}

StaticFunctionReflector::StaticFunctionReflector() {
	StaticFunctionReflector::methods = new std::tr1::unordered_map<std::string, std::pair<void(*)(), int>>(2048);
	CvUnit::RegistStaticFunctions();
	CvCity::RegistStaticFunctions();
	CvTeam::RegistStaticFunctions();
	CvPlot::RegistStaticFunctions();
	CvPlayer::RegistStaticFunctions();

	CvLuaUnit::RegistStaticFunctions();
	CvLuaPlayer::RegistStaticFunctions();
	CvLuaCity::RegistStaticFunctions();
	CvLuaMap::RegistStaticFunctions();
	CvLuaGame::RegistStaticFunctions();
	CvLuaPlot::RegistStaticFunctions();
	CvLuaTeamTech::RegistStaticFunctions();
}

InstanceFunctionReflector::InstanceFunctionReflector() {
	methods = new std::tr1::unordered_map<std::string, std::pair<void(None::*)(), int>>(2048);
	CvUnit::RegistInstanceFunctions();
	CvCity::RegistInstanceFunctions();
	CvPlot::RegistInstanceFunctions();
	CvTeam::RegistInstanceFunctions();
	CvPlayer::RegistInstanceFunctions();
}

std::tr1::unordered_map<std::string, std::pair<void(None::*)(), int>>* InstanceFunctionReflector::methods;
std::tr1::unordered_map<std::string, std::pair<void(*)(), int>>* StaticFunctionReflector::methods;
