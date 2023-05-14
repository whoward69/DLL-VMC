#include "CvBuildingClassCollectionsClasses.h"

#include "CvGameCoreDLLPCH.h"

#ifdef MOD_BUILDINGCLASS_COLLECTIONS
bool CvBuildingClassCollectionsEntry::CacheResults(Database::Results &kResults, CvDatabaseUtility &kUtility)
{
    if (!CvBaseInfo::CacheResults(kResults, kUtility))
        return false;

    const char *szThisType = CvBaseInfo::GetType();
    const size_t lenThisType = strlen(szThisType);

    {
        std::string strKey = "BuildingClassCollections - Entries";
        Database::Results *pResults = kUtility.GetResults(strKey);
        if (pResults == NULL)
        {
            pResults = kUtility.PrepareResults(strKey, "select *, t2.ID as BuildingClassID from BuildingClassCollections_Entries t1 "
                                                       "left join BuildingClasses t2 on t1.BuildingClassType = t2.Type where t1.CollectionType = ? order by t1.BuildingClassIndex;");
        }

        pResults->Bind(1, szThisType, lenThisType, false);

        while (pResults->Step())
        {
            BuildingClassEntry entry;
            entry.eBuildingClass = (BuildingClassTypes)pResults->GetInt("BuildingClassID");
            entry.iIndex = pResults->GetInt("BuildingClassIndex");
            m_vBuildingClasses.push_back(entry);
        }
        pResults->Reset();
    }
}

#endif