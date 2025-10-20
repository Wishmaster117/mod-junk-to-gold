#include "j2g_conf.h"
#include "Config.h"
#include "Log.h"

namespace
{
    bool sEnabled  = true;
    bool sAnnounce = true;
	bool sEnableForHumans = true;
	bool sSellCommonIfWorse = false;
	bool sSellWeaponsIfWorse = false;
}

namespace J2G
{
    bool IsEnabled()
    {
        return sEnabled;
    }

    void LoadConfig()
    {
        sEnabled  = sConfigMgr->GetOption<bool>("JunkToGold.Enable",   true);
        sAnnounce = sConfigMgr->GetOption<bool>("JunkToGold.Announce", true);
		sEnableForHumans = sConfigMgr->GetOption<bool>("JunkToGold.EnableForHumans", true);
		sSellCommonIfWorse = sConfigMgr->GetOption<bool>("JunkToGold.SellCommonIfWorse", false);
		sSellWeaponsIfWorse = sConfigMgr->GetOption<bool>("JunkToGold.SellWeaponsIfWorse", false);

        if (sAnnounce)
            LOG_INFO("module", "mod-junk-to-gold: {}", sEnabled ? "enabled" : "disabled");
    }

    bool EnableForHumans()
    {
        return sEnableForHumans;
    }
	 
    bool SellCommonIfWorse()
    {
        return sSellCommonIfWorse;
    }

    bool SellWeaponsIfWorse()
    {
        return sSellWeaponsIfWorse;
    }

    void World::OnAfterConfigLoad(bool /*reload*/)
    {
        LoadConfig();
    }
}
