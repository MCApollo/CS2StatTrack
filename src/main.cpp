#define META_IS_SOURCE2 1

#include "main.h"
#include "iserver.h"
#include "appframework/IAppSystem.h"
#include "icvar.h"
#include "interface.h"
#include "tier0/dbg.h"
#include "plat.h"
#include "te.pb.h"
#include "cs_gameevents.pb.h"
#include "tier0/vprof.h"
#include "tier0/memdbgon.h"

#include "steam/steam_api.h"
#include "GCHelper.hpp"

#include "CPlayer_ItemServices.h"
#include "addresses.h"
#include "protobuf/generated/base_gcmessages_csgo.pb.h"
#include "protobuf/generated/econ_gcmessages.pb.h"

#include <inttypes.h>

void Message(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	char buf[1024] = {};
	V_vsnprintf(buf, sizeof(buf) - 1, msg, args);

	ConColorMsg(Color(255, 0, 255, 255), "\n\n[CS2StatTrack] %s\n\n", buf);
	// META_CONPRINTF("[CS2StatTrack] %s", buf);

	va_end(args);
}

void Panic(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	char buf[1024] = {};
	V_vsnprintf(buf, sizeof(buf) - 1, msg, args);

	Warning("[CS2StatTrack] %s", buf);

	va_end(args);
}

SH_DECL_HOOK0_void(IServerGameDLL, GameServerSteamAPIActivated, SH_NOATTRIB, 0);
SH_DECL_HOOK3_void(ICvar, DispatchConCommand, SH_NOATTRIB, 0, ConCommandHandle, const CCommandContext &, const CCommand &);
SH_DECL_HOOK0_void(IServerGameDLL, GameServerSteamAPIDeactivated, SH_NOATTRIB, 0);
SH_DECL_HOOK1_void(IServerGameClients, ClientSettingsChanged, SH_NOATTRIB, 0, CPlayerSlot);
SH_DECL_HOOK2_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, CPlayerSlot, const CCommand &);

CS2StatTrack g_CS2StatTrack;
CSteamGameServerAPIContext g_steamAPI;
IGameEventManager2 *g_gameEventManager = nullptr;
INetworkGameServer *g_pNetworkGameServer = nullptr;
CGlobalVars *gpGlobals = nullptr;
IVEngineServer2 *g_pEngineServer2 = nullptr;
ISteamHTTP *g_http = nullptr;

PLUGIN_EXPOSE(CS2StatTrack, g_CS2StatTrack);
bool CS2StatTrack::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, g_pEngineServer2, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pGameResourceServiceServer, IGameResourceServiceServer, GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, g_pSource2ServerConfig, ISource2ServerConfig, SOURCE2SERVERCONFIG_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, g_pSource2GameEntities, ISource2GameEntities, SOURCE2GAMEENTITIES_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, g_pSource2GameClients, IServerGameClients, SOURCE2GAMECLIENTS_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, g_pNetworkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, g_pNetworkMessages, INetworkMessages, NETWORKMESSAGES_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);

	SH_ADD_HOOK_MEMFUNC(ICvar, DispatchConCommand, g_pCVar, this, &CS2StatTrack::Hook_DispatchConCommand, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameServerSteamAPIActivated, g_pSource2Server, this, &CS2StatTrack::Hook_GameServerSteamAPIActivated, false);

	ConVar_Register(FCVAR_RELEASE | FCVAR_CLIENT_CAN_EXECUTE | FCVAR_GAMEDLL);
	return true;
}

bool CS2StatTrack::Unload(char *error, size_t maxlen)
{
	return true;
}

bool CS2StatTrack::Pause(char *error, size_t maxlen)
{
	return true;
}

bool CS2StatTrack::Unpause(char *error, size_t maxlen)
{
	return true;
}

// --- Hooks

void CS2StatTrack::Hook_GameServerSteamAPIActivated()
{
	if (!GetGCHelper().Init())
	{
		Panic("!GetGCHelper().Init()");
	}
	else
	{
		Message("Steam api intialized!\n");
	}

	RETURN_META(MRES_IGNORED);
}

void CS2StatTrack::Hook_DispatchConCommand(ConCommandHandle cmdHandle, const CCommandContext &ctx, const CCommand &args)
{
	RETURN_META(MRES_IGNORED);
}

// --- Information
const char *CS2StatTrack::GetLicense()
{
	return "GPL v3 License";
}

const char *CS2StatTrack::GetVersion()
{
	return "0.0.1";
}

const char *CS2StatTrack::GetDate()
{
	return __DATE__;
}

const char *CS2StatTrack::GetLogTag()
{
	return "CS2StatTrack";
}

const char *CS2StatTrack::GetAuthor()
{
	return "See README for thanks";
}

const char *CS2StatTrack::GetDescription()
{
	return "";
}

const char *CS2StatTrack::GetName()
{
	return "CS2StatTrack";
}

const char *CS2StatTrack::GetURL()
{
	return "";
}

// commands

void Say(const char *msg)
{
	char buf[1024] = {};
	V_snprintf(buf, sizeof(buf) - 1, "say %s", msg);
	g_pEngineServer2->ServerCommand(buf);
}

CON_COMMAND_F(st_increment, "<`listplayers`-> id>, <uint64 -> itemID>, <int -> event type>, <int -> amount>", FCVAR_CLIENT_CAN_EXECUTE)
{
	CPlayerSlot killer = context.GetPlayerSlot();
	CPlayerSlot victim = {atoi(args.Arg(1))};
	long itemId = atol(args.Arg(2));
	int type = std::max(atoi(args.Arg(3)), 0);
	int amount = std::max(atoi(args.Arg(4)), 1);

	if ((victim.Get() == 0) || (itemId == 0))
	{
		ConMsg("Bailing, (%s || %s) == 0\n", "victim", "itemId");
		Say("Invalid victim or itemID, bailing.");
		Say("Usage: `st_increment victimPlayerId itemID eventType amount`");

		RETURN_META(MRES_IGNORED);
	}

	auto killerSteamId = g_pEngineServer2->GetClientSteamID(killer);
	auto victimSteamId = g_pEngineServer2->GetClientSteamID(victim);

	if (killerSteamId == NULL || victimSteamId == NULL)
	{
		Say("killerSteamId == NULL || victimSteamId == NULL, bailing");

		RETURN_META(MRES_IGNORED);
	}

	ExecuteIncrement(killerSteamId->GetAccountID(), victimSteamId->GetAccountID(), itemId, type, amount);
};

inline void ExecuteIncrement(uint32_t killer, uint32_t victim, uint64_t itemID, int type, int amount)
{
	GCHelper GameCoordinator = GetGCHelper();

	if (!(GameCoordinator.BInited()))
	{
		return;
	}

	CMsgIncrementKillCountAttribute msg;
	msg.set_killer_account_id(killer);
	msg.set_victim_account_id(victim);
	msg.set_item_id(itemID);
	msg.set_event_type(0);
	msg.set_amount(amount);

	auto result = GameCoordinator.SendMessageToGC(EGCItemMsg::k_EMsgGC_IncrementKillCountAttribute, msg);

	Message("GameCoordinator.send(...) = %s", result ? "true" : "false");
	ConMsg("increment(...) | ");
	ConMsg("%" PRIu32 "\t", killer);
	ConMsg("%" PRIu32 "\t", victim);
	ConMsg("%" PRIu64 "\t", itemID);
	ConMsg("%d\t", type);
	ConMsg("%d\n", amount);

	if (result)
	{
		Say("Sent protobuf to gamecoordinator.");
	}
}