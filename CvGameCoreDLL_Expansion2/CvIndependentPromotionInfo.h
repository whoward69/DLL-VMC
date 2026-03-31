#pragma once
#ifndef CV_INDEPENDENTPROMOTIONINFO_H
#define CV_INDEPENDENTPROMOTIONINFO_H
class CvIndependentPromotionInfo
{
public:
    void Init();
	int GetAllyCityStateCombatModifier(const CvUnit& pUnit) const;
	int GetHappinessCombatModifier(const CvUnit& pUnit) const;
	int GetResourceCombatModifier(const CvUnit& pUnit) const;
	int GetNearbyUnitPromotionBonus(const CvUnit& pUnit) const;
private:
	std::vector<PromotionTypes> m_vAllyCityStateCombatModifiers;
	std::vector<PromotionTypes> m_vHappinessCombatModifiers;
	std::vector<PromotionTypes> m_vResourceCombatModifiers;
	std::vector<PromotionTypes> m_vNearbyUnitPromotionBonuses;
};

#endif