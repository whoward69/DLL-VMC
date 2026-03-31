#include "CvGameCoreDLLPCH.h"
#include "CvLuaFormula.h"

bool CvLuaFormula::CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility)
{
	if (!CvBaseInfo::CacheResults(kResults, kUtility))
	{
		return false;
	}

	const char* text = kResults.GetText("Formula");
	if (text == nullptr)
	{
		m_strFormula = "";
	}
	else
	{
		m_strFormula = text;
	}

	return true;
}

std::string CvLuaFormula::GetFormula() const
{
	return m_strFormula;
}

std::vector<CvLuaFormula*>& CvLuaFormulaXMLEntries::GetEntries()
{
	return m_vEntries;
}
CvLuaFormula* CvLuaFormulaXMLEntries::GetEntry(LuaFormulaTypes eFormula)
{
	if (eFormula < 0 || eFormula >= m_vEntries.size())
	{
		return nullptr;
	}

	return m_vEntries[eFormula];
}
int CvLuaFormulaXMLEntries::GetNumEntries()
{
	return m_vEntries.size();
}

namespace lua {
	Evaluator* EvaluatorManager::GetEvaluator(const char* type)
	{
		if (m_pGlobal == nullptr)
		{
			return nullptr;
		}
		LuaFormulaTypes eType = static_cast<LuaFormulaTypes>(m_pGlobal->getInfoTypeForString(type));
		return GetEvaluator(eType);
	}

	Evaluator* EvaluatorManager::GetEvaluator(LuaFormulaTypes eType)
	{
		auto it = m_data.find(eType);
		if (it == m_data.end())
		{
			return nullptr;
		}

		return &it->second;
	}

	bool EvaluatorManager::Init(CvGlobals* gc)
	{
		if (gc == nullptr)
		{
			return false;
		}

		m_pGlobal = gc;
		m_data.clear();
		EvaluatorFactory factory;
		for (auto* formula : gc->GetLuaFormulaEntries())
		{
			Evaluator evaluator;
			if (!factory.Init(formula, &evaluator))
			{
				continue;
			}
			m_data[static_cast<LuaFormulaTypes>(formula->GetID())] = static_cast<Evaluator&&>(evaluator);
		}

		return true;
	}
}