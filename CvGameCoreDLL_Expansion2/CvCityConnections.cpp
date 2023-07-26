/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#include "CvGameCoreDLLPCH.h"
#include "CvCityConnections.h"
#include "CvPlayer.h"
#include "CvAStar.h"
#include "CvMinorCivAI.h"
#include "CvDiplomacyAI.h"
#include "FStlContainerSerialization.h"

// must be included after all other headers
#include "LintFree.h"


FDataStream& operator<<(FDataStream& saveTo, const CvCityConnections::RouteInfo& readFrom)
{
	saveTo << readFrom.m_cRouteState;
	return saveTo;
}

FDataStream& operator>>(FDataStream& loadFrom, CvCityConnections::RouteInfo& writeTo)
{
	loadFrom >> writeTo.m_cRouteState;
	return loadFrom;
}

/// Constructor
CvCityConnections::CvCityConnections(void)
{
	m_aRouteInfos = NULL;
	Uninit();
}

/// Destructor
CvCityConnections::~CvCityConnections(void)
{
	Uninit();
}

/// Init
void CvCityConnections::Init(CvPlayer* pPlayer)
{
	CvBuildingXMLEntries* pkBuildingEntries = GC.GetGameBuildings();
	m_pPlayer = pPlayer;

	m_uiRouteInfosDimension = 64;
	m_aRouteInfos = NULL;
	ResizeRouteInfo(m_uiRouteInfosDimension);

	m_aBuildingsAllowWaterRoutes.clear();
	for(int i = 0; i < GC.GetGameBuildings()->GetNumBuildings(); i++)
	{
		const BuildingTypes eBuilding = static_cast<BuildingTypes>(i);
		CvBuildingEntry* pkBuildingInfo = pkBuildingEntries->GetEntry(eBuilding);
		if(pkBuildingInfo)
		{
			if(pkBuildingInfo->AllowsWaterRoutes())
			{
				m_aBuildingsAllowWaterRoutes.push_back(eBuilding);
			}
		}
	}

	m_aPlotRouteInfos.clear();
}

/// Uninit
void CvCityConnections::Uninit(void)
{
	m_pPlayer = NULL;
	if(m_aRouteInfos)
	{
		delete [] m_aRouteInfos;
		m_aRouteInfos = NULL;
	}
}

/// Serialization read
void CvCityConnections::Read(FDataStream& kStream)
{
	// Version number to maintain backwards compatibility
	uint uiVersion;
	kStream >> uiVersion;
	MOD_SERIALIZE_INIT_READ(kStream);

	kStream >> m_uiRouteInfosDimension;
	ResizeRouteInfo(m_uiRouteInfosDimension);
	for(uint ui = 0; ui < m_uiRouteInfosDimension * m_uiRouteInfosDimension; ui++)
	{
		kStream >> m_aRouteInfos[ui].m_cPassEval;
		kStream >> m_aRouteInfos[ui].m_cRouteState;
	}

	// read in city ids
	int iNumCityIDs;
	kStream >> iNumCityIDs;
	m_aiCityPlotIDs.clear();
	for(int i = 0; i < iNumCityIDs; i++)
	{
		int iValue;
		kStream >> iValue;
		m_aiCityPlotIDs.push_back(iValue);
	}
}

/// Serialization write
void CvCityConnections::Write(FDataStream& kStream) const
{
	// Current version number
	uint uiVersion = 1;
	kStream << uiVersion;
	MOD_SERIALIZE_INIT_WRITE(kStream);

	kStream << m_uiRouteInfosDimension;
	for(uint ui = 0; ui < m_uiRouteInfosDimension * m_uiRouteInfosDimension; ui++)
	{
		kStream << m_aRouteInfos[ui].m_cPassEval;
		kStream << m_aRouteInfos[ui].m_cRouteState;
	}

	kStream << m_aiCityPlotIDs.size();
	for(uint ui = 0; ui < m_aiCityPlotIDs.size(); ui++)
	{
		kStream << m_aiCityPlotIDs[ui];
	}
}

/// Update - called from within CvPlayer
void CvCityConnections::Update(void)
{
	if(m_pPlayer->isBarbarian())
	{
		return;
	}

	UpdatePlotRouteStates();
	UpdateCityPlotIDs();
	UpdateRouteInfo();
	BroadcastPlotRouteStateChanges();
}

/// Update the city ids to the correct ones
void CvCityConnections::UpdateCityPlotIDs(void)
{
	ResetCityPlotIDs();
	int iIndex = 0;
	int iLoop;
	CvCity* pLoopCity;

	for(uint ui = 0; ui < MAX_CIV_PLAYERS; ui++)
	{
		PlayerTypes ePlayer = (PlayerTypes)ui;
		if(GET_PLAYER(ePlayer).isBarbarian())
		{
			continue;
		}

		TeamTypes ePlayerTeam = GET_PLAYER(ePlayer).getTeam();
		TeamTypes eMyPlayerTeam = m_pPlayer->getTeam();

		if(ePlayerTeam == eMyPlayerTeam)
		{
			// player's city
			for(pLoopCity = GET_PLAYER(ePlayer).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(ePlayer).nextCity(&iLoop))
			{
				CvAssertMsg(pLoopCity->plot(), "pLoopCity does not have a plot. What??");
				int iPlotIndex = pLoopCity->plot()->GetPlotIndex();
				m_aiCityPlotIDs.push_back(iPlotIndex);
				iIndex++;
			}
		}
		else if(ShouldConnectToOtherPlayer(ePlayer))
		{
			CvCity* pOtherCapital = GET_PLAYER(ePlayer).getCapitalCity();
			if(pOtherCapital)
			{
				int iPlotIndex = pOtherCapital->plot()->GetPlotIndex();
				m_aiCityPlotIDs.push_back(iPlotIndex);
				iIndex++;
			}
		}
	}
}

CvCityConnections::RouteInfo* CvCityConnections::GetRouteInfo(uint uiFirstCityIndex, uint uiSecondCityIndex)
{
	if(uiFirstCityIndex >= m_uiRouteInfosDimension || uiSecondCityIndex >= m_uiRouteInfosDimension)
	{
		return NULL;
	}

	return &(m_aRouteInfos[uiFirstCityIndex * m_uiRouteInfosDimension + uiSecondCityIndex]);
}

/// Reset the route info array to have empty data
void CvCityConnections::ResetRouteInfo(void)
{
	RouteInfo* pRouteInfo = NULL;
	for(uint ui = 0; ui < m_uiRouteInfosDimension; ui++)
	{
		for(uint ui2 = 0; ui2 < m_uiRouteInfosDimension; ui2++)
		{
			pRouteInfo = GetRouteInfo(ui, ui2);
			if(!pRouteInfo)
			{
				continue;
			}

			pRouteInfo->m_cRouteState = 0;
			pRouteInfo->m_cPassEval = -1;
		}
	}
}

void CvCityConnections::UpdateRouteInfo(void)
{
	RouteTypes eBestRouteType = m_pPlayer->getBestRoute();

	// build city list
	FStaticVector<CvCity*, SAFE_ESTIMATE_NUM_CITIES, true, c_eCiv5GameplayDLL, 0> vpCities;
	CvCity* pLoopCity = NULL;
	int iLoop;

#if defined(MOD_EVENTS_CITY_CONNECTIONS)
	bool bAllowDirectRoutes = (eBestRouteType != NO_ROUTE);
	bool bAllowIndirectRoutes = false;
	
	bool bCallDirectEvents = false;
	bool bCallIndirectEvents = false;
#else
	bool bAllowWaterRoutes = false;
#endif

	// add all the cities we control and those that we want to connect to
	for(uint ui = 0; ui < MAX_CIV_PLAYERS; ui++)
	{
		PlayerTypes ePlayer = (PlayerTypes)ui;
		if(GET_PLAYER(ePlayer).isBarbarian())
		{
			continue;
		}

		if(ePlayer == m_pPlayer->GetID())
		{
			// player's city
			for(pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
			{
				vpCities.push_back(pLoopCity);
				pLoopCity->SetRouteToCapitalConnected(false);

#if defined(MOD_EVENTS_CITY_CONNECTIONS)
				if(!bAllowIndirectRoutes)
#else
				if(!bAllowWaterRoutes)
#endif
				{
					for(uint uiBuildingTypes = 0; uiBuildingTypes < m_aBuildingsAllowWaterRoutes.size(); uiBuildingTypes++)
					{
						if(pLoopCity->GetCityBuildings()->GetNumActiveBuilding(m_aBuildingsAllowWaterRoutes[uiBuildingTypes]) > 0)
						{
#if defined(MOD_EVENTS_CITY_CONNECTIONS)
							bAllowIndirectRoutes = true;
#else
							bAllowWaterRoutes = true;
#endif
						}
					}
				}
			}
		}
		else if(ShouldConnectToOtherPlayer(ePlayer))
		{
			CvCity* pOtherCapital = GET_PLAYER(ePlayer).getCapitalCity();
			if(pOtherCapital)
			{
				vpCities.push_back(pOtherCapital);
			}
		}
	}

	if(vpCities.size() > m_uiRouteInfosDimension)
	{
		ResizeRouteInfo((uint)((float)m_uiRouteInfosDimension * 1.5f));
	}
	ResetRouteInfo();
	
#if defined(MOD_EVENTS_CITY_CONNECTIONS)
	if (MOD_EVENTS_CITY_CONNECTIONS) {
		// Events to determine if we support alternative direct and/or indirect route types
		if (GAMEEVENTINVOKE_TESTANY(GAMEEVENT_CityConnections, m_pPlayer->GetID(), true) == GAMEEVENTRETURN_TRUE) {
			bAllowDirectRoutes = true;
			bCallDirectEvents = true;
		}

		if (GAMEEVENTINVOKE_TESTANY(GAMEEVENT_CityConnections, m_pPlayer->GetID(), false) == GAMEEVENTRETURN_TRUE) {
			bAllowIndirectRoutes = true;
			bCallIndirectEvents = true;
		}
	}
#endif

	// if the player can't build any routes, then we don't need to check this
#if defined(MOD_EVENTS_CITY_CONNECTIONS)
	if(!(bAllowDirectRoutes || bAllowIndirectRoutes))
#else
	if(eBestRouteType == NO_ROUTE && !bAllowWaterRoutes)
#endif
	{
		return;
	}

	// pass 0 = can cities connect via water routes
	// pass 1 = can cities connect via land and water routes
	for(int iPass = 0; iPass < 2; iPass++)
	{
#if defined(MOD_EVENTS_CITY_CONNECTIONS)
		if(iPass == 0 && !bAllowIndirectRoutes)  // if in the first pass and we can't create indirect routes, skip
#else
		if(iPass == 0 && !bAllowWaterRoutes)  // if in the first pass, we can't embark, skip
#endif
		{
			continue;
		}
#if defined(MOD_EVENTS_CITY_CONNECTIONS)
		else if(iPass == 1 && !(bAllowDirectRoutes || bAllowIndirectRoutes))  // if in the second pass and we can't create any routes (this allows for harbour to harbour connections without The Wheel), skip
#else
#if defined(MOD_BUGFIX_MINOR)
		else if(iPass == 1 && !(eBestRouteType != NO_ROUTE || bAllowWaterRoutes))  // if in the second pass, we can't build a road or a water connection, skip
#else
		else if(iPass == 1 && eBestRouteType == NO_ROUTE)  // if in the second pass, we can't build a road, skip
#endif
#endif
		{
			continue;
		}

		CvCity* pFirstCity = NULL;
		CvCity* pSecondCity = NULL;

		CvAStar* pkLandRouteFinder;
		pkLandRouteFinder = &GC.getRouteFinder();

		for(uint uiFirstCityIndex = 0; uiFirstCityIndex < vpCities.size(); uiFirstCityIndex++)
		{
			pFirstCity = vpCities[uiFirstCityIndex];
			int iFirstCityArrayIndex = GetIndexFromCity(pFirstCity);

			for(uint uiSecondCityIndex = 0; uiSecondCityIndex < vpCities.size(); uiSecondCityIndex++)
			{
				// same city! ignore
				if(uiSecondCityIndex == uiFirstCityIndex)
				{
					continue;
				}
				pSecondCity = vpCities[uiSecondCityIndex];
				int iSecondCityArrayIndex = GetIndexFromCity(pSecondCity);

				RouteInfo* pRouteInfo = GetRouteInfo(iFirstCityArrayIndex, iSecondCityArrayIndex);
				RouteInfo* pInverseRouteInfo = GetRouteInfo(iSecondCityArrayIndex, iFirstCityArrayIndex);

				// bail if either are null
				if(!pRouteInfo || !pInverseRouteInfo)
				{
					continue;
				}

				// if the route has already been evaluated, copy the data
				if(pInverseRouteInfo->m_cPassEval > iPass)
				{
					pRouteInfo->m_cPassEval = pInverseRouteInfo->m_cPassEval;
					pRouteInfo->m_cRouteState = pInverseRouteInfo->m_cRouteState;
					continue;
				}

#if !defined(MOD_EVENTS_CITY_CONNECTIONS)
				// this path already has an existing route (usually water)
				//if(pRouteInfo->m_cRouteState & (HAS_ANY_ROUTE | HAS_BEST_ROUTE | HAS_WATER_ROUTE))
				//{
				//	continue;
				//}
#endif

				pRouteInfo->m_cPassEval = iPass + 1;

				if(iPass == 0)  // check water route
				{
					// if either city is blockaded, don't consider a water connection
					if(pFirstCity->IsBlockaded() || pSecondCity->IsBlockaded())
					{
						continue;
					}

					bool bFirstCityHasHarbor = false;
					bool bSecondCityHasHarbor = false;

					// Loop through adding the available buildings
					for(int i = 0; i < (int)m_aBuildingsAllowWaterRoutes.size(); i++)
					{
						if(pFirstCity->GetCityBuildings()->GetNumActiveBuilding(m_aBuildingsAllowWaterRoutes[i]) > 0)
						{
							bFirstCityHasHarbor = true;
						}

						if(pSecondCity->GetCityBuildings()->GetNumActiveBuilding(m_aBuildingsAllowWaterRoutes[i]) > 0)
						{
							bSecondCityHasHarbor = true;
						}
					}

					if(bFirstCityHasHarbor && bSecondCityHasHarbor)
					{
						if(GC.GetWaterRouteFinder().GeneratePath(pFirstCity->getX(), pFirstCity->getY(), pSecondCity->getX(), pSecondCity->getY(), m_pPlayer->GetID(), true))
						{
#if defined(MOD_EVENTS_CITY_CONNECTIONS)
							pRouteInfo->m_cRouteState |= HAS_ANY_ROUTE | HAS_INDIRECT_ROUTE;
#else
							pRouteInfo->m_cRouteState |= HAS_ANY_ROUTE | HAS_WATER_ROUTE;
#endif
						}
					}

#if defined(MOD_EVENTS_CITY_CONNECTIONS)
					if (!(pRouteInfo->m_cRouteState & HAS_ANY_ROUTE) && bCallIndirectEvents)
					{
						// Event to determine if pFirstCity is connected to pSecondCity by an alternative indirect route type
						if (GAMEEVENTINVOKE_TESTANY(GAMEEVENT_CityConnected, m_pPlayer->GetID(), pFirstCity->getX(), pFirstCity->getY(), pSecondCity->getX(), pSecondCity->getY(), false) == GAMEEVENTRETURN_TRUE) {
							pRouteInfo->m_cRouteState |= HAS_ANY_ROUTE | HAS_INDIRECT_ROUTE;
						}
					}
#endif
				}
				else if(iPass == 1)  // check land route
				{
					bool bAnyRouteFound = false;
#if !defined(MOD_EVENTS_CITY_CONNECTIONS)
					bool bBestRouteFound = false;

					// assuming that there are fewer than 256 players
					int iRouteValue = eBestRouteType + 1;
					int iPathfinderFlags = (iRouteValue << 8);

					if(pkLandRouteFinder->GeneratePath(pFirstCity->getX(), pFirstCity->getY(), pSecondCity->getX(), pSecondCity->getY(), iPathfinderFlags | m_pPlayer->GetID(), true))
					{
						bAnyRouteFound = true;
						bBestRouteFound = true;
					}
#endif

#if !defined(MOD_EVENTS_CITY_CONNECTIONS)
					if(!bAnyRouteFound)
#endif
					{
						if(pkLandRouteFinder->GeneratePath(pFirstCity->getX(), pFirstCity->getY(), pSecondCity->getX(), pSecondCity->getY(), MOVE_ANY_ROUTE | m_pPlayer->GetID(), true))
						{
							bAnyRouteFound = true;
						}
					}

#if defined(MOD_EVENTS_CITY_CONNECTIONS)
					if (!bAnyRouteFound && bCallDirectEvents)
					{
						// Event to determine if pFirstCity is connected to pSecondCity by an alternative direct route type
						if (GAMEEVENTINVOKE_TESTANY(GAMEEVENT_CityConnected, m_pPlayer->GetID(), pFirstCity->getX(), pFirstCity->getY(), pSecondCity->getX(), pSecondCity->getY(), true) == GAMEEVENTRETURN_TRUE) {
							bAnyRouteFound = true;
						}
					}
#endif

#if !defined(MOD_EVENTS_CITY_CONNECTIONS)
					if(bBestRouteFound)
					{
						pRouteInfo->m_cRouteState |= HAS_BEST_ROUTE | HAS_ANY_ROUTE;
					}
					else if(bAnyRouteFound)
#else
					if(bAnyRouteFound)
#endif
					{
						pRouteInfo->m_cRouteState |= HAS_ANY_ROUTE;
					}

					// walk through the nodes for plot route info
					if(pFirstCity->isCapital() || pSecondCity->isCapital())
					{
						if(bAnyRouteFound)
						{
							CvPlot* pPlot = NULL;
							CvAStarNode* pNode = pkLandRouteFinder->GetLastNode();
							while(pNode)
							{
								pPlot = GC.getMap().plot(pNode->m_iX, pNode->m_iY);
								ConnectPlotRoute(pPlot);
								pNode = pNode->m_pParent;
							}

							pFirstCity->SetRouteToCapitalConnected(true);
							pSecondCity->SetRouteToCapitalConnected(true);
						}
					}
				}
			}
		}
	}
}

/// Reset the city id array to have invalid data
void CvCityConnections::ResetCityPlotIDs(void)
{
	m_aiCityPlotIDs.clear();
}

/// if there are no cities in the route list
bool CvCityConnections::IsEmpty(void)
{
	if(m_aiCityPlotIDs.size() > 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

/// Get the index value from the city passed in
uint CvCityConnections::GetIndexFromCity(CvCity* pCity)
{
	CvCity* pOtherCity = NULL;
	for(uint ui = 0; ui < m_aiCityPlotIDs.size(); ui++)
	{
		int iPlotIndex = m_aiCityPlotIDs[ui];
		CvPlot* pPlot = GC.getMap().plotByIndex(iPlotIndex);
		CvAssertMsg(pPlot, "invalid plot. whut??");
		if(pPlot)
		{
			pOtherCity = pPlot->getPlotCity();
			CvAssertMsg(pOtherCity, "No city on this plot. Whut?");

			if(pOtherCity == pCity)
			{
				return ui;
			}
		}
	}

	CvAssertMsg(false, "City not found");
	return UINT_MAX;
}

CvCity* CvCityConnections::GetCityFromIndex(int iIndex)
{
	int iPlotIndex = m_aiCityPlotIDs[iIndex];
	CvPlot* pPlot = GC.getMap().plotByIndex(iPlotIndex);
	CvAssertMsg(pPlot, "invalid plot. whut??");

	if(!pPlot) return 0;

	CvCity* pCity = pPlot->getPlotCity();
	CvAssertMsg(pCity, "No city on this plot. Whut?");

	return pCity;
}

uint CvCityConnections::GetNumConnectableCities(void)
{
	return m_aiCityPlotIDs.size();
}


bool CvCityConnections::ShouldConnectToOtherPlayer(PlayerTypes eOtherPlayer)
{
	bool result = false;

	// shouldn't be able to connect to yourself
	if(m_pPlayer->GetID() == eOtherPlayer)
	{
		return false;
	}

	if(!GET_PLAYER(eOtherPlayer).isAlive())
	{
		return false;
	}

	if(GET_PLAYER(eOtherPlayer).isBarbarian())
	{
		return false;
	}

	CvPlayer* pOtherPlayer = &(GET_PLAYER(eOtherPlayer));

	// only majors and minors should connect to each other at this point.
	bool bMajorMinor = m_pPlayer->isMinorCiv() != pOtherPlayer->isMinorCiv();
	if(!bMajorMinor)
	{
		return false;
	}

	if(m_pPlayer->isMinorCiv())
	{
		CvPlayer* pMajorCiv = pOtherPlayer;

		// If the major is a human, don't decide a connection to a minor is desirable on their behalf
		if(pMajorCiv->isHuman())
		{
			return false;
		}

		if(!m_pPlayer->GetMinorCivAI()->IsActiveQuestForPlayer(pMajorCiv->GetID(), MINOR_CIV_QUEST_ROUTE))
		{
			return false;
		}

		result = true;
	}
	else // player is a major
	{
		CvPlayer* pMinorPlayer = pOtherPlayer;
		if(!pMinorPlayer->isAlive())
		{
			return false;
		}

		// If the major is a human, don't decide a connection to a minor is desirable on their behalf
		if(m_pPlayer->isHuman())
		{
			return false;
		}

		if(!m_pPlayer->GetDiplomacyAI()->IsWantToRouteConnectToMinor(pMinorPlayer->GetID()))
		{
			return false;
		}

		if(!pMinorPlayer->GetMinorCivAI()->IsActiveQuestForPlayer(m_pPlayer->GetID(), MINOR_CIV_QUEST_ROUTE))
		{
			return false;
		}

		result = true;
	}

	return result;
}

void CvCityConnections::UpdatePlotRouteStates(void)
{
	for(uint ui = 0; ui < m_aPlotRouteInfos.size(); ui++)
	{
		if(m_aPlotRouteInfos[ui].m_bPlotRouteState & CONNECTION)
		{
			m_aPlotRouteInfos[ui].m_bPlotRouteState = CONNECTION_LAST_TURN;
		}
		else
		{
			m_aPlotRouteInfos[ui].m_bPlotRouteState = NO_CONNECTION;
		}
	}
}

void CvCityConnections::BroadcastPlotRouteStateChanges(void)
{
	for(uint ui = 0; ui < m_aPlotRouteInfos.size(); ui++)
	{
		if(m_aPlotRouteInfos[ui].m_bPlotRouteState & CONNECTION_LAST_TURN)
		{
			if(!(m_aPlotRouteInfos[ui].m_bPlotRouteState & CONNECTION))
			{
				// indicate removed route
				CvPlot* pPlot = GC.getMap().plotByIndex(m_aPlotRouteInfos[ui].m_iPlotIndex);
				pPlot->SetTradeRoute(m_pPlayer->GetID(), false);
			}
		}
		else
		{
			if(m_aPlotRouteInfos[ui].m_bPlotRouteState & CONNECTION)
			{
				// broadcast new connected trade route
				CvPlot* pPlot = GC.getMap().plotByIndex(m_aPlotRouteInfos[ui].m_iPlotIndex);
				pPlot->SetTradeRoute(m_pPlayer->GetID(), true);
			}
		}
	}
}

void CvCityConnections::ConnectPlotRoute(CvPlot* pPlot)
{
	int iPlotIndex = pPlot->GetPlotIndex();
	uint uiTargetIndex = MAX_UNSIGNED_INT;

	for(uint ui = 0; ui < m_aPlotRouteInfos.size(); ui++)
	{
		if(m_aPlotRouteInfos[ui].m_iPlotIndex == iPlotIndex)
		{
			uiTargetIndex = ui;
			break;
		}
		else if(uiTargetIndex == MAX_UNSIGNED_INT && m_aPlotRouteInfos[ui].m_bPlotRouteState == NO_CONNECTION)
		{
			uiTargetIndex = ui;
		}
	}

	if(uiTargetIndex >= m_aPlotRouteInfos.size())
	{
		uiTargetIndex = m_aPlotRouteInfos.size();
		PlotRouteInfo info;
		m_aPlotRouteInfos.push_back(info);
		m_aPlotRouteInfos[uiTargetIndex].m_bPlotRouteState = 0;
	}

	m_aPlotRouteInfos[uiTargetIndex].m_iPlotIndex = iPlotIndex;
	m_aPlotRouteInfos[uiTargetIndex].m_bPlotRouteState |= CONNECTION;
}

void CvCityConnections::ResizeRouteInfo(uint uiNewSize)
{
	if(m_aRouteInfos)
	{
		delete [] m_aRouteInfos;
		m_aRouteInfos = NULL;
	}

	m_aRouteInfos = new RouteInfo[uiNewSize * uiNewSize];
	CvAssertMsg(m_aRouteInfos, "m_aRouteInfo null");
	m_uiRouteInfosDimension = uiNewSize;
}
