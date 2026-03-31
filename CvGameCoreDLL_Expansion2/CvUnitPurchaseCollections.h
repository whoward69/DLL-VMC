#ifndef CV_UNITPURCHASECOLLECTIONS_H
#define CV_UNITPURCHASECOLLECTIONS_H

class CvUnitPurchaseCollectionsPrivate;
class CvUnitPurchaseCollections
{
public:
	CvUnitPurchaseCollections();
	~CvUnitPurchaseCollections();
    void Init();
    int GetUnitClassPurchaseCost(CvCity *pCity, UnitClassTypes eUnitClass, YieldTypes eYield) const;
private:
	CvUnitPurchaseCollectionsPrivate *m_pPrivte = nullptr;
};
#endif