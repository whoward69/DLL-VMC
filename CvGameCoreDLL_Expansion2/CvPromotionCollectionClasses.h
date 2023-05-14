#pragma once
#include "CvInfos.h"
#include "CvEnums.h"

#ifdef MOD_PROMOTION_COLLECTIONS

class CvPromotionCollectionEntry : public CvBaseInfo
{
private:
    struct TriggerAddPromotionInfo {
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
        TriggerAddPromotionInfo m_kTriggerAddPromotionInfo;
    };

    std::vector<PromotionEntry> m_vPromotions;
    std::vector<PromotionCollectionsTypes> m_vAddEnermyPromotionPools;

public:
    CvPromotionCollectionEntry() = default;
    ~CvPromotionCollectionEntry() = default;

    bool CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility) override;

    bool CanAddEnermyPromotions() const { return !m_vAddEnermyPromotionPools.empty();}

    auto GetPromotions() -> decltype(m_vPromotions)& { return m_vPromotions; }
    auto GetAddEnermyPromotionPools() -> decltype(m_vAddEnermyPromotionPools)& { return m_vAddEnermyPromotionPools; }
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