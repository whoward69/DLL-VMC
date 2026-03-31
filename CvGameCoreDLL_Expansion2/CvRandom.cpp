/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */

#include "CvGameCoreDLLPCH.h"
#include "CvRandom.h"
#include "CvGlobals.h"
#include "FCallStack.h"
#include "FStlContainerSerialization.h"


#ifdef WIN32
#	include "Win32/FDebugHelper.h"
#endif//_WINPC



RNGStackWalker dbgRNGStackWalker;
// include this after all other headers!
#include "LintFree.h"

#define RANDOM_A      (1103515245)
#define RANDOM_C      (12345)
#define RANDOM_SHIFT  (16)


CvRandom::CvRandom() :
	m_ulRandomSeed(0)
	, m_ulCallCount(0)
	, m_ulResetCount(0)
	, m_bSynchronous(false)
#ifdef _DEBUG
	, m_bExtendedCallStackDebugging(false)
	, m_kCallStacks()
	, m_seedHistory()
	, m_resolvedCallStacks()
#endif//_debug
{
	reset();
}

CvRandom::CvRandom(bool extendedCallStackDebugging) :
	m_ulRandomSeed(0)
	, m_ulCallCount(0)
	, m_ulResetCount(0)
	, m_bSynchronous(true)
#ifdef _DEBUG
	, m_bExtendedCallStackDebugging(extendedCallStackDebugging || GC.getOutOfSyncDebuggingEnabled())
	, m_kCallStacks()
	, m_seedHistory()
	, m_resolvedCallStacks()
#endif//_debug
{
	extendedCallStackDebugging;
}

CvRandom::CvRandom(const CvRandom& source) :
	m_ulRandomSeed(source.m_ulRandomSeed)
	, m_ulCallCount(source.m_ulCallCount)
	, m_ulResetCount(source.m_ulResetCount)
	, m_bSynchronous(source.m_bSynchronous)
#ifdef _DEBUG
	, m_bExtendedCallStackDebugging(source.m_bExtendedCallStackDebugging)
	, m_kCallStacks(source.m_kCallStacks)
	, m_seedHistory(source.m_seedHistory)
	, m_resolvedCallStacks(source.m_resolvedCallStacks)
#endif//_debug
{
}

bool CvRandom::operator==(const CvRandom& source) const
{
#if defined(MOD_BUGFIX_RANDOM)
	return(m_ulRandomSeed == source.m_ulRandomSeed && m_ulCallCount == source.m_ulCallCount);
#else
	return(m_ulRandomSeed == source.m_ulRandomSeed);
#endif
}

bool CvRandom::operator!=(const CvRandom& source) const
{
	return !(*this == source);
}

CvRandom::~CvRandom()
{
	uninit();
}


void CvRandom::init(unsigned long ulSeed)
{
	//--------------------------------
	// Init saved data
	reset(ulSeed);

	//--------------------------------
	// Init non-saved data
}


void CvRandom::uninit()
{
}


// FUNCTION: reset()
// Initializes data members that are serialized.
void CvRandom::reset(unsigned long ulSeed)
{
	//--------------------------------
	// Uninit class
	uninit();

#if defined(MOD_BUGFIX_RANDOM)
	reseed(ulSeed);
#else
	recordCallStack();
	m_ulRandomSeed = ulSeed;
	m_ulResetCount++;
#endif
}

unsigned short CvRandom::get(unsigned short usNum, const char* pszLog)
{
	recordCallStack();
	m_ulCallCount++;

	unsigned long ulNewSeed = ((RANDOM_A * m_ulRandomSeed) + RANDOM_C);
	unsigned short us = ((unsigned short)((((ulNewSeed >> RANDOM_SHIFT) & MAX_UNSIGNED_SHORT) * ((unsigned long)usNum)) / (MAX_UNSIGNED_SHORT + 1)));

	if(true)
	{
		int iRandLogging = GC.getRandLogging();
		if(iRandLogging > 0 && m_bSynchronous)
		{
#if !defined(FINAL_RELEASE)
			if(!gDLL->IsGameCoreThread() && gDLL->IsGameCoreExecuting() && m_bSynchronous)
			{
				CvAssertMsg(0, "App side is accessing the synchronous random number generator while the game core is running.");
			}
#endif
			
			//if(kGame.getTurnSlice() > 0 || ((iRandLogging & RAND_LOGGING_PREGAME_FLAG) != 0))
			{
				FILogFile* pLog = LOGFILEMGR.GetLog("RandCalls.log", FILogFile::kDontTimeStamp);
				if(pLog)
				{
					char szOut[2048] = { 0 };
					char buf[1024] = {0};
					if(pszLog) strcpy(buf, pszLog);
					auto turn = GC.getGame().getGameTurn();
					string outStr = "turn: ";
					_itoa_s(turn, buf, 10);
					outStr += buf;
					
					outStr += ", max: ";
					_itoa_s(usNum, buf, 10);
					outStr += buf;

					outStr += ", result: ";
					_itoa_s(us, buf, 10);
					outStr += buf;

					outStr += ", seed: ";
					_ui64toa_s(ulNewSeed, buf, 1024, 10);
					outStr += buf;

					outStr += ", call count: ";
					_ui64toa_s(m_ulCallCount, buf, 1024, 10);
					outStr += buf;

					outStr += ", reset count: ";
					_ui64toa_s(m_ulResetCount, buf, 1024, 10);
					outStr += buf;

					/*sprintf(szOut, "turn: %d, max: %u, result: %u, seed: %I64u, call count: %I64u, reset count: %I64u, desc:  ", turn,
						usNum, us, ulNewSeed, m_ulCallCount, m_ulResetCount);*/

					outStr += ", desc: ";
					//string outStr = string(szOut);
					if (pszLog) outStr += pszLog;
					outStr += "\n";
					pLog->Msg(outStr.c_str());
					dbgRNGStackWalker.SetLog(pLog);
					dbgRNGStackWalker.ShowCallstack();
					pLog->Msg("\n");
				}
			}
		}
	}

	m_ulRandomSeed = ulNewSeed;
	return us;
}


float CvRandom::getFloat()
{
	return (((float)(get(MAX_UNSIGNED_SHORT))) / ((float)MAX_UNSIGNED_SHORT));
}


void CvRandom::reseed(unsigned long ulNewValue)
{
	recordCallStack();
	m_ulResetCount++;
	m_ulRandomSeed = ulNewValue;
#if defined(MOD_BUGFIX_RANDOM)
	m_ulCallCount = 0;
#endif
}


unsigned long CvRandom::getSeed() const
{
	return m_ulRandomSeed;
}

unsigned long CvRandom::getCallCount() const
{
	return m_ulCallCount;
}

unsigned long CvRandom::getResetCount() const
{
	return m_ulResetCount;
}

void CvRandom::read(FDataStream& kStream)
{
	reset();

	// Version number to maintain backwards compatibility
	uint uiVersion;
	kStream >> uiVersion;
	MOD_SERIALIZE_INIT_READ(kStream);

	kStream >> m_ulRandomSeed;
	kStream >> m_ulCallCount;
	kStream >> m_ulResetCount;
#ifdef _DEBUG
	kStream >> m_bExtendedCallStackDebugging;
	if(m_bExtendedCallStackDebugging)
	{
		kStream >> m_seedHistory;
		kStream >> m_resolvedCallStacks;
	}
#else
	bool b;
	kStream >> b;
#endif//_DEBUG
}


void CvRandom::write(FDataStream& kStream) const
{
	// Current version number
	uint uiVersion = 1;
	kStream << uiVersion;
	MOD_SERIALIZE_INIT_WRITE(kStream);

	kStream << m_ulRandomSeed;
	kStream << m_ulCallCount;
	kStream << m_ulResetCount;
#ifdef _DEBUG
	kStream << m_bExtendedCallStackDebugging;
	if(m_bExtendedCallStackDebugging)
	{
		resolveCallStacks();
		kStream << m_seedHistory;
		kStream << m_resolvedCallStacks;
	}
#else
	kStream << false;
#endif
}

void CvRandom::recordCallStack()
{
#ifdef _DEBUG
	if(m_bExtendedCallStackDebugging)
	{
		FDebugHelper& debugHelper = FDebugHelper::GetInstance();
		FCallStack callStack;
		debugHelper.GetCallStack(&callStack, 1, 8);
		m_kCallStacks.push_back(callStack);
		m_seedHistory.push_back(m_ulRandomSeed);
	}
#endif//_DEBUG
}

void CvRandom::resolveCallStacks() const
{
#ifdef _DEBUG
	std::vector<FCallStack>::const_iterator i;
	for(i = m_kCallStacks.begin() + m_resolvedCallStacks.size(); i != m_kCallStacks.end(); ++i)
	{
		const FCallStack callStack = *i;
		std::string stackTrace = callStack.toString(true);
		m_resolvedCallStacks.push_back(stackTrace);
	}
#endif//_DEBUG
}

const std::vector<std::string>& CvRandom::getResolvedCallStacks() const
{
#ifdef _DEBUG
	return m_resolvedCallStacks;
#else
	static std::vector<std::string> empty;
	return empty;
#endif//_debug
}

const std::vector<unsigned long>& CvRandom::getSeedHistory() const
{
#ifdef _DEBUG
	return m_seedHistory;
#else
	static std::vector<unsigned long> empty;
	return empty;
#endif//_DEBUG
}

bool CvRandom::callStackDebuggingEnabled() const
{
#ifdef _DEBUG
	return m_bExtendedCallStackDebugging;
#else
	return false;
#endif//_DEBUG
}

void CvRandom::setCallStackDebuggingEnabled(bool enabled)
{
#ifdef _DEBUG
	m_bExtendedCallStackDebugging = enabled;
#endif//_DEBUG
	enabled;
}

void CvRandom::clearCallstacks()
{
#ifdef _DEBUG
	m_kCallStacks.clear();
	m_seedHistory.clear();
	m_resolvedCallStacks.clear();
#endif//_DEBUG
}
FDataStream& operator<<(FDataStream& saveTo, const CvRandom& readFrom)
{
	readFrom.write(saveTo);
	return saveTo;
}

FDataStream& operator>>(FDataStream& loadFrom, CvRandom& writeTo)
{
	writeTo.read(loadFrom);
	return loadFrom;
}
