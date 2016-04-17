#include "CvGameCoreDLLPCH.h"
#include "CvQuestInfo.h"

// must be included after all other headers
#include "LintFree.h"

#if defined(MOD_EVENTS_QUESTS)

CvQuestInfo::CvQuestInfo() :
	m_bInternal(false),
	m_bEnabled(true),
	m_bGlobal(false),
	m_bRevokeOnBully(true),
	m_bContest(false),
	m_iMinPlayers(1),
	m_iDuration(-1),
	m_iFriendship(0),
	m_iBiasFriendly(100),
	m_iBiasNeutral(100),
	m_iBiasHostile(100),
	m_iBiasIrrational(100),
	m_iBiasMaritime(100),
	m_iBiasMercantile(100),
	m_iBiasCultured(100),
	m_iBiasMilitaristic(100),
	m_iBiasReligious(100),
	m_strDisabledOnOption(NULL),
	m_strStartSummary(NULL),
	m_strStartMessage(NULL),
	m_strFinishSummary(NULL),
	m_strFinishMessage(NULL),
	m_strCancelSummary(NULL),
	m_strCancelMessage(NULL)
{
}

CvQuestInfo::~CvQuestInfo()
{
}

bool CvQuestInfo::isInternal() const
{
	return m_bInternal;
}

bool CvQuestInfo::isEnabled() const
{
	if (!m_strDisabledOnOption.IsEmpty())
	{
		int i = 0;
		CvPreGame::GetGameOption(m_strDisabledOnOption.c_str(), i);
		
		if (i == 1) return false;
	}

	return m_bEnabled;
}

bool CvQuestInfo::isGlobal() const
{
	return m_bGlobal;
}

bool CvQuestInfo::isRevokeOnBully() const
{
	return m_bRevokeOnBully;
}

bool CvQuestInfo::isContest() const
{
	return m_bContest;
}

int CvQuestInfo::getMinPlayers() const
{
	return m_iMinPlayers;
}

int CvQuestInfo::getDuration() const
{
	return m_iDuration;
}

int CvQuestInfo::getFriendship() const
{
	return m_iFriendship;
}

int CvQuestInfo::getBiasFriendly() const
{
	return (m_iBiasFriendly <= 0 ? 100 : m_iBiasFriendly);
}

int CvQuestInfo::getBiasNeutral() const
{
	return (m_iBiasNeutral <= 0 ? 100 : m_iBiasNeutral);
}

int CvQuestInfo::getBiasHostile() const
{
	return (m_iBiasHostile <= 0 ? 100 : m_iBiasHostile);
}

int CvQuestInfo::getBiasIrrational() const
{
	return (m_iBiasIrrational <= 0 ? 100 : m_iBiasIrrational);
}

int CvQuestInfo::getBiasMaritime() const
{
	return (m_iBiasMaritime <= 0 ? 100 : m_iBiasMaritime);
}

int CvQuestInfo::getBiasMercantile() const
{
	return (m_iBiasMercantile <= 0 ? 100 : m_iBiasMercantile);
}

int CvQuestInfo::getBiasCultured() const
{
	return (m_iBiasCultured <= 0 ? 100 : m_iBiasCultured);
}

int CvQuestInfo::getBiasMilitaristic() const
{
	return (m_iBiasMilitaristic <= 0 ? 100 : m_iBiasMilitaristic);
}

int CvQuestInfo::getBiasReligious() const
{
	return (m_iBiasReligious <= 0 ? 100 : m_iBiasReligious);
}

CvString CvQuestInfo::getStartSummary() const
{
	return m_strStartSummary;
}

CvString CvQuestInfo::getStartMessage() const
{
	return m_strStartMessage;
}

CvString CvQuestInfo::getFinishSummary() const
{
	return m_strFinishSummary;
}

CvString CvQuestInfo::getFinishMessage() const
{
	return m_strFinishMessage;
}

CvString CvQuestInfo::getCancelSummary() const
{
	return m_strCancelSummary;
}

CvString CvQuestInfo::getCancelMessage() const
{
	return m_strCancelMessage;
}

bool CvQuestInfo::CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility)
{
	if(!CvBaseInfo::CacheResults(kResults, kUtility))
		return false;

	m_bInternal = kResults.GetBool("Internal");
	m_bEnabled = kResults.GetBool("Enabled");
	m_bGlobal = kResults.GetBool("Global");
	m_bRevokeOnBully = kResults.GetBool("RevokeOnBully");
	m_bContest = kResults.GetBool("Contest");
	m_iMinPlayers = kResults.GetInt("MinPlayers");
	m_iDuration = kResults.GetInt("Duration");
	m_iFriendship = kResults.GetInt("Friendship");
	m_iBiasFriendly = kResults.GetInt("BiasFriendly");
	m_iBiasNeutral = kResults.GetInt("BiasNeutral");
	m_iBiasHostile = kResults.GetInt("BiasHostile");
	m_iBiasIrrational = kResults.GetInt("BiasIrrational");
	m_iBiasMaritime = kResults.GetInt("BiasMaritime");
	m_iBiasMercantile = kResults.GetInt("BiasMercantile");
	m_iBiasCultured = kResults.GetInt("BiasCultured");
	m_iBiasMilitaristic = kResults.GetInt("BiasMilitaristic");
	m_iBiasReligious = kResults.GetInt("BiasReligious");
	m_strDisabledOnOption = kResults.GetText("DisabledOnOption");
	m_strStartSummary = kResults.GetText("StartSummary");
	m_strStartMessage = kResults.GetText("StartMessage");
	m_strFinishSummary = kResults.GetText("FinishSummary");
	m_strFinishMessage = kResults.GetText("FinishMessage");
	m_strCancelSummary = kResults.GetText("CancelSummary");
	m_strCancelMessage = kResults.GetText("CancelMessage");

	return true;
}

#endif
