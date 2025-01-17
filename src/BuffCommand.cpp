#include "ScriptMgr.h"
#include "BuffCommand.h"
#include "World.h"
#include "WorldSession.h"
#include "Config.h"
#include "Player.h"
#include "Chat.h"

std::unordered_map<ObjectGuid, uint32> BuffCooldown;

void Kargatum_Buff::LoadDB()
{
	uint32 oldMSTime = getMSTime();
	_buffStore.clear();

	QueryResult result = WorldDatabase.PQuery("SELECT spellid FROM `player_buff`");
	if (!result)
	{
		sLog->outErrorDb(">> Loaded 0 buff. DB table `player_buff` is empty!");
		sLog->outString();
		return;
	}

	do
	{
		uint32 id = result->Fetch()->GetInt32();
		_buffStore.push_back(id);

	} while (result->NextRow());

	sLog->outString(">> Loaded %u buff in %u ms", uint32(_buffStore.size()), GetMSTimeDiffToNow(oldMSTime));
	sLog->outString();
}

class KargatumCS_BuffCOmmand : public CommandScript
{
public:
	KargatumCS_BuffCOmmand() : CommandScript("KargatumCS_BuffCOmmand") {}

	std::vector<ChatCommand> GetCommands() const override
	{
		static std::vector<ChatCommand> commandTable = // .commands
		{
			{ "buff",				SEC_PLAYER,			false, &HandleBuffCommand,	"" }
		};

		return commandTable;
	}

	static bool HandleBuffCommand(ChatHandler *handler, const char *args)
	{
		Player* player = handler->GetSession()->GetPlayer();
		std::string ArgStr = (char*)args;

        if (sConfigMgr->GetIntDefault("BuffCommand.Enable", 1) == 0) {
            handler->SendSysMessage("The command is currently disabled");
            handler->SetSentErrorMessage(true);
            return false;
        }

		if (sConfigMgr->GetIntDefault("BuffCommand.MinLevel", 80) > 0) {
			uint8 MinLevel  = sConfigMgr->GetIntDefault("BuffCommand.MinLevel", 80);
			if (player->getLevel() < sConfigMgr->GetIntDefault("BuffCommand.MinLevel", 80))
			{
				std::string MinLevelError = "You must be ";
				if (MinLevel != 80)
					MinLevelError += "atleast ";
				MinLevelError += "level " + std::to_string(MinLevel) + " to use this command.";
				handler->SendSysMessage((MinLevelError).c_str());
	            handler->SetSentErrorMessage(true);
				return false;
			}
		}

		if (ArgStr == "reload" && AccountMgr::IsAdminAccount(player->GetSession()->GetSecurity()))
		{
			sLog->outString("Re-Loading Player Buff data...");
			sKargatumBuff->LoadDB();
			handler->SendGlobalGMSysMessage("|cff6C8CD5#|cFFFF0000 Table|r `player_buff` |cFFFF0000re.|r");
			return true;
		}
		else
		{      
//			if (player->duel || player->GetMap()->IsBattleArena() || player->InBattleground() || player->HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH) || player->isDead()|| player->IsInCombat() || player->IsInFlight() || player->HasStealthAura() || player->HasInvisibilityAura())
			if (player->duel || player->GetMap()->IsBattleArena() || player->InBattleground() || !player->GetMap()->IsRaid() || player->HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH) || player->isDead()|| player->IsInCombat() || player->IsInFlight() || player->HasStealthAura() || player->HasInvisibilityAura())
			{
				handler->SendSysMessage("You can use this command only inside Raid instance");
				handler->SetSentErrorMessage(true);
				return false;
            }

//            if (!player->GetMap()->IsRaid())
//              {
//                    player->RemoveAura(75447)
//              }

            auto searchGUID = BuffCooldown.find(player->GetGUID());

            if (searchGUID == BuffCooldown.end())
                BuffCooldown[player->GetGUID()] = 0; // Leader GUID not found, initialize with 0

            if (sWorld->GetGameTime() - BuffCooldown[player->GetGUID()] < sConfigMgr->GetIntDefault("BuffCommand.Cooldown", 120) || sWorld->GetGameTime() == BuffCooldown[player->GetGUID()])
            {
                handler->SendSysMessage(("You have to wait atleast " + std::to_string(sConfigMgr->GetIntDefault("BuffCommand.Cooldown", 120)) + " seconds before using .buff again!").c_str());
                handler->SetSentErrorMessage(true);
				return false;
            }

            if (sWorld->GetGameTime() - BuffCooldown[player->GetGUID()] >= sConfigMgr->GetIntDefault("BuffCommand.Cooldown", 120)) {
            	BuffCooldown[player->GetGUID()] = sWorld->GetGameTime();
            }

            player->RemoveAurasByType(SPELL_AURA_MOUNTED);

			Kargatum_Buff::Kargatum_Buff_Container& sn = sKargatumBuff->GetBuffData();
			for (auto i : sn)
				player->CastSpell(player, i, true);

			return true;
		}
	}
};

//void OnMapChanged(Player* player)
//    {
//        if (!player->GetMap()->IsRaid())
//            {
//                player->RemoveAura(75447);
//            }
//    }

//class MyPlayer : public PlayerScript
//{
//public:
//    MyPlayer() : PlayerScript("MyPlayer") { }

//    void OnMapChanged(Player* player) override
//    {
//        if (!player->GetMap()->IsRaid())
//        {
//            player->RemoveAura(75447);
//        }
//    }
//};

// Add player scripts
class MyPlayer : public PlayerScript
{
public:
    MyPlayer() : PlayerScript("MyPlayer") { }

    void OnLogin(Player* player) override
    {
        if (sConfigMgr->GetOption<bool>("BuffCommand.Enable", false))
        {
            ChatHandler(player->GetSession()).SendSysMessage("Buff reset module is active!");
        }
    }
};

// Remove player buffs after exiting ICC
//class ClearBuffs : public PlayerScript
//{
//public:

//    ClearBuffs() : PlayerScript("ClearBuffs") {}
    
//void ApplyBuffs(Player* player, Map* map)
//    {
//        if (!player->GetMap()->IsRaid())
//            {
//                player->RemoveAura(75447);
//            }
//        else
//        ClearBuffs(player, map);
//    }
//};
    
//    void OnMapChanged(Player *player) override {
////        if (sConfigMgr->GetIntDefault("BuffCommand.Enable", 1) == 1 && GetVirtualMapForMapAndZone(GetMapId(),newZone) != 631)
////        if (sConfigMgr->GetIntDefault("BuffCommand.Enable", 1) == 1)
////        {
////            Map *map = player->GetMap();
////            player->RemoveAura(75447);
////        }
////    }

//if (!player->GetMap()->IsRaid())
//			{
//                player->RemoveAura(75447);
////				handler->SendSysMessage("Buffs Cleared Test");
////				handler->SetSentErrorMessage(true);
////				return false;
//            }
//      }
//};

//    void OnMapChanged(Player *player) override {
//        if (sConfigMgr->GetBoolDefault("BuffCommand.Enable", true))
//        if (sConfigMgr->GetIntDefault("BuffCommand.Enable", 1) == 0)
//        {
//            Map *map = player->GetMap();
//            float difficulty = CalculateDifficulty(map, player);
//			int dunLevel = CalculateDungeonLevel(map, player);
//            int numInGroup = GetNumInGroup(player);
//            ApplyBuffs(player, map, difficulty, dunLevel, numInGroup);
//        }
//    }

//    void OnMapChanged(Player *player) override {
//        if (sConfigMgr->GetBoolDefault("BuffCommand.Enable", true))
//        {
//           player->RemoveAura(75447)
//           ClearBuffs(player, map);
//        }
//    }

//    void OnMapChanged(Player *player)
//        {
////          if (sConfigMgr->GetBoolDefault("BuffCommand.Enable", true))
//           if (!player->GetMap()->IsRaid())
//            {
//               player->RemoveAura(75447);
////               ClearBuffs(player, map);
//            }
//        }

//void OnMapChanged(Player* /*player*/)
//    {
    
//        Player* player = handler->GetSession()->GetPlayer();
//		std::string ArgStr = (char*)args;

//        if (sConfigMgr->GetIntDefault("BuffCommand.Enable", 1) == 0) {
//            handler->SendSysMessage("The command is currently disabled");
//            handler->SetSentErrorMessage(true);
//            return false;
//        }
//    
//        else
//        {
//            player->RemoveAura(75447);
//        }
//    }

class Kargatum_BuffLoad : public WorldScript
{
public:
	Kargatum_BuffLoad() : WorldScript("Kargatum_BuffLoad") {}

	void OnStartup() override
	{
		sLog->outString("Loading player buff command ...");
		sKargatumBuff->LoadDB();
	}
};

void AddMyPlayerScripts()
{
    new MyPlayer();
}

void AddBuffCommandScripts()
{
	new KargatumCS_BuffCOmmand();
	new Kargatum_BuffLoad();
}
