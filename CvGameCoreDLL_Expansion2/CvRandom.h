/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#pragma once

// random.h

#ifndef CIV5_RANDOM_H
#define CIV5_RANDOM_H

#ifdef _DEBUG
#include <vector>
#include <FCallStack.h>
#endif//_DEBUG

#include "StackWalker.h"
#include <string>


// Get a fake random number which depends only on game state
// This should be good enough for most purposes
// Very importantly, it should reduce desyncs in multiplayer
inline uint hash32(uint input)
{
	uint state = input * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

/// Helper type for producing seed values.
struct NODISCARD CvSeeder {
	uint value;

	inline CvSeeder() : value(0) {}
	template<typename T> explicit inline CvSeeder(T value) : value(hash32(static_cast<uint>(value))) {}
	inline CvSeeder(const CvSeeder& other) : value(other.value) {}

	inline static CvSeeder fromRaw(uint rawValue)
	{
		CvSeeder newSeed;
		newSeed.value = rawValue;
		return newSeed;
	}

	template<typename T> inline CvSeeder& mixRawAssign(T otherValue)
	{
		value ^= static_cast<uint>(otherValue) + 0x9e3779b9 + (value << 6) + (value >> 2);
		return *this;
	}

	template<typename T> inline CvSeeder& mixAssign(T otherValue)
	{
		return mixRawAssign(hash32(static_cast<uint>(otherValue)));
	}

	// `mixAssign` is specialized for mixing two seeders.
	// This is done to avoid rehashing the value within the seeder because it is presumably already hashed.
	template<> inline CvSeeder& mixAssign<CvSeeder>(CvSeeder otherSeed)
	{
		return mixRawAssign(otherSeed.value);
	}

	template<typename T> inline CvSeeder mixRaw(T otherValue) const
	{
		CvSeeder newSeed = *this;
		newSeed.mixRawAssign(otherValue);
		return newSeed;
	}

	template<typename T> inline CvSeeder mix(T otherValue) const
	{
		CvSeeder newSeed = *this;
		newSeed.mixAssign(otherValue);
		return newSeed;
	}

	inline operator uint() const
	{
		return value;
	}
};



class RNGStackWalker : public StackWalker {
public:
	RNGStackWalker() : m_pLog(nullptr), StackWalker() {}
	void SetLog(FILogFile* pLog) { m_pLog = pLog; }
protected:
	virtual void OnSymInit(LPCSTR, DWORD, LPCSTR) {  }
	virtual void OnLoadModule(LPCSTR, LPCSTR, DWORD64, DWORD, DWORD, LPCSTR, LPCSTR, ULONGLONG) {  }
	virtual void OnOutput(LPCSTR szText)
	{
		if (strstr(szText, "ERROR") == nullptr && strstr(szText, "not available") == nullptr)
		{
			if (m_pLog) m_pLog->Msg(szText);
		}
	}
	FILogFile* m_pLog;
};

extern RNGStackWalker dbgRNGStackWalker;

class CvRandom
{

public:
	explicit CvRandom(bool extendedCallStackDebugging);
	CvRandom();
	CvRandom(const CvRandom& source);
	virtual ~CvRandom();

	void init(unsigned long ulSeed);
	void uninit();
	void reset(unsigned long ulSeed = 0);

	unsigned short get(unsigned short usNum, const char* pszLog = NULL);  //  Returns value from 0 to num-1 inclusive.
	float getFloat();

	void reseed(unsigned long ulNewValue);
	unsigned long getSeed() const;
	unsigned long getCallCount() const;
	unsigned long getResetCount() const;

	// for serialization
	void read(FDataStream& Stream);
	void write(FDataStream& Stream) const;

	bool operator==(const CvRandom& rhs) const;
	bool operator!=(const CvRandom& rhs) const;

	// for OOS debugging
	const std::vector<std::string>& getResolvedCallStacks() const;
	const std::vector<unsigned long>& getSeedHistory() const;
	void resolveCallStacks() const;
	bool callStackDebuggingEnabled() const;
	void setCallStackDebuggingEnabled(bool enabled);
	void clearCallstacks();

protected:
	void recordCallStack();

protected:

	unsigned long m_ulRandomSeed;

	// for OOS checks/debugging
	
	std::string m_name;
	unsigned long m_ulCallCount;
	unsigned long m_ulResetCount;
	bool m_bSynchronous;		// If true, the instance is marked as being one that should be synchronous across multi-player games.

#ifdef _DEBUG
	bool m_bExtendedCallStackDebugging;
	// something awful is happening with synchronization, log call stacks
	mutable std::vector<FCallStack> m_kCallStacks;
	mutable std::vector<unsigned long> m_seedHistory;
	// just in case addresses don't match up, resolve symbols on send
	// and compare on the remote for somewhat meaningful output
	mutable std::vector<std::string>  m_resolvedCallStacks;
#endif//_DEBUG

};

FDataStream& operator<<(FDataStream& saveTo, const CvRandom& readFrom);
FDataStream& operator>>(FDataStream& loadFrom, CvRandom& writeTo);
#endif
