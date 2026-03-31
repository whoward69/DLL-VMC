#pragma once
#include "CustomMods.h"
#ifdef MOD_BUILDINGCLASS_COLLECTIONS

#include "CvInfos.h"

class CvBuildingClassCollectionsEntry : public CvBaseInfo
{
public:
    struct BuildingClassEntry
    {
        BuildingClassTypes eBuildingClass;
        int iIndex;
    };

private:
    std::vector<BuildingClassEntry> m_vBuildingClasses;

public:
    bool CacheResults(Database::Results &kResults, CvDatabaseUtility &kUtility) override;

    auto GetBuildingClasses() -> decltype(m_vBuildingClasses) & { return m_vBuildingClasses; }
};

class CvBuildingClassCollectionsXMLEntries
{
private:
    std::vector<CvBuildingClassCollectionsEntry*> m_vEntries;

public:
    int GetNumEntries() const { return m_vEntries.size(); }
    std::vector<CvBuildingClassCollectionsEntry*>& GetEntries() { return m_vEntries; }
    CvBuildingClassCollectionsEntry* GetEntry(int index) { return m_vEntries[index]; }
};

#endif