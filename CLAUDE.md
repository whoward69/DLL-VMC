# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

CIV5MPDLL is a Civilization V DLL mod framework based on whoward69's DLL-VMC v97. Its key innovation is the `SendAndExecuteLuaFunction` system that enables multiplayer synchronization for mods by broadcasting Lua-C++ calls across all clients.

## Core Architecture

### 1. Database-Driven Architecture

The game uses SQLite database to store all configuration data. Key components:

- **Database Tables**: Define game rules (Units, Buildings, Promotions, etc.)
- **Info Classes**: C++ classes that load and cache database data (e.g., `CvUnitEntry`, `CvBuildingEntry`)
- **Global Context (GC)**: Singleton providing access to all loaded game data

### 2. Main Game Classes

**CvGame** - Central game manager
- Controls turn processing, victory conditions, game state
- Singleton accessed via `GC.getGame()`

**CvPlayer** - Represents each civilization
- Contains cities, units, techs, policies, religions
- AI subsystems (Military, Economic, Diplomacy, Grand Strategy)

**CvCity** - Individual city management
- Buildings, citizens, production, yields
- Components: `CvCityBuildings`, `CvCityCitizens`, `CvCityReligions`

**CvUnit** - Military and civilian units
- Movement, combat, promotions, missions
- Mission queue for AI automation

**CvPlot** - Map hexagonal tiles
- Terrain, features, improvements, resources
- Unit presence and visibility

### 3. Modding Framework

**CustomMods System**
- Runtime feature toggles stored in `CustomModOptions` database table
- Options loaded via `gCustomMods.isOPTION_NAME()` checks
- Defined in `CustomMods.h` with `MOD_OPT_DECL` macros

**Lua Integration**
- C++ classes exposed through `CvLua*` wrappers
- Static and instance methods registered for Lua access
- Event system for mod callbacks

**Multiplayer Synchronization (CIV5MPDLL Specific)**
- `SendAndExecuteLuaFunction` broadcasts function calls to all clients
- Uses Protocol Buffers for serialization
- Available on Game, Map, Unit, Player, Plot, City objects

## Adding New SQL Fields - Step by Step

When adding a new database field (e.g., `NumSpyStayAttackMod`), modify these files:

### 1. SQL Definition
**File**: `(1) CIV5MPDLL/DB/NewTableChanges.sql`
```sql
ALTER TABLE UnitPromotions ADD 'NumSpyStayAttackMod' INTEGER DEFAULT 0;
```

### 2. Info Class (Database Reading)

For **UnitPromotions** example:

**CvPromotionClasses.h**
```cpp
private:
    int m_iNumSpyStayAttackMod;
public:
    int GetNumSpyStayAttackMod() const;
```

**CvPromotionClasses.cpp**
```cpp
// Constructor initialization:
m_iNumSpyStayAttackMod(0),

// In CacheResults():
m_iNumSpyStayAttackMod = kResults.GetInt("NumSpyStayAttackMod");

// Getter implementation:
int CvPromotionEntry::GetNumSpyStayAttackMod() const {
    return m_iNumSpyStayAttackMod;
}
```

### 3. Game Logic Integration

**CvUnit.h** (for unit-level tracking)
```cpp
private:
    int m_iNumSpyStayAttackMod;
public:
    void ChangeNumSpyStayAttackMod(int iValue);
    int GetNumSpyStayAttackMod() const;
```

**CvUnit.cpp**
```cpp
// Constructor (with sync archive for MP):
m_iNumSpyStayAttackMod("CvUnit::m_iNumSpyStayAttackMod", m_syncArchive)

// Initialize in init():
m_iNumSpyStayAttackMod = 0;

// Change/Get methods:
void CvUnit::ChangeNumSpyStayAttackMod(int iValue) {
    m_iNumSpyStayAttackMod += iValue;
}
int CvUnit::GetNumSpyStayAttackMod() const {
    return m_iNumSpyStayAttackMod;
}

// In processPromotion():
ChangeNumSpyStayAttackMod(thisPromotion.GetNumSpyStayAttackMod() * iChange);

// Serialization (read/write methods):
kStream >> m_iNumSpyStayAttackMod;
kStream << m_iNumSpyStayAttackMod;
```

### 4. Apply Game Effect
Use the value where needed, e.g., in combat calculations:
```cpp
if (iNumSpyStayAttackMod != 0 && GET_TEAM(getTeam()).HasSpyAtTeam(pDefender->getTeam())) {
    iModifier += iNumSpyStayAttackMod;
}
```

### 5. Lua Binding (Optional)
**CvLuaUnit.h**
```cpp
static int lGetNumSpyStayAttackMod(lua_State* L);
```

**CvLuaUnit.cpp**
```cpp
// Register in RegistInstanceFunctions():
Method(GetNumSpyStayAttackMod);

// Implementation:
int CvLuaUnit::lGetNumSpyStayAttackMod(lua_State* L) {
    CvUnit* pkUnit = GetInstance(L);
    lua_pushinteger(L, pkUnit->GetNumSpyStayAttackMod());
    return 1;
}
```

## Key Patterns

### Boolean Promotions
For stackable boolean effects:
```cpp
// Use counter instead of bool
int m_iImmueMeleeAttack;

// Check method:
bool IsImmueMeleeAttack() const { 
    return m_iImmueMeleeAttack > 0; 
}

// In processPromotion:
ChangeImmueMeleeAttackCount(thisPromotion.IsImmueMeleeAttack() ? iChange : 0);
```

### Database Value Types
- `GetInt()` - INTEGER columns
- `GetBool()` - BOOLEAN columns (0/1)
- `GetText()` - TEXT columns
- `GetFloat()`/`GetDouble()` - Floating point

### Multiplayer Sync
For unit member variables, use sync archive:
```cpp
m_iValue("CvUnit::m_iValue", m_syncArchive)
```

## Build Instructions

```bash
# Update version information
cd CIV5MPDLL
update_commit_id.bat

# Build with Visual Studio
# Open: CvGameCoreDLL.vs2022.sln
# Build: Release|Win32
# Output: BuildOutput/VS2022_ReleaseWin32/CvGameCore_Expansion2.dll
```

**Prerequisites**: VS2008 SP1 toolset + VS2013/2022 IDE

## File Organization

- **SQL Changes**: `(1) CIV5MPDLL/DB/` - Database modifications
- **C++ Core**: `CvGameCoreDLL_Expansion2/` - Game logic
- **Lua Scripts**: `LUA/` - Mod scripts
- **UI Mods**: `UI/` - Interface modifications

## Important Notes

1. **Multiplayer Safety**: Avoid non-deterministic operations (e.g., std::map with pointer keys)
2. **Lua-UI Separation**: UI hooks must not modify game state
3. **Binary Compatibility**: Member variable order affects save game compatibility
4. **CustomMods**: Check feature flags with `gCustomMods.isFeatureName()` before using modded features