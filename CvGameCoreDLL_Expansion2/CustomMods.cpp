// CustomMods.cpp
#include "CvGameCoreDLLPCH.h"

// must be included after all other headers
#include "LintFree.h"

CustomMods gCustomMods;

CustomMods::CustomMods() :
	m_bInit(false)
{
}


// I would rather have all of this shit here (where it need never be read more than once) to make the calling of events cleaner in the main code
int CustomMods::eventHook(const char* szName, const char* p, ...) {
	CvLuaArgsHandle args;

	va_list vl;
	va_start(vl, p);

	for (const char* it = p; *it; ++it) {
		if (*it == 'b') {
			// It's a boolean
			args->Push(va_arg(vl, bool));
		} else if (*it == 'i') {
			// It's an int
			args->Push(va_arg(vl, int));
		} else {
			// Assume it's a string (char *)
			char* s = va_arg(vl, char*);
			args->Push(s, strlen(s));
			break;
		}
	}

	va_end(vl);

	return eventHook(szName, args);
}

int CustomMods::eventTestAll(const char* szName, const char* p, ...) {
	CvLuaArgsHandle args;

	va_list vl;
	va_start(vl, p);

	for (const char* it = p; *it; ++it) {
		if (*it == 'b') {
			// It's a boolean
			args->Push(va_arg(vl, bool));
		} else if (*it == 'i') {
			// It's an int
			args->Push(va_arg(vl, int));
		} else {
			// Assume it's a string (char *)
			char* s = va_arg(vl, char*);
			args->Push(s, strlen(s));
			break;
		}
	}

	va_end(vl);

	return eventTestAll(szName, args);
}

int CustomMods::eventTestAny(const char* szName, const char* p, ...) {
	CvLuaArgsHandle args;

	va_list vl;
	va_start(vl, p);

	for (const char* it = p; *it; ++it) {
		if (*it == 'b') {
			// It's a boolean
			args->Push(va_arg(vl, bool));
		} else if (*it == 'i') {
			// It's an int
			args->Push(va_arg(vl, int));
		} else {
			// Assume it's a string (char *)
			char* s = va_arg(vl, char*);
			args->Push(s, strlen(s));
			break;
		}
	}

	va_end(vl);

	return eventTestAny(szName, args);
}

int CustomMods::eventAccumulator(int &iValue, const char* szName, const char* p, ...) {
	CvLuaArgsHandle args;

	va_list vl;
	va_start(vl, p);

	for (const char* it = p; *it; ++it) {
		if (*it == 'b') {
			// It's a boolean
			args->Push(va_arg(vl, bool));
		} else if (*it == 'i') {
			// It's an int
			args->Push(va_arg(vl, int));
		} else {
			// Assume it's a string (char *)
			char* s = va_arg(vl, char*);
			args->Push(s, strlen(s));
			break;
		}
	}

	va_end(vl);

	return eventAccumulator(iValue, szName, args);
}

int CustomMods::eventHook(const char* szName, CvLuaArgsHandle &args) {
	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if (pkScriptSystem) {
		bool bResult;
		if (LuaSupport::CallHook(pkScriptSystem, szName, args.get(), bResult)) {
			return GAMEEVENTRETURN_HOOK;
		}
	}

	return GAMEEVENTRETURN_NONE;
}

int CustomMods::eventTestAll(const char* szName, CvLuaArgsHandle &args) {
	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if (pkScriptSystem) {
		bool bResult = false;
		if (LuaSupport::CallTestAll(pkScriptSystem, szName, args.get(), bResult)) {
			if (bResult) {
				return GAMEEVENTRETURN_TRUE;
			} else {
				return GAMEEVENTRETURN_FALSE;
			}
		}
	}

	return GAMEEVENTRETURN_NONE;
}

int CustomMods::eventTestAny(const char* szName, CvLuaArgsHandle &args) {
	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if (pkScriptSystem) {
		bool bResult = false;
		if (LuaSupport::CallTestAny(pkScriptSystem, szName, args.get(), bResult)) {
			if (bResult) {
				return GAMEEVENTRETURN_TRUE;
			} else {
				return GAMEEVENTRETURN_FALSE;
			}
		}
	}

	return GAMEEVENTRETURN_NONE;
}

int CustomMods::eventAccumulator(int &iValue, const char* szName, CvLuaArgsHandle &args) {
	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if (pkScriptSystem) {
		if (LuaSupport::CallAccumulator(pkScriptSystem, szName, args.get(), iValue)) {
			return GAMEEVENTRETURN_TRUE;
		}
	}

	return GAMEEVENTRETURN_NONE;
}


// Update CustomModOptions table from references in CustomModPostDefines
// Based on code in CvDllDatabaseUtility::PerformDatabasePostProcessing()
void CustomMods::prefetchCache() {
	Database::Connection* db = GC.GetGameDatabase();
	db->BeginTransaction();

	Database::Results kInsert;
	db->Execute(kInsert, "INSERT OR REPLACE INTO CustomModOptions(Name, Value) VALUES(?, ?)");

	Database::Results kPostDefines;
	db->SelectAll(kPostDefines, "CustomModPostDefines");
	while (kPostDefines.Step()) {
		Database::Results kLookup;
		char szSQL[512];
		sprintf_s(szSQL, "select ROWID from %s where Type = '%s' LIMIT 1", kPostDefines.GetText("Table"), kPostDefines.GetText("Type"));

		if (db->Execute(kLookup, szSQL)) {
			if (kLookup.Step()) {
				kInsert.Bind(1, kPostDefines.GetText("Name"));
				kInsert.Bind(2, kLookup.GetInt(0));
				kInsert.Step();
				kInsert.Reset();
			}
		}
	}

	db->EndTransaction();
}

void CustomMods::preloadCache() {
	prefetchCache();

	(void) getOption("EVENTS_CIRCUMNAVIGATION");
}

void CustomMods::reloadCache() {
	m_bInit = false;

	preloadCache();
}

int CustomMods::getOption(const char* szOption, int defValue) {
	return getOption(string(szOption), defValue);
}

int CustomMods::getOption(string sOption, int defValue) {
	if (!m_bInit) {
		const char* szBadPrefix = "MOD_";

		Database::Results kResults;
		DB.SelectAll(kResults, MOD_DB_TABLE);

		while (kResults.Step()) {
			const char* szName = kResults.GetText(MOD_DB_COL_NAME);
			const int iDbUpdates = kResults.GetInt(MOD_DB_COL_DBUPDATES);
			int iValue = kResults.GetInt(MOD_DB_COL_VALUE);

			bool bPrefixError = (strncmp(szName, szBadPrefix, strlen(szBadPrefix)) == 0);

			if (iValue && iDbUpdates) {
				Database::Results kUpdates;
				char szQuery[512] = {0};

				// Did the required mods to the database occur?  We'll assume they didn't, unless proven otherwise!
				bool bOK = false;

				sprintf_s(szQuery, "Name='%s' AND Value=1", szName);
				if (DB.SelectWhere(kUpdates, MOD_DB_UPDATES, szQuery)) {
					if (kUpdates.Step()) {
						// BINGO!  We have our updates
						bOK = true;
					} else {
						// All is not lost as there could be BOTH xml and sql updates
						sprintf_s(szQuery, "Name='%s_SQL' AND Value=1", szName);
						if (DB.SelectWhere(kUpdates, MOD_DB_UPDATES, szQuery)) {
							if (kUpdates.Step()) {
								sprintf_s(szQuery, "Name='%s_XML' AND Value=1", szName);
								if (DB.SelectWhere(kUpdates, MOD_DB_UPDATES, szQuery)) {
									if (kUpdates.Step()) {
										// BINGO!  We have BOTH our updates
										bOK = true;
									}
								}
							}
						}
					}
				}

				if (bOK) {
					CUSTOMLOG("%s: %s appears to have the required database updates", (bPrefixError ? "PREFIX ERROR" : "Cache"), szName);
				} else {
					CUSTOMLOG("%s: %s has missing database updates!", (bPrefixError ? "PREFIX ERROR" : "Cache"), szName);
					iValue = 0;
				}
			}

			CUSTOMLOG("%s: %s = %d", (bPrefixError ? "PREFIX ERROR" : "Cache"), szName, iValue);
			m_options[string(szName)] = iValue;
		}

		MOD_OPT_CACHE(GLOBAL_INTERNAL_TRADE_ROUTE_BONUS_FROM_ORIGIN_CITY);
		MOD_OPT_CACHE(GLOBAL_EARLY_COOP_WAR_LOCK);
		MOD_OPT_CACHE(GLOBAL_STACKING_RULES);
		MOD_OPT_CACHE(GLOBAL_BREAK_CIVILIAN_1UPT);
		MOD_OPT_CACHE(GLOBAL_BREAK_CIVILIAN_RESTRICTIONS);
		MOD_OPT_CACHE(GLOBAL_LOCAL_GENERALS);
		MOD_OPT_CACHE(GLOBAL_SEPARATE_GREAT_ADMIRAL);
		MOD_OPT_CACHE(GLOBAL_PROMOTION_CLASSES);
		MOD_OPT_CACHE(GLOBAL_PASSABLE_FORTS);
		MOD_OPT_CACHE(GLOBAL_PASSABLE_FORTS_ANY);
		MOD_OPT_CACHE(GLOBAL_ANYTIME_GOODY_GOLD);
		MOD_OPT_CACHE(GLOBAL_CITY_FOREST_BONUS);
		MOD_OPT_CACHE(GLOBAL_CITY_JUNGLE_BONUS);
		MOD_OPT_CACHE(GLOBAL_CITY_WORKING);
		MOD_OPT_CACHE(GLOBAL_CITY_AUTOMATON_WORKERS);
		MOD_OPT_CACHE(GLOBAL_RELOCATION);
		MOD_OPT_CACHE(GLOBAL_ALPINE_PASSES);
		MOD_OPT_CACHE(GLOBAL_CS_GIFT_SHIPS);
		MOD_OPT_CACHE(GLOBAL_CS_UPGRADES);
		MOD_OPT_CACHE(GLOBAL_CS_RAZE_RARELY);
		MOD_OPT_CACHE(GLOBAL_CS_LIBERATE_AFTER_BUYOUT);
		MOD_OPT_CACHE(GLOBAL_CS_GIFTS);
		MOD_OPT_CACHE(GLOBAL_CS_GIFTS_LOCAL_XP);
		MOD_OPT_CACHE(GLOBAL_CS_OVERSEAS_TERRITORY);
		MOD_OPT_CACHE(GLOBAL_CS_NO_ALLIED_SKIRMISHES);
		MOD_OPT_CACHE(GLOBAL_VENICE_KEEPS_RESOURCES);
		MOD_OPT_CACHE(GLOBAL_CS_MARRIAGE_KEEPS_RESOURCES);
		MOD_OPT_CACHE(GLOBAL_NO_FOLLOWUP);
		MOD_OPT_CACHE(GLOBAL_NO_FOLLOWUP_FROM_CITIES);
		MOD_OPT_CACHE(GLOBAL_CAPTURE_AFTER_ATTACKING);
		MOD_OPT_CACHE(GLOBAL_NO_OCEAN_PLUNDERING);
		MOD_OPT_CACHE(GLOBAL_NO_CONQUERED_SPACESHIPS);
		MOD_OPT_CACHE(GLOBAL_ADJACENT_BLOCKADES);
		MOD_OPT_CACHE(GLOBAL_ALLIES_BLOCK_BLOCKADES);
		MOD_OPT_CACHE(GLOBAL_SHORT_EMBARKED_BLOCKADES);
		MOD_OPT_CACHE(GLOBAL_GRATEFUL_SETTLERS);
		MOD_OPT_CACHE(GLOBAL_RELIGIOUS_SETTLERS);
		MOD_OPT_CACHE(GLOBAL_QUICK_ROUTES);
		MOD_OPT_CACHE(GLOBAL_SUBS_UNDER_ICE_IMMUNITY);
		MOD_OPT_CACHE(GLOBAL_PARATROOPS_MOVEMENT);
		MOD_OPT_CACHE(GLOBAL_PARATROOPS_AA_DAMAGE);
		MOD_OPT_CACHE(GLOBAL_NUKES_MELT_ICE); 
		MOD_OPT_CACHE(GLOBAL_GREATWORK_YIELDTYPES); 
		MOD_OPT_CACHE(GLOBAL_NO_LOST_GREATWORKS); 
		MOD_OPT_CACHE(GLOBAL_EXCLUDE_FROM_GIFTS);
		MOD_OPT_CACHE(GLOBAL_MOVE_AFTER_UPGRADE);
		MOD_OPT_CACHE(GLOBAL_CANNOT_EMBARK);
		MOD_OPT_CACHE(GLOBAL_SEPARATE_GP_COUNTERS);
		MOD_OPT_CACHE(GLOBAL_TRULY_FREE_GP);
		
		MOD_OPT_CACHE(DIPLOMACY_BY_NUMBERS);
		MOD_OPT_CACHE(DIPLOMACY_TECH_BONUSES);
		MOD_OPT_CACHE(DIPLOMACY_AUTO_DENOUNCE);
		MOD_OPT_CACHE(DIPLOMACY_STFU);
		MOD_OPT_CACHE(DIPLOMACY_NO_LEADERHEADS);
		
		MOD_OPT_CACHE(BUILDINGS_YIELD_FROM_OTHER_YIELD);
		MOD_OPT_CACHE(TRAITS_GG_FROM_BARBARIANS);
		MOD_OPT_CACHE(TRAITS_CROSSES_ICE);
		MOD_OPT_CACHE(TRAITS_CITY_WORKING);
		MOD_OPT_CACHE(TRAITS_CITY_AUTOMATON_WORKERS);
		MOD_OPT_CACHE(TRAITS_OTHER_PREREQS);
		MOD_OPT_CACHE(TRAITS_ANY_BELIEF);
		MOD_OPT_CACHE(TRAITS_TRADE_ROUTE_BONUSES);
		MOD_OPT_CACHE(TRAITS_EXTRA_SUPPLY);
		MOD_OPT_CACHE(TRAITS_CAN_FOUND_MOUNTAIN_CITY);
		MOD_OPT_CACHE(TRAITS_CAN_FOUND_COAST_CITY);

		MOD_OPT_CACHE(POLICIES_CITY_WORKING);
		MOD_OPT_CACHE(POLICIES_CITY_AUTOMATON_WORKERS);

		MOD_OPT_CACHE(TECHS_CITY_WORKING);
		MOD_OPT_CACHE(TECHS_CITY_AUTOMATON_WORKERS);

		MOD_OPT_CACHE(PROMOTIONS_AURA_CHANGE);
		MOD_OPT_CACHE(PROMOTIONS_GG_FROM_BARBARIANS);
		MOD_OPT_CACHE(PROMOTIONS_VARIABLE_RECON);
		MOD_OPT_CACHE(PROMOTIONS_CROSS_MOUNTAINS);
		MOD_OPT_CACHE(PROMOTIONS_CROSS_OCEANS);
		MOD_OPT_CACHE(PROMOTIONS_CROSS_ICE);
		MOD_OPT_CACHE(PROMOTIONS_HALF_MOVE);
		MOD_OPT_CACHE(PROMOTIONS_DEEP_WATER_EMBARKATION);
		MOD_OPT_CACHE(PROMOTIONS_FLAGSHIP);
		MOD_OPT_CACHE(PROMOTIONS_UNIT_NAMING);
		MOD_OPT_CACHE(PROMOTIONS_IMPROVEMENT_BONUS);
		MOD_OPT_CACHE(PROMOTIONS_ALLYCITYSTATE_BONUS);
		MOD_OPT_CACHE(DEFENSE_MOVES_BONUS);
		MOD_OPT_CACHE(PROMOTIONS_EXTRARES_BONUS);

		MOD_OPT_CACHE(UI_CITY_PRODUCTION);
		MOD_OPT_CACHE(UI_CITY_EXPANSION);

		MOD_OPT_CACHE(BUILDINGS_NW_EXCLUDE_RAZING);
		MOD_OPT_CACHE(BUILDINGS_PRO_RATA_PURCHASE);
		MOD_OPT_CACHE(BUILDINGS_CITY_WORKING);
		MOD_OPT_CACHE(BUILDINGS_CITY_AUTOMATON_WORKERS);
		MOD_OPT_CACHE(BUILDINGS_GOLDEN_AGE_EXTEND);

		MOD_OPT_CACHE(TRADE_ROUTE_SCALING);
		MOD_OPT_CACHE(TRADE_WONDER_RESOURCE_ROUTES);

		MOD_OPT_CACHE(UNITS_NO_SUPPLY);
		MOD_OPT_CACHE(UNITS_MAX_HP);
		MOD_OPT_CACHE(UNITS_XP_TIMES_100);
		MOD_OPT_CACHE(UNITS_LOCAL_WORKERS);
		MOD_OPT_CACHE(UNITS_HOVERING_LAND_ONLY_HEAL);
		MOD_OPT_CACHE(UNITS_HOVERING_COASTAL_ATTACKS);

		MOD_OPT_CACHE(RELIGION_NO_PREFERRENCES);
		MOD_OPT_CACHE(RELIGION_RANDOMISE);
		MOD_OPT_CACHE(RELIGION_CONVERSION_MODIFIERS);
		MOD_OPT_CACHE(RELIGION_KEEP_PROPHET_OVERFLOW);
		MOD_OPT_CACHE(RELIGION_ALLIED_INQUISITORS);
		MOD_OPT_CACHE(RELIGION_RECURRING_PURCHASE_NOTIFIY);
		MOD_OPT_CACHE(RELIGION_PLOT_YIELDS);
		MOD_OPT_CACHE(RELIGION_POLICY_BRANCH_FAITH_GP);
		MOD_OPT_CACHE(RELIGION_LOCAL_RELIGIONS);

		MOD_OPT_CACHE(PROCESS_STOCKPILE);

		MOD_OPT_CACHE(AI_NO_ZERO_VALUE_TRADE_ITEMS);
		MOD_OPT_CACHE(AI_SECONDARY_WORKERS);
		MOD_OPT_CACHE(AI_SECONDARY_SETTLERS);
		MOD_OPT_CACHE(AI_GREAT_PEOPLE_CHOICES);
		MOD_OPT_CACHE(AI_MP_DIPLOMACY);
		MOD_OPT_CACHE(AI_SMART_V3);
		
		MOD_OPT_CACHE(EVENTS_TERRAFORMING);
		MOD_OPT_CACHE(EVENTS_TILE_IMPROVEMENTS);
		MOD_OPT_CACHE(EVENTS_TILE_REVEALED);
		MOD_OPT_CACHE(EVENTS_CIRCUMNAVIGATION);
		MOD_OPT_CACHE(EVENTS_NEW_ERA);
		MOD_OPT_CACHE(EVENTS_NW_DISCOVERY);
		MOD_OPT_CACHE(EVENTS_DIPLO_EVENTS);
		MOD_OPT_CACHE(EVENTS_DIPLO_MODIFIERS);
		MOD_OPT_CACHE(EVENTS_MINORS);
		MOD_OPT_CACHE(EVENTS_MINORS_GIFTS);
		MOD_OPT_CACHE(EVENTS_MINORS_INTERACTION);
		MOD_OPT_CACHE(EVENTS_QUESTS);
		MOD_OPT_CACHE(EVENTS_BARBARIANS);
		MOD_OPT_CACHE(EVENTS_GOODY_CHOICE);
		MOD_OPT_CACHE(EVENTS_GOODY_TECH);
		MOD_OPT_CACHE(EVENTS_AI_OVERRIDE_TECH);
		MOD_OPT_CACHE(EVENTS_GREAT_PEOPLE);
		MOD_OPT_CACHE(EVENTS_FOUND_RELIGION);
		MOD_OPT_CACHE(EVENTS_ACQUIRE_BELIEFS);
		MOD_OPT_CACHE(EVENTS_RELIGION);
		MOD_OPT_CACHE(EVENTS_ESPIONAGE);
		MOD_OPT_CACHE(EVENTS_PLOT);
		MOD_OPT_CACHE(EVENTS_PLAYER_TURN);
		MOD_OPT_CACHE(EVENTS_GOLDEN_AGE);
		MOD_OPT_CACHE(EVENTS_CITY);
		MOD_OPT_CACHE(EVENTS_CITY_CAPITAL);
		MOD_OPT_CACHE(EVENTS_CITY_BORDERS);
		MOD_OPT_CACHE(EVENTS_CITY_FOUNDING);
		MOD_OPT_CACHE(EVENTS_LIBERATION);
		MOD_OPT_CACHE(EVENTS_CITY_RAZING);
		MOD_OPT_CACHE(EVENTS_CITY_AIRLIFT);
		MOD_OPT_CACHE(EVENTS_CITY_BOMBARD);
		MOD_OPT_CACHE(EVENTS_CITY_CONNECTIONS);
		MOD_OPT_CACHE(EVENTS_AREA_RESOURCES);
		MOD_OPT_CACHE(EVENTS_PARADROPS);
		MOD_OPT_CACHE(EVENTS_UNIT_RANGEATTACK);
		MOD_OPT_CACHE(EVENTS_UNIT_CREATED);
		MOD_OPT_CACHE(EVENTS_UNIT_FOUNDED);
		MOD_OPT_CACHE(EVENTS_UNIT_PREKILL);
		MOD_OPT_CACHE(EVENTS_UNIT_CAPTURE);
		MOD_OPT_CACHE(EVENTS_CAN_MOVE_INTO);
		MOD_OPT_CACHE(EVENTS_UNIT_ACTIONS);
		MOD_OPT_CACHE(EVENTS_UNIT_UPGRADES);
		MOD_OPT_CACHE(EVENTS_UNIT_DATA);
		MOD_OPT_CACHE(EVENTS_WAR_AND_PEACE);
		MOD_OPT_CACHE(EVENTS_TRADE_ROUTES);
		MOD_OPT_CACHE(EVENTS_TRADE_ROUTE_PLUNDERED);
		MOD_OPT_CACHE(EVENTS_RESOLUTIONS);
		MOD_OPT_CACHE(EVENTS_IDEOLOGIES);
		MOD_OPT_CACHE(EVENTS_NUCLEAR_DETONATION);
		MOD_OPT_CACHE(EVENTS_AIRLIFT);
		MOD_OPT_CACHE(EVENTS_REBASE);
		MOD_OPT_CACHE(EVENTS_COMMAND);
		MOD_OPT_CACHE(EVENTS_CUSTOM_MISSIONS);
		MOD_OPT_CACHE(EVENTS_BATTLES);
		MOD_OPT_CACHE(EVENTS_BATTLES_DAMAGE);
		MOD_OPT_CACHE(EVENTS_BATTLES_CUSTOM_DAMAGE);
		MOD_OPT_CACHE(EVENTS_TRADE_ROUTE_MOVE);
		MOD_OPT_CACHE(PSEUDO_NATURAL_WONDER);

		MOD_OPT_CACHE(API_RELIGION_EXTENSIONS);
		MOD_OPT_CACHE(API_PLAYER_LOGS);
		MOD_OPT_CACHE(API_ESPIONAGE);
		MOD_OPT_CACHE(API_TRADEROUTES);
		MOD_OPT_CACHE(API_RELIGION);
		MOD_OPT_CACHE(API_PLOT_BASED_DAMAGE);
		MOD_OPT_CACHE(API_PLOT_YIELDS);
		MOD_OPT_CACHE(API_VP_ADJACENT_YIELD_BOOST);
		MOD_OPT_CACHE(API_BUILDING_ENABLE_PURCHASE_UNITS);
		MOD_OPT_CACHE(API_ACHIEVEMENTS);
		MOD_OPT_CACHE(API_EXTENSIONS);
		MOD_OPT_CACHE(API_LUA_EXTENSIONS);

		MOD_OPT_CACHE(API_MP_PLOT_SIGNAL);
		MOD_OPT_CACHE(API_PROMOTION_TO_PROMOTION_MODIFIERS);
		MOD_OPT_CACHE(API_UNIT_CANNOT_BE_RANGED_ATTACKED);
		MOD_OPT_CACHE(API_TRADE_ROUTE_YIELD_RATE);

		MOD_OPT_CACHE(CONFIG_GAME_IN_XML);
		MOD_OPT_CACHE(CONFIG_AI_IN_XML);

		MOD_OPT_CACHE(BUGFIX_DUMMY_POLICIES);
		MOD_OPT_CACHE(BUGFIX_RADARING);
		MOD_OPT_CACHE(BUGFIX_RESEARCH_OVERFLOW);
		MOD_OPT_CACHE(BUGFIX_LUA_CHANGE_VISIBILITY_COUNT);
		MOD_OPT_CACHE(BUGFIX_RELIGIOUS_SPY_PRESSURE);
		MOD_OPT_CACHE(BUGFIX_MOVE_AFTER_PURCHASE);
		MOD_OPT_CACHE(BUGFIX_UNITCLASS_NOT_UNIT);
		MOD_OPT_CACHE(BUGFIX_BUILDINGCLASS_NOT_BUILDING);
		MOD_OPT_CACHE(BUGFIX_FREE_FOOD_BUILDING);
		MOD_OPT_CACHE(BUGFIX_NAVAL_FREE_UNITS);
		MOD_OPT_CACHE(BUGFIX_NAVAL_NEAREST_WATER);
		MOD_OPT_CACHE(BUGFIX_CITY_STACKING);
		MOD_OPT_CACHE(BUGFIX_BARB_CAMP_TERRAINS);
		MOD_OPT_CACHE(BUGFIX_BARB_CAMP_SPAWNING);
		MOD_OPT_CACHE(BUGFIX_BARB_GP_XP);
		MOD_OPT_CACHE(BUGFIX_REMOVE_GHOST_ROUTES);
		MOD_OPT_CACHE(BUGFIX_UNITS_AWAKE_IN_DANGER);
		MOD_OPT_CACHE(BUGFIX_WORKERS_VISIBLE_DANGER);
		MOD_OPT_CACHE(BUGFIX_FEATURE_REMOVAL);
		MOD_OPT_CACHE(BUGFIX_INTERCEPTOR_STRENGTH);
		MOD_OPT_CACHE(BUGFIX_UNIT_POWER_CALC);
		MOD_OPT_CACHE(BUGFIX_UNIT_POWER_BONUS_VS_DOMAIN_ONLY);
		MOD_OPT_CACHE(BUGFIX_UNIT_POWER_NAVAL_CONSISTENCY);
		MOD_OPT_CACHE(BUGFIX_UNIT_PREREQ_PROJECT);
		MOD_OPT_CACHE(BUGFIX_NO_HOVERING_REBELS);
		MOD_OPT_CACHE(BUGFIX_HOVERING_PATHFINDER);
		MOD_OPT_CACHE(BUGFIX_EMBARKING_PATHFINDER);
		MOD_OPT_CACHE(BUGFIX_BUILDING_FREEBUILDING);

		MOD_OPT_CACHE(BALANCE_CORE);

		MOD_OPT_CACHE(ERA_EFFECTS_EXTENSIONS);


		MOD_OPT_CACHE(ROG_CORE);


		MOD_OPT_CACHE(EVENTS_TILE_SET_OWNER);
		MOD_OPT_CACHE(EVENTS_IMPROVEMENTS_PILLAGED);
		MOD_OPT_CACHE(EVENTS_GREAT_WORK_CREATED);
		MOD_OPT_CACHE(EVENTS_GREAT_PEOPLE_BOOST);
		MOD_OPT_CACHE(EVENTS_CITY_PUPPETED);
		MOD_OPT_CACHE(EVENTS_WLKD_DAY);
		MOD_OPT_CACHE(EVENTS_CITY_RANGE_STRIKE);
		MOD_OPT_CACHE(EVENTS_UNIT_CAN_RANGEATTACK);
		MOD_OPT_CACHE(EVENTS_UNIT_MOVE);
		MOD_OPT_CACHE(EVENTS_UNIT_DO_TURN);
	
		MOD_OPT_CACHE(IMPROVEMENTS_UPGRADE);
		MOD_OPT_CACHE(IMPROVEMENTS_CREATE_ITEMS);
		MOD_OPT_CACHE(IMPROVEMENT_TRADE_ROUTE_BONUSES);

		MOD_OPT_CACHE(GLOBAL_UNLIMITED_ONE_TURN_GROWTH);
		MOD_OPT_CACHE(GLOBAL_UNLIMITED_ONE_TURN_PRODUCTION);
		MOD_OPT_CACHE(GLOBAL_UNLIMITED_ONE_TURN_CULTURE);
		MOD_OPT_CACHE(GLOBAL_WAR_CASUALTIES);

		MOD_OPT_CACHE(GLOBAL_UNIT_EXTRA_ATTACK_DEFENSE_EXPERENCE);

		MOD_OPT_CACHE(PROMOTION_SPLASH_DAMAGE);
		MOD_OPT_CACHE(PROMOTION_COLLATERAL_DAMAGE);
		MOD_OPT_CACHE(PROMOTION_ADD_ENEMY_PROMOTIONS);
		MOD_OPT_CACHE(PROMOTION_CITY_DESTROYER);

		MOD_OPT_CACHE(GLOBAL_PROMOTIONS_REMOVAL);
		MOD_OPT_CACHE(TRAIT_RELIGION_FOLLOWER_EFFECTS);

		MOD_OPT_CACHE(PROMOTION_FEATURE_INVISIBLE);
		MOD_OPT_CACHE(PROMOTION_MULTIPLE_INIT_EXPERENCE);
		MOD_OPT_CACHE(PROMOTION_REMOVE_PROMOTION_UPGRADE);
		MOD_OPT_CACHE(PROMOTION_GET_INSTANCE_FROM_ATTACK);
		MOD_OPT_CACHE(GLOBAL_TRIGGER_NEW_GOLDEN_AGE_IN_GA);
		MOD_OPT_CACHE(POLICY_WATER_BUILD_SPEED_MODIFIER);
		MOD_OPT_CACHE(GLOBAL_UNIT_MOVES_AFTER_DISEMBARK);
		MOD_OPT_CACHE(GLOBAL_BUILDING_INSTANT_YIELD);
		MOD_OPT_CACHE(BELIEF_BIRTH_INSTANT_YIELD);

		MOD_OPT_CACHE(GLOBAL_CITY_SCALES);
		MOD_OPT_CACHE(EVENTS_CITY_SCALES);

		MOD_OPT_CACHE(SPECIALIST_RESOURCES);

		MOD_OPT_CACHE(POLICIY_PUBLIC_OPTION);

		MOD_OPT_CACHE(TRAITS_GOLDEN_AGE_YIELD_MODIFIER);
		MOD_OPT_CACHE(BUGFIX_CITY_NEGATIVE_YIELD_MODIFIED);

		MOD_OPT_CACHE(BATTLE_CAPTURE_NEW_RULE);

		MOD_OPT_CACHE(BUGFIX_INVISIBLE_UNIT_MOVE_ENEMY_CITY);
		
		m_bInit = true;
	}

	if (m_options.find(sOption) == m_options.end()) {
		return defValue;
	}

	return m_options[sOption];
}

int CustomMods::getCivOption(const char* szCiv, const char* szName, int defValue) {
	return getOption(string(szCiv) + "_" + szName, getOption(szName, defValue));
}
