// TrailMap - Plugin lifecycle
// Union SOURCE file
#include "resource.h"

namespace GOTHIC_ENGINE {

    void Game_Entry() {
    }

    void Game_Init() {
        trailMap = std::make_unique<TrailMap>();
        trailMap->Init();
    }

    void Game_Exit() {
        trailMap.release();
    }

    void Game_PreLoop() {
        trailMap->GamePreLoop();
    }

    void Game_Loop() {
        trailMap->GameLoop();
    }

    void Game_PostLoop() {
        trailMap->GamePostLoop();
    }

    void Game_MenuLoop() {
    }

    // Information about current saving or loading world
    TSaveLoadGameInfo& SaveLoadGameInfo = UnionCore::SaveLoadGameInfo;

    void Game_SaveBegin() {
        // Set the current save slot from SaveLoadGameInfo
        if (trailMap) {
            trailMap->saveSlot = SaveLoadGameInfo.slotID;
        }
    }

    void Game_SaveEnd() {
        // Save trail data for the current save slot
        if (trailMap && trailMap->enabled && trailMap->saveSlot >= 0) {
            trailMap->SaveData(trailMap->saveSlot);
        }
    }

    void LoadBegin() {
    }

    void LoadEnd() {
    }

    void Game_LoadBegin_NewGame() {
        LoadBegin();
        // New game - clear all trail data, assign slot 0
        if (trailMap) {
            trailMap->ClearAllData();
            trailMap->saveSlot = 0;
        }
    }

    void Game_LoadEnd_NewGame() {
        LoadEnd();
    }

    void Game_LoadBegin_SaveGame() {
        LoadBegin();
        // Loading a save - read slot from SaveLoadGameInfo and load data
        if (trailMap) {
            trailMap->saveSlot = SaveLoadGameInfo.slotID;
            trailMap->LoadData(trailMap->saveSlot);  // LoadData already clears old data
        }
    }

    void Game_LoadEnd_SaveGame() {
        LoadEnd();
    }

    void Game_LoadBegin_ChangeLevel() {
        LoadBegin();
    }

    void Game_LoadEnd_ChangeLevel() {
        LoadEnd();
    }

    void Game_LoadBegin_Trigger() {
    }

    void Game_LoadEnd_Trigger() {
    }

    void Game_Pause() {
    }

    void Game_Unpause() {
    }

    void Game_DefineExternals() {
    }

    void Game_ApplyOptions() {
        if (trailMap) {
            trailMap->ReadOptions();
        }
    }

#define AppDefault True
    CApplication* lpApplication = !CHECK_THIS_ENGINE ? Null : CApplication::CreateRefApplication(
        Enabled(AppDefault) Game_Entry,
        Enabled(AppDefault) Game_Init,
        Enabled(AppDefault) Game_Exit,
        Enabled(AppDefault) Game_PreLoop,
        Enabled(AppDefault) Game_Loop,
        Enabled(AppDefault) Game_PostLoop,
        Enabled(AppDefault) Game_MenuLoop,
        Enabled(AppDefault) Game_SaveBegin,
        Enabled(AppDefault) Game_SaveEnd,
        Enabled(AppDefault) Game_LoadBegin_NewGame,
        Enabled(AppDefault) Game_LoadEnd_NewGame,
        Enabled(AppDefault) Game_LoadBegin_SaveGame,
        Enabled(AppDefault) Game_LoadEnd_SaveGame,
        Enabled(AppDefault) Game_LoadBegin_ChangeLevel,
        Enabled(AppDefault) Game_LoadEnd_ChangeLevel,
        Enabled(AppDefault) Game_LoadBegin_Trigger,
        Enabled(AppDefault) Game_LoadEnd_Trigger,
        Enabled(AppDefault) Game_Pause,
        Enabled(AppDefault) Game_Unpause,
        Enabled(AppDefault) Game_DefineExternals,
        Enabled(AppDefault) Game_ApplyOptions
    );
}