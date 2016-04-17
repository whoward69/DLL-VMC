#pragma once

#ifndef CV_QUEST_INFO_H
#define CV_QUEST_INFO_H

#if defined(MOD_EVENTS_QUESTS)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvQuestInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvQuestInfo : public CvBaseInfo
{
public:
	CvQuestInfo();
	virtual ~CvQuestInfo();

	bool isInternal() const;
	bool isEnabled() const;
	bool isGlobal() const;
	bool isRevokeOnBully() const;
	bool isContest() const;
	int getMinPlayers() const;
	int getDuration() const;
	int getFriendship() const;
	int getBiasFriendly() const;
	int getBiasNeutral() const;
	int getBiasHostile() const;
	int getBiasIrrational() const;
	int getBiasMaritime() const;
	int getBiasMercantile() const;
	int getBiasCultured() const;
	int getBiasMilitaristic() const;
	int getBiasReligious() const;
	CvString getStartSummary() const;
	CvString getStartMessage() const;
	CvString getFinishSummary() const;
	CvString getFinishMessage() const;
	CvString getCancelSummary() const;
	CvString getCancelMessage() const;

	// Other
	virtual bool CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility);

protected:
	bool m_bInternal;
	bool m_bEnabled;
	bool m_bGlobal;
	bool m_bRevokeOnBully;
	bool m_bContest;
	int m_iMinPlayers;
	int m_iDuration;
	int m_iFriendship;
	int m_iBiasFriendly;
	int m_iBiasNeutral;
	int m_iBiasHostile;
	int m_iBiasIrrational;
	int m_iBiasMaritime;
	int m_iBiasMercantile;
	int m_iBiasCultured;
	int m_iBiasMilitaristic;
	int m_iBiasReligious;
	CvString m_strDisabledOnOption;
	CvString m_strStartSummary;
	CvString m_strStartMessage;
	CvString m_strFinishSummary;
	CvString m_strFinishMessage;
	CvString m_strCancelSummary;
	CvString m_strCancelMessage;

private:
	CvQuestInfo(const CvQuestInfo&);
	CvQuestInfo& operator=(const CvQuestInfo&);
};

#endif

#endif
