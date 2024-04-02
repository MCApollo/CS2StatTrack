#ifndef A7FDCB08_57BC_496B_8C8B_5CED5734EC48
#define A7FDCB08_57BC_496B_8C8B_5CED5734EC48

#define META_IS_SOURCE2 1

#include <ISmmPlugin.h>
#include "igameevents.h"
#include <iplayerinfo.h>
#include <sh_vector.h>
#include "networksystem/inetworkserializer.h"
#include <iserver.h>

class CS2StatTrack : public ISmmPlugin, public IMetamodListener
{
    public: // implements ISmmPlugin
        bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
        bool Unload(char *error, size_t maxlen);
        bool Pause(char *error, size_t maxlen);
        bool Unpause(char *error, size_t maxlen);
    public: // hooks
        void Hook_GameServerSteamAPIActivated();
        void Hook_DispatchConCommand(ConCommandHandle cmdHandle, const CCommandContext &ctx, const CCommand &args);

        void OnEntitySpawned(CEntityInstance *pEntity);

    public: // plugin information
        const char *GetAuthor();
        const char *GetName();
        const char *GetDescription();
        const char *GetURL();
        const char *GetLicense();
        const char *GetVersion();
        const char *GetDate();
        const char *GetLogTag();
};

extern CS2StatTrack g_CS2StatTrack;

PLUGIN_GLOBALVARS();

void ExecuteIncrement(uint32_t killer, uint32_t victim, uint64_t itemID, int type, int amount);

#endif /* A7FDCB08_57BC_496B_8C8B_5CED5734EC48 */
