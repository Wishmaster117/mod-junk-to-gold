# Junk to Gold
This module will automatically sell gray items when the player loots them

# Difs between the original mod:

Add Config file + smarter auto-sell + Human player detection

# Summary :

New config file (conf/mod_junk_to_gold.conf.dist) with runtime reload.

Enable/disable switch: JunkToGold.Enable (1 on / 0 off).

White items auto-sell (opt-in): JunkToGold.SellCommonIfWorse=1 => sell if strictly worse than equipped.

White weapons auto-sell (opt-in): JunkToGold.SellWeaponsIfWorse=1 => DPS-aware, sell if strictly worse.

Sell only for bots or Human and bots: JunkToGold.EnableForHumans= 1 => Selling for booth

Backwards compatible: defaults keep current behavior (sell greys only).

Config (example):

[worldserver]
JunkToGold.Enable = 1
JunkToGold.Announce = 1
JunkToGold.SellCommonIfWorse = 0
JunkToGold.SellWeaponsIfWorse = 0
JunkToGold.EnableForHumans = 1

