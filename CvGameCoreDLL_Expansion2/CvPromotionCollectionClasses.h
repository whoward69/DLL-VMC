#pragma once
#include "CvInfos.h"
#include "CvEnums.h"

#ifdef MOD_PROMOTION_COLLECTIONS

class CvPromotionCollectionEntry : public CvBaseInfo
{
public:
    struct TriggerInfo {
        bool m_bMeleeAttack = false;
        bool m_bRangedAttack = false;
        bool m_bMeleeDefense = false;
        bool m_bRangedDefense = false;

        int m_iHPFixed = 0;
        int m_iHPPercent = 0;
        bool m_bLuaCheck = false;
        
        bool m_bLuaHook = false;
    };

    struct PromotionEntry {
        PromotionTypes m_ePromotionType;

        int m_iIndex = 0;
        TriggerInfo m_kTriggerInfo;
    };

private:
    std::vector<PromotionEntry> m_vPromotions;
    std::vector<PromotionCollectionsTypes> m_vAddEnemyPromotionPools;
    bool m_bStackingFightBack = false;
    bool m_bStopAttacker = false;

public:
    CvPromotionCollectionEntry() = default;
    ~CvPromotionCollectionEntry() = default;

    bool CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility) override;

    bool CanAddEnemyPromotions() const { return !m_vAddEnemyPromotionPools.empty();}

    auto GetPromotions() -> decltype(m_vPromotions)& { return m_vPromotions; }
    auto GetAddEnemyPromotionPools() -> decltype(m_vAddEnemyPromotionPools)& { return m_vAddEnemyPromotionPools; }

    auto GetStackingFightBack() -> decltype(m_bStackingFightBack) { return m_bStackingFightBack; }
    auto GetStopAttacker() -> decltype(m_bStopAttacker) { return m_bStopAttacker; }
};

class CvPromotionCollectionEntries
{
public:
    CvPromotionCollectionEntries() = default;
    ~CvPromotionCollectionEntries() = default;

    std::vector<CvPromotionCollectionEntry*>& GetEntries() { return entries; }
    CvPromotionCollectionEntry* GetEntry(PromotionCollectionsTypes ePromotionCollection) { return entries[ePromotionCollection]; }
    int GetNumEntries() { return entries.size(); }

private:
    std::vector<CvPromotionCollectionEntry*> entries;
};

#endif