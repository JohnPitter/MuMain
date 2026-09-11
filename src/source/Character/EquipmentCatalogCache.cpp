#include "Character/EquipmentCatalogCache.h"

namespace Character::Equipment
{
    namespace
    {
        BonusCatalog SessionCatalog;
    }

    const BonusCatalog& GetCatalog() { return SessionCatalog; }
    void SetCatalog(const BonusCatalog& catalog) { SessionCatalog = catalog; }
    void ResetCatalog() { SessionCatalog = {}; }
}
