#pragma once

#include <GWCA\Utilities\Hook.h>

#include <GWCA\Packets\StoC.h>

#include <Utils/GuiUtils.h>
#include <ToolboxWindow.h>
#include <Modules/HallOfMonumentsModule.h>

class InfoWindow : public ToolboxWindow {
    InfoWindow() = default;
    ~InfoWindow() = default;

public:
    static InfoWindow& Instance() {
        static InfoWindow instance;
        return instance;
    }

    const char* Name() const override { return "Info"; }
    const char* Icon() const override { return ICON_FA_INFO_CIRCLE; }

    void Initialize() override;
    void Terminate() override;

    void Draw(IDirect3DDevice9* pDevice) override;
    void Update(float delta) override;

    void DrawSettingInternal() override;
    void LoadSettings(CSimpleIni* ini) override;
    void SaveSettings(CSimpleIni* ini) override;


    static void CmdResignLog(const wchar_t* cmd, int argc, wchar_t** argv);
    static void OnInstanceLoad(GW::HookStatus*, GW::Packet::StoC::InstanceLoadFile*);

private:

    void DrawResignlog();

    void DrawSkillInfo(GW::Skill* skill, GuiUtils::EncString* name, bool force_advanced = false);
    void DrawItemInfo(GW::Item* item, GuiUtils::EncString* name, bool force_advanced = false);
    void DrawAgentInfo(GW::Agent* agent);
    void DrawGuildInfo(GW::Guild* guild);
    void DrawHomAchievements(const GW::Player* player);
    DWORD mapfile = 0;

    std::map<std::wstring,HallOfMonumentsAchievements*> target_achievements;

    clock_t send_timer = 0;

    uint32_t quoted_item_id = 0;

    bool show_widgets = true;
    bool show_open_chest = true;
    bool show_player = true;
    bool show_target = true;
    bool show_map = true;
    bool show_dialog = true;
    bool show_item = true;
    bool show_mobcount = true;
    bool show_quest = true;
    bool show_resignlog = true;

    GW::HookEntry MessageCore_Entry;
    GW::HookEntry InstanceLoadFile_Entry;
    GW::HookEntry OnDialogBody_Entry;
    GW::HookEntry OnDialogButton_Entry;
    GW::HookEntry OnSendDialog_Entry;
};
