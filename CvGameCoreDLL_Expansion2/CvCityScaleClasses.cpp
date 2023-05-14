#include "CvGameCoreDLLPCH.h"
#include "CvCityScaleClasses.h"

#ifdef MOD_GLOBAL_CITY_SCALES

bool CvCityScaleEntry::CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility)
{
	if (!CvBaseInfo::CacheResults(kResults, kUtility))
	{
#ifdef DEBUG_CvCityScaleEntry
		CUSTOMLOG("CvCityScaleEntry::CacheResults CvBaseInfo::CacheResults failed!");
#endif
		return false;
	}
#ifdef DEBUG_CvCityScaleEntry
	CUSTOMLOG("CvCityScaleEntry::CacheResults start!");
#endif
	const char* szThisType = CvBaseInfo::GetType();
	const size_t lenThisType = strlen(szThisType);

	m_iMinPopulation = kResults.GetInt("MinPopulation");

	// m_vFreeBuildingClassInfo
	{
		std::string strKey = "CityScales - FreeBuildingClass";
		Database::Results* pResults = kUtility.GetResults(strKey);
		if (pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select t2.ID, t1.NumBuildings from CityScale_FreeBuildingClass t1 "
													"left join BuildingClasses t2 on t1.BuildingClassType = t2.Type "
													"where t1.CityScaleType = ? and t1.RequiredTraitType is null and t1.RequiredPolicyType is null;");
		}

		pResults->Bind(1, szThisType, lenThisType, false);

		while (pResults->Step())
		{
			FreeBuildingClassInfo info;
			info.m_eBuildingClass = static_cast<BuildingClassTypes>(pResults->GetInt(0));
			info.m_iNum = pResults->GetInt(1);
			CvAssert(info.m_eBuildingClass != NO_BUILDINGCLASS);

			m_vFreeBuildingClassInfo.push_back(info);
		}
		pResults->Reset();
	}

	// m_vFreeBuildingClassInfoFromTraits
	{
		std::string strKey = "CityScales - FreeBuildingClassFromTraits";
		Database::Results* pResults = kUtility.GetResults(strKey);
		if (pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select t2.ID, t1.NumBuildings, t3.ID from CityScale_FreeBuildingClass t1 "
													"left join BuildingClasses t2 on t1.BuildingClassType = t2.Type "
													"left join Traits t3 on t1.RequiredTraitType = t3.Type "
													"where t1.CityScaleType = ? and t1.RequiredTraitType is not null;");
		}

		pResults->Bind(1, szThisType, lenThisType, false);

		while (pResults->Step())
		{
			FreeBuildingClassInfo info;
			info.m_eBuildingClass = static_cast<BuildingClassTypes>(pResults->GetInt(0));
			info.m_iNum = pResults->GetInt(1);
			info.m_eRequiredTrait = (TraitTypes) pResults->GetInt(2);
			CvAssert(info.m_eBuildingClass != NO_BUILDINGCLASS);
			CvAssert(info.m_eRequiredTrait != NO_TRAIT);

			m_vFreeBuildingClassInfoFromTraits.push_back(info);
		}
		pResults->Reset();
	}

	// m_vFreeBuildingClassInfoFromPolicies
	{
		std::string strKey = "CityScales - FreeBuildingClassFromPolicies";
		Database::Results* pResults = kUtility.GetResults(strKey);
		if (pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select t2.ID, t1.NumBuildings, t3.ID from CityScale_FreeBuildingClass t1 "
													"left join BuildingClasses t2 on t1.BuildingClassType = t2.Type "
													"left join Policies t3 on t1.RequiredPolicyType = t3.Type "
													"where t1.CityScaleType = ? and t1.RequiredPolicyType is not null;");
		}

		pResults->Bind(1, szThisType, lenThisType, false);

		while (pResults->Step())
		{
			FreeBuildingClassInfo info;
			info.m_eBuildingClass = static_cast<BuildingClassTypes>(pResults->GetInt(0));
			info.m_iNum = pResults->GetInt(1);
			info.m_eRequiredPolicy = (PolicyTypes) pResults->GetInt(2);
			CvAssert(info.m_eBuildingClass != NO_BUILDINGCLASS);
			CvAssert(info.m_eRequiredTrait != NO_POLICY);

			m_vFreeBuildingClassInfoFromPolicies.push_back(info);
		}
		pResults->Reset();
	}

#ifdef DEBUG_CvCityScaleEntry
	CUSTOMLOG("CvCityScaleEntry::CacheResults end!");
#endif

	return true;
}

CvCityScaleEntry::~CvCityScaleEntry() {}

#endif