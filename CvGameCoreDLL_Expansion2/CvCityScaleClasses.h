#pragma once
#include "CvInfos.h"
#include "CvEnums.h"

#ifdef MOD_GLOBAL_CITY_SCALES

class CvCityScaleEntry :
    public CvBaseInfo
{
public:
    CvCityScaleEntry() = default;
    ~CvCityScaleEntry();

    bool CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility) override;

public:
    struct FreeBuildingClassInfo {
        BuildingClassTypes m_eBuildingClass = NO_BUILDINGCLASS;
        int m_iNum = 1;

        PolicyTypes m_eRequiredPolicy = NO_POLICY;
        TraitTypes m_eRequiredTrait = NO_TRAIT;
    };

protected:
    int m_iMinPopulation = -1;
    std::vector<FreeBuildingClassInfo> m_vFreeBuildingClassInfo;

    std::vector<FreeBuildingClassInfo> m_vFreeBuildingClassInfoFromPolicies;
    std::vector<FreeBuildingClassInfo> m_vFreeBuildingClassInfoFromTraits;

public:
    inline int GetMinPopulation() const { return m_iMinPopulation; }
    inline auto GetFreeBuildingClassInfo() -> decltype(m_vFreeBuildingClassInfo)& { return m_vFreeBuildingClassInfo; }
    inline auto GetFreeBuildingClassInfoFromPolicies() -> decltype(m_vFreeBuildingClassInfoFromPolicies)& { return m_vFreeBuildingClassInfoFromPolicies; }
    inline auto GetFreeBuildingClassInfoFromTraits() -> decltype(m_vFreeBuildingClassInfoFromTraits)& { return m_vFreeBuildingClassInfoFromTraits; }
};

class CvCityScaleXMLEntries
{
public:
    CvCityScaleXMLEntries() = default;
    ~CvCityScaleXMLEntries() = default;

    // Accessor functions
    inline std::vector<CvCityScaleEntry*>& GetEntries() { return m_paCvCityScaleEntries; }
    inline int GetNumCityScales() { return m_paCvCityScaleEntries.size(); }
    inline _Ret_maybenull_ CvCityScaleEntry* GetEntry(int index) { return m_paCvCityScaleEntries[index]; }
private:
    std::vector<CvCityScaleEntry*> m_paCvCityScaleEntries;
};

#endif