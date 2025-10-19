#include "Chat.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "j2g_conf.h"
#include "Item.h"
#include "SharedDefines.h"
#include <vector>
#include <cmath>

// --- Helpers to compare a white-quality loot item with the current equipment (armor & weapons) ---
static inline int CompareItemTemplates(ItemTemplate const* a, ItemTemplate const* b)
{
    // >0: a is better; 0: equal; <0: a is worse
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    if (a->Quality != b->Quality)
        return int(a->Quality) - int(b->Quality);
    if (a->ItemLevel != b->ItemLevel)
        return int(a->ItemLevel) - int(b->ItemLevel);
    if (a->RequiredLevel != b->RequiredLevel)
        return int(a->RequiredLevel) - int(b->RequiredLevel);
    return 0;
}

static inline bool IsWeaponInvType(uint32 invType)
{
    switch (invType)
    {
        case INVTYPE_WEAPON:
        case INVTYPE_WEAPONMAINHAND:
        case INVTYPE_WEAPONOFFHAND:
        case INVTYPE_2HWEAPON:
        case INVTYPE_RANGED:
        case INVTYPE_RANGEDRIGHT:
        case INVTYPE_THROWN:
#ifdef INVTYPE_WAND
        case INVTYPE_WAND:
#endif
            return true;
        default:
            return false;
    }
}

static inline double ComputeWeaponDPS(ItemTemplate const* proto)
{
    if (!proto || proto->Delay == 0)
        return 0.0;
    // Use the first valid damage line
    double minD = 0.0, maxD = 0.0;
    if (proto->Damage[0].DamageMin > 0.0 || proto->Damage[0].DamageMax > 0.0)
    {
        minD = proto->Damage[0].DamageMin;
        maxD = proto->Damage[0].DamageMax;
    }
    else if (proto->Damage[1].DamageMin > 0.0 || proto->Damage[1].DamageMax > 0.0)
    {
        minD = proto->Damage[1].DamageMin;
        maxD = proto->Damage[1].DamageMax;
    }
    double avg = (minD + maxD) * 0.5;
    return (avg * 1000.0) / double(proto->Delay); // dmg per second
}

static inline int CompareForEquipDecision(ItemTemplate const* a, ItemTemplate const* b, bool weaponPreferredDPS)
{
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    if (a->Quality != b->Quality)
        return int(a->Quality) - int(b->Quality);
    if (weaponPreferredDPS)
    {
        double dpsA = ComputeWeaponDPS(a), dpsB = ComputeWeaponDPS(b);
        if (std::fabs(dpsA - dpsB) > 1e-6)
            return dpsA > dpsB ? 1 : -1;
    }
    if (a->ItemLevel != b->ItemLevel)
        return int(a->ItemLevel) - int(b->ItemLevel);
    if (a->RequiredLevel != b->RequiredLevel)
        return int(a->RequiredLevel) - int(b->RequiredLevel);
    return 0;
}

static void GetCandidateSlots(uint32 invType, std::vector<uint8>& slots)
{
    switch (invType)
    {
        case INVTYPE_HEAD:        slots.push_back(EQUIPMENT_SLOT_HEAD); break;
        case INVTYPE_NECK:        slots.push_back(EQUIPMENT_SLOT_NECK); break;
        case INVTYPE_SHOULDERS:   slots.push_back(EQUIPMENT_SLOT_SHOULDERS); break;
        case INVTYPE_BODY:        slots.push_back(EQUIPMENT_SLOT_BODY); break;     // chemise
        case INVTYPE_CHEST:       slots.push_back(EQUIPMENT_SLOT_CHEST); break;
        case INVTYPE_ROBE:        slots.push_back(EQUIPMENT_SLOT_CHEST); break;
        case INVTYPE_WAIST:       slots.push_back(EQUIPMENT_SLOT_WAIST); break;
        case INVTYPE_LEGS:        slots.push_back(EQUIPMENT_SLOT_LEGS); break;
        case INVTYPE_FEET:        slots.push_back(EQUIPMENT_SLOT_FEET); break;
        case INVTYPE_WRISTS:      slots.push_back(EQUIPMENT_SLOT_WRISTS); break;
        case INVTYPE_HANDS:       slots.push_back(EQUIPMENT_SLOT_HANDS); break;
        case INVTYPE_FINGER:      slots.push_back(EQUIPMENT_SLOT_FINGER1); slots.push_back(EQUIPMENT_SLOT_FINGER2); break;
        case INVTYPE_TRINKET:     slots.push_back(EQUIPMENT_SLOT_TRINKET1); slots.push_back(EQUIPMENT_SLOT_TRINKET2); break;
        case INVTYPE_CLOAK:       slots.push_back(EQUIPMENT_SLOT_BACK); break;
        case INVTYPE_SHIELD:      slots.push_back(EQUIPMENT_SLOT_OFFHAND); break;
        // --- Arms ---
        case INVTYPE_WEAPON:          // can go in main hand OR off-hand
            slots.push_back(EQUIPMENT_SLOT_MAINHAND);
            slots.push_back(EQUIPMENT_SLOT_OFFHAND);
            break;
        case INVTYPE_WEAPONMAINHAND:  slots.push_back(EQUIPMENT_SLOT_MAINHAND); break;
        case INVTYPE_WEAPONOFFHAND:   slots.push_back(EQUIPMENT_SLOT_OFFHAND);  break;
        case INVTYPE_2HWEAPON:        slots.push_back(EQUIPMENT_SLOT_MAINHAND); break;
        case INVTYPE_RANGED:
        case INVTYPE_RANGEDRIGHT:
        case INVTYPE_THROWN:
#ifdef INVTYPE_WAND
        case INVTYPE_WAND:
#endif
                                      slots.push_back(EQUIPMENT_SLOT_RANGED);   break;
        default: break;
    }
}

static bool IsWhiteItemWorseThanEquipped(Player* player, ItemTemplate const* proto, bool weapon)
{
    if (!player || !proto)
        return false;

    std::vector<uint8> slots;
    GetCandidateSlots(proto->InventoryType, slots);
    if (slots.empty())
        return false; // non-equippable (not relevant)

    // If any candidate slot is empty OR the loot is >= an equipped item -> don't sell (possible upgrade)
    bool strictlyWorse = false;
    for (uint8 slot : slots)
    {
        Item* eq = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!eq)
            return false; // empty slot -> potentially useful
        ItemTemplate const* eqProto = eq->GetTemplate();
        if (CompareForEquipDecision(proto, eqProto, weapon) >= 0)
            return false; // proto >= an equipped item -> keep it
        strictlyWorse = true; // proto < eqProto for this slot
    }
    return strictlyWorse;
}

class JunkToGold : public PlayerScript
{
public:
    JunkToGold() : PlayerScript("JunkToGold") {}

    //void OnPlayerLootItem(Player* player, Item* item, uint32 count, ObjectGuid /*lootguid*/) override
	void OnPlayerLootItem(Player* player, Item* item, uint32 count, ObjectGuid /*lootGuid*/) override
    {
        // --- Global Toggle ---
        if (!J2G::IsEnabled())
            return; // // module off: do nothing

        if (!item)
            return;
        ItemTemplate const* proto = item->GetTemplate();
        if (!proto)
            return;

        bool shouldSell = false;
        // (1) Grey: always sold
        if (proto->Quality == ITEM_QUALITY_POOR)
            shouldSell = true;
        // (2) White: only (if enabled) when worse than equipped
        else if (proto->Quality == ITEM_QUALITY_NORMAL)
        {
            bool isWeapon = IsWeaponInvType(proto->InventoryType);
            if (isWeapon && J2G::SellWeaponsIfWorse())
                shouldSell = IsWhiteItemWorseThanEquipped(player, proto, /*weapon=*/true);
            else if (!isWeapon && J2G::SellCommonIfWorse())
                shouldSell = IsWhiteItemWorseThanEquipped(player, proto, /*weapon=*/false);
        }

        if (!shouldSell)
            return; // we kkep item in bag

        // Sale: info message + gold conversion + removal (once only)
        SendTransactionInformation(player, item, count);
        player->ModifyMoney(proto->SellPrice * count);
        player->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
    }

private:
    void SendTransactionInformation(Player* player, Item* item, uint32 count)
    {
        std::string name;
        // Color based on quality (0=grey, 1=white)
        const char* qColor = "|cff9d9d9d"; // poor
        switch (item->GetTemplate()->Quality)
        {
            case ITEM_QUALITY_NORMAL: qColor = "|cffffffff"; break; // white
            case ITEM_QUALITY_POOR:   qColor = "|cff9d9d9d"; break; // grey
            default: break;
        }
        if (count > 1)
        {
            name = Acore::StringFormat("{}|Hitem:{}::::::::80:::::|h[{}]|h|rx{}",
                                       qColor, item->GetTemplate()->ItemId, item->GetTemplate()->Name1, count);
        }
        else
        {
            name = Acore::StringFormat("{}|Hitem:{}::::::::80:::::|h[{}]|h|r",
                                       qColor, item->GetTemplate()->ItemId, item->GetTemplate()->Name1);
        }

        uint32 money = item->GetTemplate()->SellPrice * count;
        uint32 gold = money / GOLD;
        uint32 silver = (money % GOLD) / SILVER;
        uint32 copper = (money % GOLD) % SILVER;

        std::string info;
        if (money < SILVER)
        {
            info = Acore::StringFormat("{} sold for {} copper.", name, copper);
        }
        else if (money < GOLD)
        {
            if (copper > 0)
            {
                info = Acore::StringFormat("{} sold for {} silver and {} copper.", name, silver, copper);
            }
            else
            {
                info = Acore::StringFormat("{} sold for {} silver.", name, silver);
            }
        }
        else
        {
            if (copper > 0 && silver > 0)
            {
                info = Acore::StringFormat("{} sold for {} gold, {} silver and {} copper.", name, gold, silver, copper);
            }
            else if (copper > 0)
            {
                info = Acore::StringFormat("{} sold for {} gold and {} copper.", name, gold, copper);
            }
            else if (silver > 0)
            {
                info = Acore::StringFormat("{} sold for {} gold and {} silver.", name, gold, silver);
            }
            else
            {
                info = Acore::StringFormat("{} sold for {} gold.", name, gold);
            }
        }

        ChatHandler(player->GetSession()).SendSysMessage(info);
    }
};

void Addmod_junk_to_goldScripts()
{
    J2G::LoadConfig();

    new J2G::World();
	
    new JunkToGold();
}
