#pragma once

#include <GWCA/Managers/UIMgr.h>

#include <ToolboxModule.h>

namespace GuiUtils {
    class EncString;
}
namespace GW {
    namespace UI {
        struct DialogButtonInfo;
    }
}
struct DialogButton;

class DialogModule : public ToolboxModule {
    DialogModule() = default;

public:
    static DialogModule& Instance() {
        static DialogModule instance;
        return instance;
    }

    const char* Name() const override { return "Dialogs"; }

    void Initialize() override;
    void Terminate() override;
    void Update(float) override;
    static const wchar_t* GetDialogBody();
    static void SendDialog(uint32_t dialog_id);
    static void SendDialogs(std::initializer_list<uint32_t> dialog_ids);
    static uint32_t GetDialogAgent();
    static const std::vector<GW::UI::DialogButtonInfo*>& GetDialogButtons();
    static const std::vector<GuiUtils::EncString*>& GetDialogButtonMessages();
    // Find and take the first available quest from the current dialog. Returns quest_id requested.
    static uint32_t AcceptFirstAvailableQuest();

private:
    // private method to queue multiple dialogs with the same timestamp
    static void SendDialog(uint32_t dialog_id, clock_t time);

    enum class QuestDialogType {
        TAKE = 0x800001,
        ENQUIRE = 0x800003,
        ENQUIRE_NEXT = 0x800004,
        ENQUIRE_REWARD = 0x800006,
        REWARD = 0x800007
    };
    static bool IsQuest(const uint32_t dialog_id) {
        return (dialog_id & 0x800000) != 0;
    }
    static uint32_t GetQuestID(const uint32_t dialog_id) {
        return (dialog_id ^ 0x800000) >> 8;
    }
    static uint32_t SetQuestDialogType(const uint32_t quest_id, QuestDialogType type) {
        return (quest_id << 8) | (uint32_t)type;
    }
    static QuestDialogType GetQuestDialogType(const uint32_t dialog_id) {
        return static_cast<QuestDialogType>(dialog_id & 0xf0000f);
    }
    static bool IsUWTele(const uint32_t dialog_id) {
        switch (dialog_id) {
            case GW::Constants::DialogID::UwTeleLab:
            case GW::Constants::DialogID::UwTeleVale:
            case GW::Constants::DialogID::UwTelePits:
            case GW::Constants::DialogID::UwTelePools:
            case GW::Constants::DialogID::UwTelePlanes:
            case GW::Constants::DialogID::UwTeleWastes:
            case GW::Constants::DialogID::UwTeleMnt: return true;
            default: return false;
        }
    }
    static void OnDialogSent(uint32_t dialog_id);
    static void OnPostUIMessage(GW::HookStatus* status, GW::UI::UIMessage message_id, void* wparam, void*);
    static void OnPreUIMessage(GW::HookStatus* status, GW::UI::UIMessage message_id, void* wparam, void*);
};
