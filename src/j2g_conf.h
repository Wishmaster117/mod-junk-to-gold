#pragma once

#include "ScriptMgr.h"

namespace J2G
{
    bool IsEnabled();

    bool SellCommonIfWorse();

    bool SellWeaponsIfWorse();

    void LoadConfig();

    class World final : public WorldScript
    {
    public:
        World() : WorldScript("J2G_WorldScript") {}
        void OnAfterConfigLoad(bool /*reload*/) override;
    };
}
