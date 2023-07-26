/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#include "CvGameCoreDLLPCH.h"
#include "CvGrandStrategyAI.h"
#include "CvEconomicAI.h"
#include "CvCitySpecializationAI.h"
#include "CvDiplomacyAI.h"
#include "CvMinorCivAI.h"
#include "ICvDLLUserInterface.h"

// must be included after all other headers
#include "LintFree.h"

//------------------------------------------------------------------------------
CvAIGrandStrategyXMLEntry::CvAIGrandStrategyXMLEntry(void):
	m_piFlavorValue(NULL),
	m_piSpecializationBoost(NULL),
	m_piFlavorModValue(NULL)
{
}
//------------------------------------------------------------------------------
CvAIGrandStrategyXMLEntry::~CvAIGrandStrategyXMLEntry(void)
{
	SAFE_DELETE_ARRAY(m_piFlavorValue);
	SAFE_DELETE_ARRAY(m_piSpecializationBoost);
	SAFE_DELETE_ARRAY(m_piFlavorModValue);
}
//------------------------------------------------------------------------------
bool CvAIGrandStrategyXMLEntry::CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility)
{
	if(!CvBaseInfo::CacheResults(kResults, kUtility))
		return false;

	//Arrays
	const char* szType = GetType();
	kUtility.SetFlavors(m_piFlavorValue, "AIGrandStrategy_Flavors", "AIGrandStrategyType", szType);
	kUtility.SetYields(m_piSpecializationBoost, "AIGrandStrategy_Yields", "AIGrandStrategyType", szType);
	kUtility.SetFlavors(m_piFlavorModValue, "AIGrandStrategy_FlavorMods", "AIGrandStrategyType", szType);

	return true;
}

/// What Flavors will be added by adopting this Grand Strategy?
int CvAIGrandStrategyXMLEntry::GetFlavorValue(int i) const
{
	FAssertMsg(i < GC.getNumFlavorTypes(), "Index out of bounds");
	FAssertMsg(i > -1, "Index out of bounds");
	return m_piFlavorValue ? m_piFlavorValue[i] : -1;
}

/// What Flavors will be added by adopting this Grand Strategy?
int CvAIGrandStrategyXMLEntry::GetSpecializationBoost(YieldTypes eYield) const
{
	FAssertMsg(eYield < NUM_YIELD_TYPES, "Index out of bounds");
	FAssertMsg(eYield > -1, "Index out of bounds");
	return m_piSpecializationBoost ? m_piSpecializationBoost[(int)eYield] : 0;
}

/// What Flavors will be added by adopting this Grand Strategy?
int CvAIGrandStrategyXMLEntry::GetFlavorModValue(int i) const
{
	FAssertMsg(i < GC.getNumFlavorTypes(), "Index out of bounds");
	FAssertMsg(i > -1, "Index out of bounds");
	return m_piFlavorModValue ? m_piFlavorModValue[i] : 0;
}



//=====================================
// CvAIGrandStrategyXMLEntries
//=====================================
/// Constructor
CvAIGrandStrategyXMLEntries::CvAIGrandStrategyXMLEntries(void)
{

}

/// Destructor
CvAIGrandStrategyXMLEntries::~CvAIGrandStrategyXMLEntries(void)
{
	DeleteArray();
}

/// Returns vector of AIStrategy entries
std::vector<CvAIGrandStrategyXMLEntry*>& CvAIGrandStrategyXMLEntries::GetAIGrandStrategyEntries()
{
	return m_paAIGrandStrategyEntries;
}

/// Number of defined AIStrategies
int CvAIGrandStrategyXMLEntries::GetNumAIGrandStrategies()
{
	return m_paAIGrandStrategyEntries.size();
}

/// Clear AIStrategy entries
void CvAIGrandStrategyXMLEntries::DeleteArray()
{
	for(std::vector<CvAIGrandStrategyXMLEntry*>::iterator it = m_paAIGrandStrategyEntries.begin(); it != m_paAIGrandStrategyEntries.end(); ++it)
	{
		SAFE_DELETE(*it);
	}

	m_paAIGrandStrategyEntries.clear();
}

/// Get a specific entry
CvAIGrandStrategyXMLEntry* CvAIGrandStrategyXMLEntries::GetEntry(int index)
{
	return m_paAIGrandStrategyEntries[index];
}



//=====================================
// CvGrandStrategyAI
//=====================================
/// Constructor
CvGrandStrategyAI::CvGrandStrategyAI():
	m_paiGrandStrategyPriority(NULL),
	m_eGuessOtherPlayerActiveGrandStrategy(NULL),
	m_eGuessOtherPlayerActiveGrandStrategyConfidence(NULL)
{
}

/// Destructor
CvGrandStrategyAI::~CvGrandStrategyAI(void)
{
}

/// Initialize
void CvGrandStrategyAI::Init(CvAIGrandStrategyXMLEntries* pAIGrandStrategies, CvPlayer* pPlayer)
{
	// Store off the pointer to the AIStrategies active for this game
	m_pAIGrandStrategies = pAIGrandStrategies;

	m_pPlayer = pPlayer;

	// Initialize AIGrandStrategy status array
	FAssertMsg(m_paiGrandStrategyPriority==NULL, "about to leak memory, CvGrandStrategyAI::m_paiGrandStrategyPriority");
	m_paiGrandStrategyPriority = FNEW(int[m_pAIGrandStrategies->GetNumAIGrandStrategies()], c_eCiv5GameplayDLL, 0);

	FAssertMsg(m_eGuessOtherPlayerActiveGrandStrategy==NULL, "about to leak memory, CvGrandStrategyAI::m_eGuessOtherPlayerActiveGrandStrategy");
	m_eGuessOtherPlayerActiveGrandStrategy = FNEW(int[MAX_MAJOR_CIVS], c_eCiv5GameplayDLL, 0);

	FAssertMsg(m_eGuessOtherPlayerActiveGrandStrategyConfidence==NULL, "about to leak memory, CvGrandStrategyAI::m_eGuessOtherPlayerActiveGrandStrategyConfidence");
	m_eGuessOtherPlayerActiveGrandStrategyConfidence = FNEW(int[MAX_MAJOR_CIVS], c_eCiv5GameplayDLL, 0);

	Reset();
}

/// Deallocate memory created in initialize
void CvGrandStrategyAI::Uninit()
{
	SAFE_DELETE_ARRAY(m_paiGrandStrategyPriority);
	SAFE_DELETE_ARRAY(m_eGuessOtherPlayerActiveGrandStrategy);
	SAFE_DELETE_ARRAY(m_eGuessOtherPlayerActiveGrandStrategyConfidence);
}

/// Reset AIStrategy status array to all false
void CvGrandStrategyAI::Reset()
{
	int iI;

	m_iNumTurnsSinceActiveSet = 0;

	m_eActiveGrandStrategy = NO_AIGRANDSTRATEGY;

	for(iI = 0; iI < m_pAIGrandStrategies->GetNumAIGrandStrategies(); iI++)
	{
		m_paiGrandStrategyPriority[iI] = -1;
	}

	for(iI = 0; iI < MAX_MAJOR_CIVS; iI++)
	{
		m_eGuessOtherPlayerActiveGrandStrategy[iI] = NO_AIGRANDSTRATEGY;
		m_eGuessOtherPlayerActiveGrandStrategyConfidence[iI] = NO_GUESS_CONFIDENCE_TYPE;
	}
}

/// Serialization read
void CvGrandStrategyAI::Read(FDataStream& kStream)
{
	// Version number to maintain backwards compatibility
	uint uiVersion;
	kStream >> uiVersion;
	MOD_SERIALIZE_INIT_READ(kStream);

	kStream >> m_iNumTurnsSinceActiveSet;
	kStream >> (int&)m_eActiveGrandStrategy;

	FAssertMsg(m_pAIGrandStrategies != NULL && m_pAIGrandStrategies->GetNumAIGrandStrategies() > 0, "Number of AIGrandStrategies to serialize is expected to greater than 0");
#ifdef _MSC_VER
// JAR - if m_pAIGrandStrategies can be NULL at this point,
// the load will fail if the data isn't read. Better to crash
// here where the problem is than defer it.
#pragma warning ( push )
#pragma warning ( disable : 6011 )
#endif//_MSC_VER
	ArrayWrapper<int> wrapm_paiGrandStrategyPriority(m_pAIGrandStrategies->GetNumAIGrandStrategies(), m_paiGrandStrategyPriority);
#ifdef _MSC_VER
#pragma warning ( pop )
#endif//_MSC_VER

	kStream >> wrapm_paiGrandStrategyPriority;

	ArrayWrapper<int> wrapm_eGuessOtherPlayerActiveGrandStrategy(MAX_MAJOR_CIVS, m_eGuessOtherPlayerActiveGrandStrategy);
	kStream >> wrapm_eGuessOtherPlayerActiveGrandStrategy;

	ArrayWrapper<int> wrapm_eGuessOtherPlayerActiveGrandStrategyConfidence(MAX_MAJOR_CIVS, m_eGuessOtherPlayerActiveGrandStrategyConfidence);
	kStream >> wrapm_eGuessOtherPlayerActiveGrandStrategyConfidence;

}

/// Serialization write
void CvGrandStrategyAI::Write(FDataStream& kStream)
{
	// Current version number
	uint uiVersion = 1;
	kStream << uiVersion;
	MOD_SERIALIZE_INIT_WRITE(kStream);

	kStream << m_iNumTurnsSinceActiveSet;
	kStream << m_eActiveGrandStrategy;

	FAssertMsg(GC.getNumAIGrandStrategyInfos() > 0, "Number of AIStrategies to serialize is expected to greater than 0");
	kStream << ArrayWrapper<int>(m_pAIGrandStrategies->GetNumAIGrandStrategies(), m_paiGrandStrategyPriority);

	kStream << ArrayWrapper<int>(MAX_MAJOR_CIVS, m_eGuessOtherPlayerActiveGrandStrategy);
	kStream << ArrayWrapper<int>(MAX_MAJOR_CIVS, m_eGuessOtherPlayerActiveGrandStrategyConfidence);
}

/// Returns the Player object the Strategies are associated with
CvPlayer* CvGrandStrategyAI::GetPlayer()
{
	return m_pPlayer;
}

/// Returns AIGrandStrategies object stored in this class
CvAIGrandStrategyXMLEntries* CvGrandStrategyAI::GetAIGrandStrategies()
{
	return m_pAIGrandStrategies;
}

/// Runs every turn to determine what the player's Active Grand Strategy is and to change Priority Levels as necessary
void CvGrandStrategyAI::DoTurn()
{
	DoGuessOtherPlayersActiveGrandStrategy();

	int iGrandStrategiesLoop;
	AIGrandStrategyTypes eGrandStrategy;
	CvAIGrandStrategyXMLEntry* pGrandStrategy;
	CvString strGrandStrategyName;

	// Loop through all GrandStrategies to set their Priorities
	for(iGrandStrategiesLoop = 0; iGrandStrategiesLoop < GetAIGrandStrategies()->GetNumAIGrandStrategies(); iGrandStrategiesLoop++)
	{
		eGrandStrategy = (AIGrandStrategyTypes) iGrandStrategiesLoop;
		pGrandStrategy = GetAIGrandStrategies()->GetEntry(iGrandStrategiesLoop);
		strGrandStrategyName = (CvString) pGrandStrategy->GetType();

		// Base Priority looks at Personality Flavors (0 - 10) and multiplies * the Flavors attached to a Grand Strategy (0-10),
		// so expect a number between 0 and 100 back from this
		int iPriority = GetBaseGrandStrategyPriority(eGrandStrategy);

		if(strGrandStrategyName == "AIGRANDSTRATEGY_CONQUEST")
		{
			iPriority += GetConquestPriority();
		}
		else if(strGrandStrategyName == "AIGRANDSTRATEGY_CULTURE")
		{
			iPriority += GetCulturePriority();
		}
		else if(strGrandStrategyName == "AIGRANDSTRATEGY_UNITED_NATIONS")
		{
			iPriority += GetUnitedNationsPriority();
		}
		else if(strGrandStrategyName == "AIGRANDSTRATEGY_SPACESHIP")
		{
			iPriority += GetSpaceshipPriority();
		}

		// Random element
		iPriority += GC.getGame().getJonRandNum(/*50*/ GC.getAI_GS_RAND_ROLL(), "Grand Strategy AI: GS rand roll.");

		// Give a boost to the current strategy so that small fluctuation doesn't cause a big change
		if(GetActiveGrandStrategy() == eGrandStrategy && GetActiveGrandStrategy() != NO_AIGRANDSTRATEGY)
		{
			iPriority += /*50*/ GC.getAI_GRAND_STRATEGY_CURRENT_STRATEGY_WEIGHT();
		}

		SetGrandStrategyPriority(eGrandStrategy, iPriority);
	}

	// Now look at what we think the other players in the game are up to - we might have an opportunity to capitalize somewhere
	int iNumPlayersAliveAndMet = 0;

	int iMajorLoop;

	for(iMajorLoop = 0; iMajorLoop < MAX_MAJOR_CIVS; iMajorLoop++)
	{
		if(GET_PLAYER((PlayerTypes) iMajorLoop).isAlive())
		{
			if(GET_TEAM(GetPlayer()->getTeam()).isHasMet(GET_PLAYER((PlayerTypes) iMajorLoop).getTeam()))
			{
				iNumPlayersAliveAndMet++;
			}
		}
	}

	FStaticVector< int, 5, true, c_eCiv5GameplayDLL > viNumGrandStrategiesAdopted;
	int iNumPlayers;

	// Init vector
	for(iGrandStrategiesLoop = 0; iGrandStrategiesLoop < GetAIGrandStrategies()->GetNumAIGrandStrategies(); iGrandStrategiesLoop++)
	{
		iNumPlayers = 0;

		// Tally up how many players we think are pusuing each Grand Strategy
		for(iMajorLoop = 0; iMajorLoop < MAX_MAJOR_CIVS; iMajorLoop++)
		{
			if(GetGuessOtherPlayerActiveGrandStrategy((PlayerTypes) iMajorLoop) == (AIGrandStrategyTypes) iGrandStrategiesLoop)
			{
				iNumPlayers++;
			}
		}

		viNumGrandStrategiesAdopted.push_back(iNumPlayers);
	}

	FStaticVector< int, 5, true, c_eCiv5GameplayDLL > viGrandStrategyChangeForLogging;

	int iChange;

	// Now modify our preferences based on how many people are going for stuff
	for(iGrandStrategiesLoop = 0; iGrandStrategiesLoop < GetAIGrandStrategies()->GetNumAIGrandStrategies(); iGrandStrategiesLoop++)
	{
		eGrandStrategy = (AIGrandStrategyTypes) iGrandStrategiesLoop;
		// If EVERYONE else we know is also going for this Grand Strategy, reduce our Priority by 50%
		iChange = GetGrandStrategyPriority(eGrandStrategy) * /*50*/ GC.getAI_GRAND_STRATEGY_OTHER_PLAYERS_GS_MULTIPLIER();
		iChange = iChange * viNumGrandStrategiesAdopted[eGrandStrategy] / iNumPlayersAliveAndMet;
		iChange /= 100;

		ChangeGrandStrategyPriority(eGrandStrategy, -iChange);

		viGrandStrategyChangeForLogging.push_back(-iChange);
	}

	ChangeNumTurnsSinceActiveSet(1);

	// Now see which Grand Strategy should be active, based on who has the highest Priority right now
	// Grand Strategy must be run for at least 10 turns
	if(GetActiveGrandStrategy() == NO_AIGRANDSTRATEGY || GetNumTurnsSinceActiveSet() >= /*10*/ GC.getAI_GRAND_STRATEGY_NUM_TURNS_STRATEGY_MUST_BE_ACTIVE())
	{
		int iBestPriority = -1;
		int iPriority;

		AIGrandStrategyTypes eBestGrandStrategy = NO_AIGRANDSTRATEGY;

		for(iGrandStrategiesLoop = 0; iGrandStrategiesLoop < GetAIGrandStrategies()->GetNumAIGrandStrategies(); iGrandStrategiesLoop++)
		{
			eGrandStrategy = (AIGrandStrategyTypes) iGrandStrategiesLoop;

			iPriority = GetGrandStrategyPriority(eGrandStrategy);

			if(iPriority > iBestPriority)
			{
				iBestPriority = iPriority;
				eBestGrandStrategy = eGrandStrategy;
			}
		}

		if(eBestGrandStrategy != GetActiveGrandStrategy())
		{
			SetActiveGrandStrategy(eBestGrandStrategy);
			m_pPlayer->GetCitySpecializationAI()->SetSpecializationsDirty(SPECIALIZATION_UPDATE_NEW_GRAND_STRATEGY);
		}
	}

	LogGrandStrategies(viGrandStrategyChangeForLogging);
}

/// Returns Priority for Conquest Grand Strategy
int CvGrandStrategyAI::GetConquestPriority()
{
	int iPriority = 0;

	// If Conquest Victory isn't even available then don't bother with anything
	VictoryTypes eVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_DOMINATION", true);
	if(eVictory == NO_VICTORY || !GC.getGame().isVictoryValid(eVictory))
	{
		if(!GC.getGame().areNoVictoriesValid())
		{
			return -100;
		}
	}

	int iGeneralWarlikeness = GetPlayer()->GetDiplomacyAI()->GetPersonalityMajorCivApproachBias(MAJOR_CIV_APPROACH_WAR);
	int iGeneralHostility = GetPlayer()->GetDiplomacyAI()->GetPersonalityMajorCivApproachBias(MAJOR_CIV_APPROACH_HOSTILE);
	int iGeneralDeceptiveness = GetPlayer()->GetDiplomacyAI()->GetPersonalityMajorCivApproachBias(MAJOR_CIV_APPROACH_DECEPTIVE);
	int iGeneralFriendliness = GetPlayer()->GetDiplomacyAI()->GetPersonalityMajorCivApproachBias(MAJOR_CIV_APPROACH_FRIENDLY);

	int iGeneralApproachModifier = max(max(iGeneralDeceptiveness, iGeneralHostility),iGeneralWarlikeness) - iGeneralFriendliness;
	// Boldness gives the base weight for Conquest (no flavors added earlier)
	iPriority += ((GetPlayer()->GetDiplomacyAI()->GetBoldness() + iGeneralApproachModifier) * (12 - m_pPlayer->GetCurrentEra())); // make a little less likely as time goes on

	CvTeam& pTeam = GET_TEAM(GetPlayer()->getTeam());

	// How many turns must have passed before we test for having met nobody?
	if(GC.getGame().getElapsedGameTurns() >= /*20*/ GC.getAI_GS_CONQUEST_NOBODY_MET_FIRST_TURN())
	{
		// If we haven't met any Major Civs yet, then we probably shouldn't be planning on conquering the world
		bool bHasMetMajor = false;

		for(int iTeamLoop = 0; iTeamLoop < MAX_CIV_TEAMS; iTeamLoop++)
		{
			if(pTeam.GetID() != iTeamLoop && !GET_TEAM((TeamTypes) iTeamLoop).isMinorCiv())
			{
				if(pTeam.isHasMet((TeamTypes) iTeamLoop))
				{
					bHasMetMajor = true;
					break;
				}
			}
		}
		if(!bHasMetMajor)
		{
			iPriority += /*-50*/ GC.getAI_GRAND_STRATEGY_CONQUEST_NOBODY_MET_WEIGHT();
		}
	}

	// How many turns must have passed before we test for us having a weak military?
	if(GC.getGame().getElapsedGameTurns() >= /*60*/ GC.getAI_GS_CONQUEST_MILITARY_STRENGTH_FIRST_TURN())
	{
		// Compare our military strength to the rest of the world
		int iWorldMilitaryStrength = GC.getGame().GetWorldMilitaryStrengthAverage(GetPlayer()->GetID(), true, true);

		if(iWorldMilitaryStrength > 0)
		{
			int iMilitaryRatio = (GetPlayer()->GetMilitaryMight() - iWorldMilitaryStrength) * /*100*/ GC.getAI_GRAND_STRATEGY_CONQUEST_POWER_RATIO_MULTIPLIER() / iWorldMilitaryStrength;

			// Make the likelihood of BECOMING a warmonger lower than dropping the bad behavior
			if(iMilitaryRatio > 0)
				iMilitaryRatio /= 2;

			iPriority += iMilitaryRatio;	// This will add between -100 and 100 depending on this player's MilitaryStrength relative the world average. The number will typically be near 0 though, as it's fairly hard to get away from the world average
		}
	}

	// If we're at war, then boost the weight a bit
	if(pTeam.getAtWarCount(/*bIgnoreMinors*/ false) > 0)
	{
		iPriority += /*10*/ GC.getAI_GRAND_STRATEGY_CONQUEST_AT_WAR_WEIGHT();
	}

	// If our neighbors are cramping our style, consider less... scrupulous means of obtaining more land
	if(GetPlayer()->IsCramped())
	{
		PlayerTypes ePlayer;
		int iNumPlayersMet = 1;	// Include 1 for me!
		int iTotalLandMe = 0;
		int iTotalLandPlayersMet = 0;

		// Count the number of Majors we know
		for(int iMajorLoop = 0; iMajorLoop < MAX_MAJOR_CIVS; iMajorLoop++)
		{
			ePlayer = (PlayerTypes) iMajorLoop;

			if(GET_PLAYER(ePlayer).isAlive() && iMajorLoop != GetPlayer()->GetID())
			{
				if(pTeam.isHasMet(GET_PLAYER(ePlayer).getTeam()))
				{
					iNumPlayersMet++;
				}
			}
		}

		if(iNumPlayersMet > 0)
		{
			// Check every plot for ownership
			for(int iPlotLoop = 0; iPlotLoop < GC.getMap().numPlots(); iPlotLoop++)
			{
				if(GC.getMap().plotByIndexUnchecked(iPlotLoop)->isOwned())
				{
					ePlayer = GC.getMap().plotByIndexUnchecked(iPlotLoop)->getOwner();

					if(ePlayer == GetPlayer()->GetID())
					{
						iTotalLandPlayersMet++;
						iTotalLandMe++;
					}
					else if(!GET_PLAYER(ePlayer).isMinorCiv() && pTeam.isHasMet(GET_PLAYER(ePlayer).getTeam()))
					{
						iTotalLandPlayersMet++;
					}
				}
			}

			iTotalLandPlayersMet /= iNumPlayersMet;

			if(iTotalLandMe > 0)
			{
				if(iTotalLandPlayersMet / iTotalLandMe > 0)
				{
					iPriority += /*20*/ GC.getAI_GRAND_STRATEGY_CONQUEST_CRAMPED_WEIGHT();
				}
			}
		}
	}

	// if we do not have nukes and we know someone else who does...
	if(GetPlayer()->getNumNukeUnits() == 0)
	{
		for(int iMajorLoop = 0; iMajorLoop < MAX_MAJOR_CIVS; iMajorLoop++)
		{
			PlayerTypes ePlayer = (PlayerTypes) iMajorLoop;

			if(GET_PLAYER(ePlayer).isAlive() && iMajorLoop != GetPlayer()->GetID())
			{
				if(pTeam.isHasMet(GET_PLAYER(ePlayer).getTeam()))
				{
					if (GET_PLAYER(ePlayer).getNumNukeUnits() > 0)
					{
						iPriority -= 50; 
						break;
					}
				}
			}
		}
	}

	return iPriority;
}

/// Returns Priority for Culture Grand Strategy
int CvGrandStrategyAI::GetCulturePriority()
{
	int iPriority = 0;

	// If Culture Victory isn't even available then don't bother with anything
	VictoryTypes eVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_CULTURAL", true);
	if(eVictory == NO_VICTORY || !GC.getGame().isVictoryValid(eVictory))
	{
		return -100;
	}

	// Before tourism kicks in, add weight based on flavor
	int iFlavorCulture =  m_pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_CULTURE"));
#if defined(MOD_AI_SMART_V3)
	if (MOD_AI_SMART_V3)
		iPriority += (9 - m_pPlayer->GetCurrentEra()) * iFlavorCulture * 200 / 100;
	else
#endif	
		iPriority += (10 - m_pPlayer->GetCurrentEra()) * iFlavorCulture * 200 / 100;

	// Loop through Players to see how we are doing on Tourism and Culture
	PlayerTypes eLoopPlayer;
	int iOurCulture = m_pPlayer->GetTotalJONSCulturePerTurn();
	int iOurTourism = m_pPlayer->GetCulture()->GetTourism();
	int iNumCivsBehindCulture = 0;
	int iNumCivsAheadCulture = 0;
	int iNumCivsBehindTourism = 0;
	int iNumCivsAheadTourism = 0;
	int iNumCivsAlive = 0;

	for(int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
	{
		eLoopPlayer = (PlayerTypes) iPlayerLoop;
		CvPlayer &kPlayer = GET_PLAYER(eLoopPlayer);

		if (kPlayer.isAlive() && !kPlayer.isMinorCiv() && !kPlayer.isBarbarian() && iPlayerLoop != m_pPlayer->GetID())
		{
			if (iOurCulture > kPlayer.GetTotalJONSCulturePerTurn())
			{
				iNumCivsAheadCulture++;
			}
			else
			{
				iNumCivsBehindCulture++;
			}
			if (iOurTourism > kPlayer.GetCulture()->GetTourism())
			{
				iNumCivsAheadTourism++;
			}
			else
			{
				iNumCivsBehindTourism++;
			}
			iNumCivsAlive++;
		}
	}

	if (iNumCivsAlive > 0 && iNumCivsAheadCulture > iNumCivsBehindCulture)
	{
		iPriority += (GC.getAI_GS_CULTURE_AHEAD_WEIGHT() * (iNumCivsAheadCulture - iNumCivsBehindCulture) / iNumCivsAlive);
	}
	if (iNumCivsAlive > 0 && iNumCivsAheadTourism > iNumCivsBehindTourism)
	{
		iPriority += (GC.getAI_GS_CULTURE_TOURISM_AHEAD_WEIGHT() * (iNumCivsAheadTourism - iNumCivsBehindTourism) / iNumCivsAlive);
	}

	// for every civ we are Influential over increase this
	int iNumInfluential = m_pPlayer->GetCulture()->GetNumCivsInfluentialOn();
	iPriority += iNumInfluential * GC.getAI_GS_CULTURE_INFLUENTIAL_CIV_MOD();

	return iPriority;
}

/// Returns Priority for United Nations Grand Strategy
int CvGrandStrategyAI::GetUnitedNationsPriority()
{
	int iPriority = 0;
	PlayerTypes ePlayer = m_pPlayer->GetID();

	// If UN Victory isn't even available then don't bother with anything
	VictoryTypes eVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_DIPLOMATIC", true);
	if(eVictory == NO_VICTORY || !GC.getGame().isVictoryValid(eVictory))
	{
		return -100;
	}

	int iNumMinorsAttacked = GET_TEAM(GetPlayer()->getTeam()).GetNumMinorCivsAttacked();
	iPriority += (iNumMinorsAttacked* /*-30*/ GC.getAI_GRAND_STRATEGY_UN_EACH_MINOR_ATTACKED_WEIGHT());

	int iVotesNeededToWin = GC.getGame().GetVotesNeededForDiploVictory();

	int iVotesControlled = 0;
	int iVotesControlledDelta = 0;
	int iUnalliedCityStates = 0;
	if (GC.getGame().GetGameLeagues()->GetNumActiveLeagues() == 0)
	{
		// Before leagues kick in, add weight based on flavor
		int iFlavorDiplo =  m_pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_DIPLOMACY"));
		iPriority += (10 - m_pPlayer->GetCurrentEra()) * iFlavorDiplo * 150 / 100;
	}
	else
	{
		CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
		CvAssert(pLeague != NULL);
		if (pLeague != NULL)
		{
			// Votes we control
			iVotesControlled += pLeague->CalculateStartingVotesForMember(ePlayer);

			// Votes other players control
			int iHighestOtherPlayerVotes = 0;
			for (int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
			{
				PlayerTypes eLoopPlayer = (PlayerTypes) iPlayerLoop;

				if(eLoopPlayer != ePlayer && GET_PLAYER(eLoopPlayer).isAlive())
				{
					if (GET_PLAYER(eLoopPlayer).isMinorCiv())
					{
						if (GET_PLAYER(eLoopPlayer).GetMinorCivAI()->GetAlly() == NO_PLAYER)
						{
							iUnalliedCityStates++;
						}
					}
					else
					{
						int iOtherPlayerVotes = pLeague->CalculateStartingVotesForMember(eLoopPlayer);
						if (iOtherPlayerVotes > iHighestOtherPlayerVotes)
						{
							iHighestOtherPlayerVotes = iOtherPlayerVotes;
						}
					}
				}
			}

			// How we compare
			iVotesControlledDelta = iVotesControlled - iHighestOtherPlayerVotes;
		}
	}

	// Are we close to winning?
	if (iVotesControlled >= iVotesNeededToWin)
	{
		return 1000;
	}
	else if (iVotesControlled >= ((iVotesNeededToWin * 3) / 4))
	{
		iPriority += 40;
	}

	// We have the most votes
	if (iVotesControlledDelta > 0)
	{
		iPriority += MAX(40, iVotesControlledDelta * 5);
	}
	// We are equal or behind in votes
	else
	{
		// Could we make up the difference with currently unallied city-states?
		int iPotentialCityStateVotes = iUnalliedCityStates * 2;
		int iPotentialVotesDelta = iPotentialCityStateVotes + iVotesControlledDelta;
		if (iPotentialVotesDelta > 0)
		{
			iPriority += MAX(20, iPotentialVotesDelta * 5);
		}
		else if (iPotentialVotesDelta < 0)
		{
			iPriority += MIN(-40, iPotentialVotesDelta * -5);
		}
	}

	// factor in some traits that could be useful (or harmful)
	iPriority += m_pPlayer->GetPlayerTraits()->GetCityStateFriendshipModifier();
	iPriority += m_pPlayer->GetPlayerTraits()->GetCityStateBonusModifier();
	iPriority -= m_pPlayer->GetPlayerTraits()->GetCityStateCombatModifier();

	return iPriority;
}

/// Returns Priority for Spaceship Grand Strategy
int CvGrandStrategyAI::GetSpaceshipPriority()
{
	int iPriority = 0;

	// If SS Victory isn't even available then don't bother with anything
	VictoryTypes eVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_SPACE_RACE", true);
	if(eVictory == NO_VICTORY || !GC.getGame().isVictoryValid(eVictory))
	{
		return -100;
	}

	int iFlavorScience =  m_pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_SCIENCE"));

	// the later the game the greater the chance
#if defined(MOD_AI_SMART_V3)
	if (MOD_AI_SMART_V3)
		iPriority += (4 + m_pPlayer->GetCurrentEra()) * iFlavorScience * 150 / 100;
	else
#endif
		iPriority += m_pPlayer->GetCurrentEra() * iFlavorScience * 150 / 100;

	// if I already built the Apollo Program I am very likely to follow through
	ProjectTypes eApolloProgram = (ProjectTypes) GC.getInfoTypeForString("PROJECT_APOLLO_PROGRAM", true);
	if(eApolloProgram != NO_PROJECT)
	{
		if(GET_TEAM(m_pPlayer->getTeam()).getProjectCount(eApolloProgram) > 0)
		{
#if defined(MOD_AI_SMART_V3)
			if (MOD_AI_SMART_V3)
				iPriority += /*75*/ (GC.getAI_GS_SS_HAS_APOLLO_PROGRAM() / 2);
			else
#endif
				iPriority += /*150*/ GC.getAI_GS_SS_HAS_APOLLO_PROGRAM();
		}
	}

	return iPriority;
}

/// Get the base Priority for a Grand Strategy; these are elements common to ALL Grand Strategies
int CvGrandStrategyAI::GetBaseGrandStrategyPriority(AIGrandStrategyTypes eGrandStrategy)
{
	CvAIGrandStrategyXMLEntry* pGrandStrategy = GetAIGrandStrategies()->GetEntry(eGrandStrategy);

	int iPriority = 0;

	// Personality effect on Priority
	for(int iFlavorLoop = 0; iFlavorLoop < GC.getNumFlavorTypes(); iFlavorLoop++)
	{
		if(pGrandStrategy->GetFlavorValue(iFlavorLoop) != 0)
		{
			iPriority += (pGrandStrategy->GetFlavorValue(iFlavorLoop) * GetPlayer()->GetFlavorManager()->GetPersonalityIndividualFlavor((FlavorTypes) iFlavorLoop));
		}
	}

	return iPriority;
}

/// Get the base Priority for a Grand Strategy; these are elements common to ALL Grand Strategies
#if defined(MOD_AI_SMART_V3)
int CvGrandStrategyAI::GetPersonalityAndGrandStrategy(FlavorTypes eFlavorType, bool bBoostGSMainFlavor)
#else
int CvGrandStrategyAI::GetPersonalityAndGrandStrategy(FlavorTypes eFlavorType)
#endif
{
	if(m_eActiveGrandStrategy != NO_AIGRANDSTRATEGY)
	{
		CvAIGrandStrategyXMLEntry* pGrandStrategy = GetAIGrandStrategies()->GetEntry(m_eActiveGrandStrategy);
		int iModdedFlavor = pGrandStrategy->GetFlavorModValue(eFlavorType) + m_pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor(eFlavorType);
		iModdedFlavor = max(0,iModdedFlavor);
#if defined(MOD_AI_SMART_V3)
		if(MOD_AI_SMART_V3 && bBoostGSMainFlavor && (pGrandStrategy->GetFlavorValue(eFlavorType) > 0))
		{
			iModdedFlavor = min(10, ((pGrandStrategy->GetFlavorValue(eFlavorType) + iModdedFlavor + 1) / 2));
		}
#endif
		return iModdedFlavor;
	}
	return m_pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor(eFlavorType);
}

/// Returns the Active Grand Strategy for this Player: how am I trying to win right now?
AIGrandStrategyTypes CvGrandStrategyAI::GetActiveGrandStrategy() const
{
	return m_eActiveGrandStrategy;
}

/// Sets the Active Grand Strategy for this Player: how am I trying to win right now?
void CvGrandStrategyAI::SetActiveGrandStrategy(AIGrandStrategyTypes eGrandStrategy)
{
	if(eGrandStrategy != NO_AIGRANDSTRATEGY)
	{
		m_eActiveGrandStrategy = eGrandStrategy;

		SetNumTurnsSinceActiveSet(0);
	}
}

/// The number of turns since the Active Strategy was last set
int CvGrandStrategyAI::GetNumTurnsSinceActiveSet() const
{
	return m_iNumTurnsSinceActiveSet;
}

/// Set the number of turns since the Active Strategy was last set
void CvGrandStrategyAI::SetNumTurnsSinceActiveSet(int iValue)
{
	m_iNumTurnsSinceActiveSet = iValue;
	FAssert(m_iNumTurnsSinceActiveSet >= 0);
}

/// Change the number of turns since the Active Strategy was last set
void CvGrandStrategyAI::ChangeNumTurnsSinceActiveSet(int iChange)
{
	if(iChange != 0)
	{
		m_iNumTurnsSinceActiveSet += iChange;
	}

	FAssert(m_iNumTurnsSinceActiveSet >= 0);
}

/// Returns the Priority Level the player has for a particular Grand Strategy
int CvGrandStrategyAI::GetGrandStrategyPriority(AIGrandStrategyTypes eGrandStrategy) const
{
	FAssert(eGrandStrategy != NO_AIGRANDSTRATEGY);
	return m_paiGrandStrategyPriority[eGrandStrategy];
}

/// Sets the Priority Level the player has for a particular Grand Strategy
void CvGrandStrategyAI::SetGrandStrategyPriority(AIGrandStrategyTypes eGrandStrategy, int iValue)
{
	FAssert(eGrandStrategy != NO_AIGRANDSTRATEGY);
	m_paiGrandStrategyPriority[eGrandStrategy] = iValue;
}

/// Changes the Priority Level the player has for a particular Grand Strategy
void CvGrandStrategyAI::ChangeGrandStrategyPriority(AIGrandStrategyTypes eGrandStrategy, int iChange)
{
	FAssert(eGrandStrategy != NO_AIGRANDSTRATEGY);

	if(iChange != 0)
	{
		m_paiGrandStrategyPriority[eGrandStrategy] += iChange;
	}
}



// **********
// Stuff relating to guessing what other Players are up to
// **********



/// Runs every turn to try and figure out what other known Players' Grand Strategies are
void CvGrandStrategyAI::DoGuessOtherPlayersActiveGrandStrategy()
{
	CvWeightedVector<int, 5, true> vGrandStrategyPriorities;
	FStaticVector< int, 5, true, c_eCiv5GameplayDLL >  vGrandStrategyPrioritiesForLogging;

	GuessConfidenceTypes eGuessConfidence = NO_GUESS_CONFIDENCE_TYPE;

	int iGrandStrategiesLoop = 0;
	AIGrandStrategyTypes eGrandStrategy = NO_AIGRANDSTRATEGY;
	CvAIGrandStrategyXMLEntry* pGrandStrategy = 0;
	CvString strGrandStrategyName;

	CvTeam& pTeam = GET_TEAM(GetPlayer()->getTeam());

	int iMajorLoop = 0;
	PlayerTypes eMajor = NO_PLAYER;

	int iPriority = 0;

	// Establish world Military strength average
	int iWorldMilitaryAverage = GC.getGame().GetWorldMilitaryStrengthAverage(GetPlayer()->GetID(), true, true);

	// Establish world culture and tourism averages
	int iNumPlayersAlive = 0;
	int iWorldCultureAverage = 0;
	int iWorldTourismAverage = 0;
	for(iMajorLoop = 0; iMajorLoop < MAX_MAJOR_CIVS; iMajorLoop++)
	{
		eMajor = (PlayerTypes) iMajorLoop;

		if(GET_PLAYER(eMajor).isAlive())
		{
			iWorldCultureAverage += GET_PLAYER(eMajor).GetJONSCultureEverGenerated();
			iWorldTourismAverage += GET_PLAYER(eMajor).GetCulture()->GetTourism();
			iNumPlayersAlive++;
		}
	}
	iWorldCultureAverage /= iNumPlayersAlive;
	iWorldTourismAverage /= iNumPlayersAlive;

	// Establish world Tech progress average
	iNumPlayersAlive = 0;
	int iWorldNumTechsAverage = 0;
	TeamTypes eTeam;
	for(int iTeamLoop = 0; iTeamLoop < MAX_MAJOR_CIVS; iTeamLoop++)	// Looping over all MAJOR teams
	{
		eTeam = (TeamTypes) iTeamLoop;

		if(GET_TEAM(eTeam).isAlive())
		{
			iWorldNumTechsAverage += GET_TEAM(eTeam).GetTeamTechs()->GetNumTechsKnown();
			iNumPlayersAlive++;
		}
	}
	iWorldNumTechsAverage /= iNumPlayersAlive;

	// Look at every Major we've met
	for(iMajorLoop = 0; iMajorLoop < MAX_MAJOR_CIVS; iMajorLoop++)
	{
		eMajor = (PlayerTypes) iMajorLoop;

		if(GET_PLAYER(eMajor).isAlive() && iMajorLoop != GetPlayer()->GetID())
		{
			if(pTeam.isHasMet(GET_PLAYER(eMajor).getTeam()))
			{
				for(iGrandStrategiesLoop = 0; iGrandStrategiesLoop < GetAIGrandStrategies()->GetNumAIGrandStrategies(); iGrandStrategiesLoop++)
				{
					eGrandStrategy = (AIGrandStrategyTypes) iGrandStrategiesLoop;
					pGrandStrategy = GetAIGrandStrategies()->GetEntry(iGrandStrategiesLoop);
					strGrandStrategyName = (CvString) pGrandStrategy->GetType();

					if(strGrandStrategyName == "AIGRANDSTRATEGY_CONQUEST")
					{
						iPriority = GetGuessOtherPlayerConquestPriority(eMajor, iWorldMilitaryAverage);
					}
					else if(strGrandStrategyName == "AIGRANDSTRATEGY_CULTURE")
					{
						iPriority = GetGuessOtherPlayerCulturePriority(eMajor, iWorldCultureAverage, iWorldTourismAverage);
					}
					else if(strGrandStrategyName == "AIGRANDSTRATEGY_UNITED_NATIONS")
					{
						iPriority = GetGuessOtherPlayerUnitedNationsPriority(eMajor);
					}
					else if(strGrandStrategyName == "AIGRANDSTRATEGY_SPACESHIP")
					{
						iPriority = GetGuessOtherPlayerSpaceshipPriority(eMajor, iWorldNumTechsAverage);
					}

					vGrandStrategyPriorities.push_back(iGrandStrategiesLoop, iPriority);
					vGrandStrategyPrioritiesForLogging.push_back(iPriority);
				}

				if(vGrandStrategyPriorities.size() > 0)
				{
					// Add "No Grand Strategy" in case we just don't have enough info to go on
					iPriority = /*40*/ GC.getAI_GRAND_STRATEGY_GUESS_NO_CLUE_WEIGHT();

					vGrandStrategyPriorities.push_back(NO_AIGRANDSTRATEGY, iPriority);
					vGrandStrategyPrioritiesForLogging.push_back(iPriority);

					vGrandStrategyPriorities.SortItems();

					eGrandStrategy = (AIGrandStrategyTypes) vGrandStrategyPriorities.GetElement(0);
					iPriority = vGrandStrategyPriorities.GetWeight(0);
					eGuessConfidence = NO_GUESS_CONFIDENCE_TYPE;

					// How confident are we in our Guess?
					if(eGrandStrategy != NO_AIGRANDSTRATEGY)
					{
						if(iPriority >= /*120*/ GC.getAI_GRAND_STRATEGY_GUESS_POSITIVE_THRESHOLD())
						{
							eGuessConfidence = GUESS_CONFIDENCE_POSITIVE;
						}
						else if(iPriority >= /*70*/ GC.getAI_GRAND_STRATEGY_GUESS_LIKELY_THRESHOLD())
						{
							eGuessConfidence = GUESS_CONFIDENCE_LIKELY;
						}
						else
						{
							eGuessConfidence = GUESS_CONFIDENCE_UNSURE;
						}
					}

					SetGuessOtherPlayerActiveGrandStrategy(eMajor, eGrandStrategy, eGuessConfidence);

					LogGuessOtherPlayerGrandStrategy(vGrandStrategyPrioritiesForLogging, eMajor);
				}

				vGrandStrategyPriorities.clear();
				vGrandStrategyPrioritiesForLogging.clear();
			}
		}
	}
}

/// What does this AI BELIEVE another player's Active Grand Strategy to be?
AIGrandStrategyTypes CvGrandStrategyAI::GetGuessOtherPlayerActiveGrandStrategy(PlayerTypes ePlayer) const
{
	FAssert(ePlayer < MAX_MAJOR_CIVS);
	return (AIGrandStrategyTypes) m_eGuessOtherPlayerActiveGrandStrategy[ePlayer];
}

/// How confident is the AI in its guess of what another player's Active Grand Strategy is?
GuessConfidenceTypes CvGrandStrategyAI::GetGuessOtherPlayerActiveGrandStrategyConfidence(PlayerTypes ePlayer) const
{
	FAssert(ePlayer < MAX_MAJOR_CIVS);
	return (GuessConfidenceTypes) m_eGuessOtherPlayerActiveGrandStrategyConfidence[ePlayer];
}

/// Sets what this AI BELIEVES another player's Active Grand Strategy to be
void CvGrandStrategyAI::SetGuessOtherPlayerActiveGrandStrategy(PlayerTypes ePlayer, AIGrandStrategyTypes eGrandStrategy, GuessConfidenceTypes eGuessConfidence)
{
	FAssert(ePlayer < MAX_MAJOR_CIVS);
	m_eGuessOtherPlayerActiveGrandStrategy[ePlayer] = eGrandStrategy;
	m_eGuessOtherPlayerActiveGrandStrategyConfidence[ePlayer] = eGuessConfidence;
}

/// Guess as to how much another Player is prioritizing Conquest as his means of winning the game
int CvGrandStrategyAI::GetGuessOtherPlayerConquestPriority(PlayerTypes ePlayer, int iWorldMilitaryAverage)
{
	int iConquestPriority = 0;

	// Compare their Military to the world average; Possible range is 100 to -100 (but will typically be around -20 to 20)
	if(iWorldMilitaryAverage > 0)
	{
		iConquestPriority += (GET_PLAYER(ePlayer).GetMilitaryMight() - iWorldMilitaryAverage) * /*100*/ GC.getAI_GRAND_STRATEGY_CONQUEST_POWER_RATIO_MULTIPLIER() / iWorldMilitaryAverage;
	}

	// Minors attacked
	iConquestPriority += (GetPlayer()->GetDiplomacyAI()->GetOtherPlayerNumMinorsAttacked(ePlayer) * /*5*/ GC.getAI_GRAND_STRATEGY_CONQUEST_WEIGHT_PER_MINOR_ATTACKED());

	// Minors Conquered
	iConquestPriority += (GetPlayer()->GetDiplomacyAI()->GetOtherPlayerNumMinorsConquered(ePlayer) * /*10*/ GC.getAI_GRAND_STRATEGY_CONQUEST_WEIGHT_PER_MINOR_CONQUERED());

	// Majors attacked
	iConquestPriority += (GetPlayer()->GetDiplomacyAI()->GetOtherPlayerNumMajorsAttacked(ePlayer) * /*10*/ GC.getAI_GRAND_STRATEGY_CONQUEST_WEIGHT_PER_MAJOR_ATTACKED());

	// Majors Conquered
	iConquestPriority += (GetPlayer()->GetDiplomacyAI()->GetOtherPlayerNumMajorsConquered(ePlayer) * /*15*/ GC.getAI_GRAND_STRATEGY_CONQUEST_WEIGHT_PER_MAJOR_CONQUERED());

	return iConquestPriority;
}

/// Guess as to how much another Player is prioritizing Culture as his means of winning the game
int CvGrandStrategyAI::GetGuessOtherPlayerCulturePriority(PlayerTypes ePlayer, int iWorldCultureAverage, int iWorldTourismAverage)
{
	VictoryTypes eVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_CULTURAL", true);

	// If Culture Victory isn't even available then don't bother with anything
	if(eVictory == NO_VICTORY)
	{
		return -100;
	}

	int iCulturePriority = 0;
	int iRatio;

	// Compare their Culture to the world average; Possible range is 75 to -75
	if(iWorldCultureAverage > 0)
	{
		iRatio = (GET_PLAYER(ePlayer).GetJONSCultureEverGenerated() - iWorldCultureAverage) * /*75*/ GC.getAI_GS_CULTURE_RATIO_MULTIPLIER() / iWorldCultureAverage;
		if (iRatio > GC.getAI_GS_CULTURE_RATIO_MULTIPLIER())
		{
			iCulturePriority += GC.getAI_GS_CULTURE_RATIO_MULTIPLIER();
		}
		else if (iRatio < -GC.getAI_GS_CULTURE_RATIO_MULTIPLIER())
		{
			iCulturePriority += -GC.getAI_GS_CULTURE_RATIO_MULTIPLIER();
		}
		iCulturePriority += iRatio;
	}

	// Compare their Tourism to the world average; Possible range is 75 to -75
	if(iWorldTourismAverage > 0)
	{
		iRatio = (GET_PLAYER(ePlayer).GetCulture()->GetTourism() - iWorldTourismAverage) * /*75*/ GC.getAI_GS_TOURISM_RATIO_MULTIPLIER() / iWorldTourismAverage;
		if (iRatio > GC.getAI_GS_TOURISM_RATIO_MULTIPLIER())
		{
			iCulturePriority += GC.getAI_GS_TOURISM_RATIO_MULTIPLIER();
		}
		else if (iRatio < -GC.getAI_GS_TOURISM_RATIO_MULTIPLIER())
		{
			iCulturePriority += -GC.getAI_GS_TOURISM_RATIO_MULTIPLIER();
		}
		iCulturePriority += iRatio;	}

	return iCulturePriority;
}

/// Guess as to how much another Player is prioritizing the UN as his means of winning the game
int CvGrandStrategyAI::GetGuessOtherPlayerUnitedNationsPriority(PlayerTypes ePlayer)
{
	VictoryTypes eVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_DIPLOMATIC", true);

	// If UN Victory isn't even available then don't bother with anything
	if(eVictory == NO_VICTORY)
	{
		return -100;
	}

	int iTheirCityStateAllies = 0;
	int iTheirCityStateFriends = 0;
	int iCityStatesAlive = 0;
	for(int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
	{
		PlayerTypes eLoopPlayer = (PlayerTypes) iPlayerLoop;

		if (eLoopPlayer != ePlayer && GET_PLAYER(eLoopPlayer).isAlive() && GET_PLAYER(eLoopPlayer).isMinorCiv())
		{
			iCityStatesAlive++;
			if (GET_PLAYER(eLoopPlayer).GetMinorCivAI()->IsAllies(ePlayer))
			{
				iTheirCityStateAllies++;
			}
			else if (GET_PLAYER(eLoopPlayer).GetMinorCivAI()->IsFriends(ePlayer))
			{
				iTheirCityStateFriends++;
			}
		}
	}
	iCityStatesAlive = MAX(iCityStatesAlive, 1);

	int iPriority = iTheirCityStateAllies + (iTheirCityStateFriends / 3);
	iPriority = iPriority * GC.getAI_GS_UN_SECURED_VOTE_MOD();
	iPriority = iPriority / iCityStatesAlive;

	return iPriority;
}

/// Guess as to how much another Player is prioritizing the SS as his means of winning the game
int CvGrandStrategyAI::GetGuessOtherPlayerSpaceshipPriority(PlayerTypes ePlayer, int iWorldNumTechsAverage)
{
	VictoryTypes eVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_SPACE_RACE", true);

	// If SS Victory isn't even available then don't bother with anything
	if(eVictory == NO_VICTORY)
	{
		return -100;
	}

	TeamTypes eTeam = GET_PLAYER(ePlayer).getTeam();

	// If the player has the Apollo Program we're pretty sure he's going for the SS
	ProjectTypes eApolloProgram = (ProjectTypes) GC.getInfoTypeForString("PROJECT_APOLLO_PROGRAM", true);
	if(eApolloProgram != NO_PROJECT)
	{
		if(GET_TEAM(eTeam).getProjectCount(eApolloProgram) > 0)
		{
			return /*150*/ GC.getAI_GS_SS_HAS_APOLLO_PROGRAM();
		}
	}

	int iNumTechs = GET_TEAM(eTeam).GetTeamTechs()->GetNumTechsKnown();

	// Don't divide by zero, okay?
	if(iWorldNumTechsAverage == 0)
		iWorldNumTechsAverage = 1;

	int iSSPriority = (iNumTechs - iWorldNumTechsAverage) * /*300*/ GC.getAI_GS_SS_TECH_PROGRESS_MOD() / iWorldNumTechsAverage;

	return iSSPriority;
}


// PRIVATE METHODS

/// Log GrandStrategy state: what are the Priorities and who is Active?
void CvGrandStrategyAI::LogGrandStrategies(const FStaticVector< int, 5, true, c_eCiv5GameplayDLL >& vModifiedGrandStrategyPriorities)
{
	if(GC.getLogging() && GC.getAILogging())
	{
		CvString strOutBuf;
		CvString strBaseString;
		CvString strTemp;
		CvString playerName;
		CvString strDesc;
		CvString strLogName;

		// Find the name of this civ and city
		playerName = GetPlayer()->getCivilizationShortDescription();

		// Open the log file
		if(GC.getPlayerAndCityAILogSplit())
		{
			strLogName = "GrandStrategyAI_Log_" + playerName + ".csv";
		}
		else
		{
			strLogName = "GrandStrategyAI_Log.csv";
		}

		FILogFile* pLog;
		pLog = LOGFILEMGR.GetLog(strLogName, FILogFile::kDontTimeStamp);

		AIGrandStrategyTypes eGrandStrategy;

		// Loop through Grand Strategies
		for(int iGrandStrategyLoop = 0; iGrandStrategyLoop < GC.getNumAIGrandStrategyInfos(); iGrandStrategyLoop++)
		{
			// Get the leading info for this line
			strBaseString.Format("%03d, ", GC.getGame().getElapsedGameTurns());
			strBaseString += playerName + ", ";

			eGrandStrategy = (AIGrandStrategyTypes) iGrandStrategyLoop;

			// GrandStrategy Info
			CvAIGrandStrategyXMLEntry* pEntry = GC.getAIGrandStrategyInfo(eGrandStrategy);
			const char* szAIGrandStrategyType = (pEntry != NULL)? pEntry->GetType() : "Unknown Type";

			if(GetActiveGrandStrategy() == eGrandStrategy)
			{
				strTemp.Format("*** %s, %d, %d", szAIGrandStrategyType, GetGrandStrategyPriority(eGrandStrategy), vModifiedGrandStrategyPriorities[eGrandStrategy]);
			}
			else
			{
				strTemp.Format("%s, %d, %d", szAIGrandStrategyType, GetGrandStrategyPriority(eGrandStrategy), vModifiedGrandStrategyPriorities[eGrandStrategy]);
			}
			strOutBuf = strBaseString + strTemp;
			pLog->Msg(strOutBuf);
		}
	}
}

/// Log our guess as to other Players' Active Grand Strategy
void CvGrandStrategyAI::LogGuessOtherPlayerGrandStrategy(const FStaticVector< int, 5, true, c_eCiv5GameplayDLL >& vGrandStrategyPriorities, PlayerTypes ePlayer)
{
	if(GC.getLogging() && GC.getAILogging())
	{
		CvString strOutBuf;
		CvString strBaseString;
		CvString strTemp;
		CvString playerName;
		CvString otherPlayerName;
		CvString strDesc;
		CvString strLogName;

		// Find the name of this civ and city
		playerName = GetPlayer()->getCivilizationShortDescription();

		// Open the log file
		if(GC.getPlayerAndCityAILogSplit())
		{
			strLogName = "GrandStrategyAI_Guess_Log_" + playerName + ".csv";
		}
		else
		{
			strLogName = "GrandStrategyAI_Guess_Log.csv";
		}

		FILogFile* pLog;
		pLog = LOGFILEMGR.GetLog(strLogName, FILogFile::kDontTimeStamp);

		AIGrandStrategyTypes eGrandStrategy;
		int iPriority;

		// Loop through Grand Strategies
		for(int iGrandStrategyLoop = 0; iGrandStrategyLoop < GC.getNumAIGrandStrategyInfos(); iGrandStrategyLoop++)
		{
			// Get the leading info for this line
			strBaseString.Format("%03d, ", GC.getGame().getElapsedGameTurns());
			strBaseString += playerName + ", ";

			eGrandStrategy = (AIGrandStrategyTypes) iGrandStrategyLoop;
			iPriority = vGrandStrategyPriorities[iGrandStrategyLoop];

			CvAIGrandStrategyXMLEntry* pEntry = GC.getAIGrandStrategyInfo(eGrandStrategy);
			const char* szGrandStrategyType = (pEntry != NULL)? pEntry->GetType() : "Unknown Strategy";

			// GrandStrategy Info
			if(GetActiveGrandStrategy() == eGrandStrategy)
			{
				strTemp.Format("*** %s, %d", szGrandStrategyType, iPriority);
			}
			else
			{
				strTemp.Format("%s, %d", szGrandStrategyType, iPriority);
			}
			otherPlayerName = GET_PLAYER(ePlayer).getCivilizationShortDescription();
			strOutBuf = strBaseString + otherPlayerName + ", " + strTemp;

			if(GetGuessOtherPlayerActiveGrandStrategy(ePlayer) == eGrandStrategy)
			{
				// Confidence in our guess
				switch(GetGuessOtherPlayerActiveGrandStrategyConfidence(ePlayer))
				{
				case GUESS_CONFIDENCE_POSITIVE:
					strTemp.Format("Positive");
					break;
				case GUESS_CONFIDENCE_LIKELY:
					strTemp.Format("Likely");
					break;
				case GUESS_CONFIDENCE_UNSURE:
					strTemp.Format("Unsure");
					break;
				default:
					strTemp.Format("XXX");
					break;
				}

				strOutBuf += ", " + strTemp;
			}

			pLog->Msg(strOutBuf);
		}

		// One more entry for NO GRAND STRATEGY
		// Get the leading info for this line
		strBaseString.Format("%03d, ", GC.getGame().getElapsedGameTurns());
		strBaseString += playerName + ", ";

		iPriority = vGrandStrategyPriorities[GC.getNumAIGrandStrategyInfos()];

		// GrandStrategy Info
		strTemp.Format("NO_GRAND_STRATEGY, %d", iPriority);
		otherPlayerName = GET_PLAYER(ePlayer).getCivilizationShortDescription();
		strOutBuf = strBaseString + otherPlayerName + ", " + strTemp;
		pLog->Msg(strOutBuf);
	}
}