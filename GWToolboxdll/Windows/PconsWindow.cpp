#include "stdafx.h"

#include <GWCA/Constants/Constants.h>

#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Skill.h>

#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/EffectMgr.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Managers/StoCMgr.h>

#include <Color.h>
#include <Logger.h>
#include <Utils/GuiUtils.h>
#include <Utils/TextUtils.h>
#include <Widgets/AlcoholWidget.h>
#include <Windows/HotkeysWindow.h>
#include <Windows/MainWindow.h>
#include <Windows/PconsWindow.h>

using namespace GW::Constants;

bool Pcon::map_has_effects_array = false;

namespace {
    GW::HookEntry ChatCmd_HookEntry;

    PconAlcohol* pcon_alcohol = nullptr;
    clock_t scan_inventory_timer = 0;
    bool enabled = false;

    // Interface Settings
    bool tick_with_pcons = false;
    int items_per_row = 3;
    bool show_enable_button = true;

    bool disable_pcons_on_map_change = false;
    bool disable_cons_on_vanquish_completion = true;
    bool disable_cons_on_dungeon_completion = true;
    bool disable_cons_on_mission_completion = true;
    bool disable_cons_on_objective_completion = false;
    bool disable_cons_in_final_room = false;

    bool show_auto_refill_pcons_tickbox = true;
    bool show_auto_disable_pcons_tickbox = false;

    GW::Agent* player = nullptr;

    // Pcon Settings
    // todo: tonic pop?
    // todo: morale / dp removal
    GW::HookEntry AgentSetPlayer_Entry;
    GW::HookEntry AddExternalBond_Entry;
    GW::HookEntry PostProcess_Entry;
    GW::HookEntry GenericValue_Entry;
    GW::HookEntry AgentState_Entry;
    GW::HookEntry SpeechBubble_Entry;
    GW::HookEntry ObjectiveDone_Entry;
    GW::HookEntry VanquishComplete_Entry;

    MapID map_id = MapID::None;
    InstanceType instance_type = InstanceType::Loading;
    InstanceType previous_instance_type = InstanceType::Loading;
    bool in_vanquishable_area = false;

    bool elite_area_disable_triggered = false; // Already triggered in this run?
    clock_t elite_area_check_timer = 0;

    // Map of which objectives to check per map_id
    std::vector<DWORD> objectives_complete = {};
    const std::map<MapID, std::vector<DWORD>>
    objectives_to_complete_by_map_id = {
        {MapID::The_Fissure_of_Woe, {309, 310, 311, 312, 313, 314, 315, 316, 317, 318, 319}}, // Can be done in any order - check them all.
        {MapID::The_Deep, {421}},
        {MapID::Urgozs_Warren, {357}},
        {MapID::The_Underworld, {157}} // Only need to check for Nightman Cometh for Underworld.
    };
    std::vector<DWORD> current_objectives_to_check = {};

    // Map of which locations to turn off near by map_id e.g. Kanaxai, Urgoz
    const std::map<MapID, GW::Vec2f>
    final_room_location_by_map_id = {
        {MapID::The_Deep, GW::Vec2f(30428.0f, -5842.0f)},     // Rough location of Kanaxai
        {MapID::Urgozs_Warren, GW::Vec2f(-2800.0f, 14316.0f)} // Front entrance of Urgoz's room
    };
    GW::Vec2f current_final_room_location = GW::Vec2f(0, 0);

    const char* disable_cons_on_objective_completion_hint = "Disable cons when final objective(s) completed";
    const char* disable_cons_in_final_room_hint = "Disable cons when reaching the final room in Urgoz and Deep";
    const char* disable_cons_on_vanquish_completion_hint = "Disable cons when completing a vanquish";

    constexpr std::initializer_list<std::wstring_view> air_of_superiority_messages = {
        L"\x8102\x2399", // Knowledge is power!
        L"\x8102\x239a", // You're no match for my brains!
        L"\x8102\x239b", // Is there anything I can't do?
        L"\x8102\x239c", // Kneel before your master!
        L"\x8102\x239d", // Buwahaha!
    };

    constexpr std::initializer_list<std::wstring_view> drunk_messages = {
        L"\x8CA\xA4F7\xF552\xA32",   // i love you man!
        L"\x8CB\xE20B\x9835\x4C75",  // I'm the king of the world!
        L"\x8CC\xFA4D\xF068\x393",   // I think I need to sit down
        L"\x8CD\xF2C2\xBBAD\x1EAD",  // I think I'm gonna be sick
        L"\x8CE\x85E5\xF726\x68B1",  // Oh no, not again
        L"\x8CF\xEDD3\xF2B9\x3F34",  // It's spinning...
        L"\x8D0\xF056\xE7AD\x7EE6",  // Everyone stop shouting!
        L"\x8101\x6671\xCBF8\xE717", // "BE GONE!"
        L"\x8101\x6672\xB0D6\xCE2F", // "Soon you will all be crushed."
        L"\x8101\x6673\xDAA5\xD0A1", // "You are no match for my almighty power."
        L"\x8101\x6674\x8BF9\x8C19", // "Such fools to think you can attack me here. Come closer so you can see the face of your doom!"
        L"\x8101\x6675\x996D\x87BA", // "No one can stop me, let alone you puny mortals!"
        L"\x8101\x6676\xBAFA\x8E15", // "You are messing with affairs that are beyond your comprehension. Leave now and I may let you live!"
        L"\x8101\x6677\xA186\xF84C", // "His blood has blocked = trueed me to my mortal body."
        L"\x8101\x6678\xD2ED\xE693", // "I have blocked = trueed!"
        L"\x8101\x6679\xA546\xF24A", // "Abaddon will feast on your eyes!"
        L"\x8101\x667A\xB477\xA79A", // "Abaddon's sword has been drawn. He sends me back to you with tokens of renewed power!"
        L"\x8101\x667B\x8FBB\xC739", // "Are you the Keymaster?"
        L"\x8101\x667C\xFE50\xC173", // "Human sacrifice. Dogs and cats living together. Mass hysteria!"
        L"\x8101\x667D\xBBC6\xAC9E", // "Take me now, subcreature."'
        L"\x8101\x667E\xCD71\xDEE3", // "We must prepare for the coming of Banjo the Clown, God of Puppets."
        L"\x8101\x667F\xE823\x9435", // "This house is clean."
        L"\x8101\x6680\x82FC\xDCEC",
        L"\x8101\x6681\xC86C\xB975", // "Mommy? Where are you? I can't find you. I can't. I'm afraid of the light, mommy. I'm afraid of the light."
        L"\x8101\x6682\xE586\x9311", // "Get away from my baby!"
        L"\x8101\x6683\xA949\xE643", // "This house has many hearts."'
        L"\x8101\x6684\xB765\x93F1", // "As a boy I spent much time in these lands."
        L"\x8101\x6685\xEDE0\xAF1D", // "I see dead people."
        L"\x8101\x6686\xD356\xDC69", // "Do you like my fish balloon? Can you hear it singing to you...?"
        L"\x8101\x6687\xEA3C\x96F0", // "4...Itchy...Tasty..."
        L"\x8101\x6688\xCBDD\xB1CF", // "Gracious me, was I raving? Please forgive me. I'm mad."
        L"\x8101\x6689\xE770\xEEA4", // "Keep away. The sow is mine."
        L"\x8101\x668A\x885F\xE61D", // "All is well. I'm not insane."
        L"\x8101\x668B\xCCDD\x88AA", // "I like how they've decorated this place. The talking lights are a nice touch."
        L"\x8101\x668C\x8873\x9A16", // "There's a reason there's a festival ticket in my ear. I'm trying to lure the evil spirits out of my head."
        L"\x8101\x668D\xAF68\xF84A", // "And this is where I met the Lich. He told me to burn things."
        L"\x8101\x668E\xFE43\x9CB3", // "When I grow up, I want to be a principal or a caterpillar."
        L"\x8101\x668F\xDAFF\x903E", // "Oh boy, sleep! That's where I'm a Luxon."
        L"\x8101\x6690\xA1F5\xD15F", // "My cat's breath smells like cat food."
        L"\x8101\x6691\xAE54\x8EC6", // "My cat's name is Mittens."
        L"\x8101\x6692\xDFBB\xD674", // "Then the healer told me that BOTH my eyes were lazy. And that's why it was the best summer ever!"
        L"\x8101\x6693\xAC9F\xDCBE", // "Go, banana!"
        L"\x8101\x6694\x9ACA\xC746", // "It's a trick. Get an axe."
        L"\x8101\x6695\x8ED8\xD572", // "Klaatu...barada...necktie?"
        L"\x8101\x6696\xE883\xFED7", // "You're disgusting, but I love you!"
        L"\x8101\x68BA\xA875\xA785", // "Cross over, children. All are welcome. All welcome. Go into the light. There is peace and serenity in the light."
        L"\x8102\x4939\xD402\x99F3", // start grog messages
        L"\x1FAA\xD6B1\x8599\x7B2D",
        L"\x1FB2\xD54E\x9029\x151A",
        L"\x1FAB\xFCDD\xA466\x3243",
        L"\x1FB3\xB822\xF276\x2B0C",
        L"\x1FAC\xBC27\xAC6B\x32C2",
        L"\x1FAD\xA6B8\xC903\x5EAE",
        L"\x8102\x4918\xCA53\xE82F",
        L"\x1FB4\xFF89\xC048\x17A6",
        L"\x1FB5\xE225\x97D1\x122C",
        L"\x1FAE\xD24E\x9E99\x6D3 ",
        L"\x8102\x491A\x9B62\xF12C",
        L"\x1FAF\x8265\xD9B0\x300D",
        L"\x1FB6\xBD6A\x88D1\x4F59",
        L"\x8102\x493B\x96C7\x8B03",
        L"\x8102\x4929\xA216\xC64B",
        L"\x8103\xAC8\xD5E7\x951E",
        L"\x8103\xA6C\xDAFC\xDC30",
        L"\x8102\x4916\x9FC6\x913A",
        L"\x8102\x4930\xEB29\xA9A1",
        L"\x8102\x4938\xC1E8\xC1E7",
        L"\x8102\x4932\x8516\xBF4C",
        L"\x8102\x4936\xB245\xCA89",
        L"\x8102\x4942\xA195\xF718",
        L"\x8102\x4927\x8AA7\xD5AB",
        L"\x8102\x492F\xA315\x9062",
        L"\x8103\xA90\xD7A3\x9B78",
        L"\x8102\x4933\x82F9\xEE62",
        L"\x8102\x492D\xAE7F\x9205",
        L"\x8102\x4924\x9EED\xED02",
        L"\x8102\x4931\xEA9D\xD3D5",
        L"\x8102\x4921\xA36D\xCE7A",
        L"\x8102\x4937\xACEA\x8B90",
        L"\x8102\x4928\xC8E9\x8953", // the ship, it's spinning
        L"\x8102\x4919\xBC36\xB446",
        L"\x8102\x492B\xC39F\xD6FA",
        L"\x8102\x4923\xAA60\x9F98", // end grog messages
    };


    PconsWindow& Instance()
    {
        return PconsWindow::Instance();
    }

    void CheckObjectivesCompleteAutoDisable()
    {
        if (!enabled || elite_area_disable_triggered || instance_type != InstanceType::Explorable) {
            return; // Pcons disabled, auto disable already triggered, or not in explorable area.
        }
        if (!disable_cons_on_objective_completion || objectives_complete.empty() || current_objectives_to_check.empty()) {
            return; // No objectives complete, or no objectives to check for this map.
        }
        bool objective_complete = false;
        for (size_t i = 0; i < current_objectives_to_check.size(); i++) {
            objective_complete = false;
            for (size_t j = 0; j < objectives_complete.size() && !objective_complete; j++) {
                objective_complete = current_objectives_to_check.at(i) == objectives_complete.at(j);
            }
            if (!objective_complete) {
                return; // Not all objectives complete.
            }
        }
        if (objective_complete) {
            elite_area_disable_triggered = true;
            Instance().SetEnabled(false);
            Log::Flash("Cons auto-disabled on completion");
        }
    }

    GW::HookEntry OnUIMessage_HookEntry;

    void OnUIMessage(GW::HookStatus* status, GW::UI::UIMessage message_id, void* wparam, void*)
    {
        switch (message_id) {
            case GW::UI::UIMessage::kAgentSpeechBubble: {
                const auto packet = (GW::UI::UIPacket::kAgentSpeechBubble*)wparam;
                const std::wstring_view msg{packet->message, 4};
                if (PconAlcohol::suppress_drunk_text && std::ranges::contains(drunk_messages, msg))
                    status->blocked = true;
                if (PconAlcohol::suppress_air_of_superiority_text && std::ranges::contains(air_of_superiority_messages, msg))
                    status->blocked = true;
            }
            break;
            case GW::UI::UIMessage::kVanquishComplete:
                if (enabled && disable_cons_on_vanquish_completion)
                    Instance().SetEnabled(false);
                break;
            case GW::UI::UIMessage::kDungeonComplete:
                if (enabled && disable_cons_on_dungeon_completion)
                    Instance().SetEnabled(false);
                break;
            case GW::UI::UIMessage::kMissionComplete:
                if (enabled && disable_cons_on_mission_completion)
                    Instance().SetEnabled(false);
                break;
            case GW::UI::UIMessage::kObjectiveComplete: {
                const auto packet = (GW::UI::UIPacket::kObjectiveComplete*)wparam;
                objectives_complete.push_back(packet->objective_id);
                CheckObjectivesCompleteAutoDisable();
            }
            break;
            case GW::UI::UIMessage::kPostProcessingEffect: {
                auto packet = (GW::UI::UIPacket::kPostProcessingEffect*)wparam;
                PconAlcohol::alcohol_level = (uint32_t)(packet->amount * 5.f);
                // printf("Level = %d, tint = %d\n", pak->level, pak->tint);
                if (enabled) {
                    pcon_alcohol->Update();
                }
                if (PconAlcohol::suppress_drunk_effect) {
                    packet->amount = 0.f;
                }
            }
            break;
            case GW::UI::UIMessage::kEffectAdd: {
                auto packet = (GW::UI::UIPacket::kEffectAdd*)wparam;
                if (PconAlcohol::suppress_lunar_skills
                    && (packet->effect->skill_id == SkillID::Spiritual_Possession || packet->effect->skill_id == SkillID::Lucky_Aura)) {
                    status->blocked = true;
                }
            }
            break;
        }
    }
}

PconsWindow::PconsWindow()
{
    constexpr float s = 64.0f; // all icons are 64x64

    pcons.push_back(new PconCons("Essence of Celerity", "Essence", "essence", L"Essence of Celerity",
                                 ImVec2(5 / s, 10 / s), ImVec2(46 / s, 51 / s),
                                 ItemID::ConsEssence, SkillID::Essence_of_Celerity_item_effect, 5));

    pcons.push_back(new PconCons("Grail of Might", "Grail", "grail", L"Grail of Might",
                                 ImVec2(5 / s, 12 / s), ImVec2(49 / s, 56 / s),
                                 ItemID::ConsGrail, SkillID::Grail_of_Might_item_effect, 5));

    pcons.push_back(new PconCons("Armor of Salvation", "Armor", "armor", L"Armor of Salvation",
                                 ImVec2(0 / s, 2 / s), ImVec2(56 / s, 58 / s),
                                 ItemID::ConsArmor, SkillID::Armor_of_Salvation_item_effect, 5));

    pcons.push_back(new PconGeneric("Red Rock Candy", "Red Rock", "redrock", L"Red Rock Candy",
                                    ImVec2(0 / s, 4 / s), ImVec2(52 / s, 56 / s),
                                    ItemID::RRC, SkillID::Red_Rock_Candy_Rush, 5));

    pcons.push_back(new PconGeneric("Blue Rock Candy", "Blue Rock", "bluerock", L"Blue Rock Candy",
                                    ImVec2(0 / s, 4 / s), ImVec2(52 / s, 56 / s),
                                    ItemID::BRC, SkillID::Blue_Rock_Candy_Rush, 10));

    pcons.push_back(new PconGeneric("Green Rock Candy", "Green Rock", "greenrock", L"Green Rock Candy",
                                    ImVec2(0 / s, 4 / s), ImVec2(52 / s, 56 / s),
                                    ItemID::GRC, SkillID::Green_Rock_Candy_Rush, 15));

    pcons.push_back(new PconGeneric("Golden Egg", "Egg", "egg", L"Golden Egg",
                                    ImVec2(1 / s, 8 / s), ImVec2(48 / s, 55 / s),
                                    ItemID::Eggs, SkillID::Golden_Egg_skill, 20));

    pcons.push_back(new PconGeneric("Candy Apple", "Apple", "apple", L"Candy Apple",
                                    ImVec2(0 / s, 7 / s), ImVec2(50 / s, 57 / s),
                                    ItemID::Apples, SkillID::Candy_Apple_skill, 10));

    pcons.push_back(new PconGeneric("Candy Corn", "Corn", "corn", L"Candy Corn",
                                    ImVec2(5 / s, 10 / s), ImVec2(48 / s, 53 / s),
                                    ItemID::Corns, SkillID::Candy_Corn_skill, 10));

    pcons.push_back(new PconGeneric("Birthday Cupcake", "Cupcake", "cupcake", L"Birthday Cupcake",
                                    ImVec2(1 / s, 5 / s), ImVec2(51 / s, 55 / s),
                                    ItemID::Cupcakes, SkillID::Birthday_Cupcake_skill, 10));

    pcons.push_back(new PconGeneric("Slice of Pumpkin Pie", "Pie", "pie", L"Slice of Pumpkin Pie",
                                    ImVec2(0 / s, 7 / s), ImVec2(52 / s, 59 / s),
                                    ItemID::Pies, SkillID::Pie_Induced_Ecstasy, 10));

    pcons.push_back(new PconGeneric("War Supplies", "War Supply", "warsupply", L"War Supplies",
                                    ImVec2(0 / s, 0 / s), ImVec2(63 / s, 63 / s),
                                    ItemID::Warsupplies, SkillID::Well_Supplied, 20));

    pcons.push_back(pcon_alcohol = new PconAlcohol("Alcohol", "Alcohol", "alcohol", L"Dwarven Ale",
                                                   ImVec2(-5 / s, 1 / s), ImVec2(57 / s, 63 / s),
                                                   10));

    pcons.push_back(new PconLunar("Lunar Fortunes", "Lunars", "lunars", L"Lunar Fortune",
                                  ImVec2(1 / s, 4 / s), ImVec2(56 / s, 59 / s),
                                  10));

    pcons.push_back(new PconCity("City speedboost", "City IMS", "city", L"Sugary Blue Drink",
                                 ImVec2(0 / s, 1 / s), ImVec2(61 / s, 62 / s),
                                 20));

    pcons.push_back(new PconGeneric("Drake Kabob", "Kabob", "kabob", L"Drake Kabob",
                                    ImVec2(0 / s, 0 / s), ImVec2(64 / s, 64 / s),
                                    ItemID::Kabobs, SkillID::Drake_Skin, 10));

    pcons.push_back(new PconGeneric("Bowl of Skalefin Soup", "Soup", "soup", L"Bowl of Skalefin Soup",
                                    ImVec2(2 / s, 5 / s), ImVec2(51 / s, 54 / s),
                                    ItemID::SkalefinSoup, SkillID::Skale_Vigor, 10));

    pcons.push_back(new PconGeneric("Pahnai Salad", "Salad", "salad", L"Pahnai Salad",
                                    ImVec2(0 / s, 5 / s), ImVec2(49 / s, 54 / s),
                                    ItemID::PahnaiSalad, SkillID::Pahnai_Salad_item_effect, 10));

    pcons.push_back(new PconScroll("XP scroll", "XP scroll", "scroll", L"Scroll of Hunter's Insight",
                                   {0, 0}, {1, 1}, 20));

    // Refillers

    pcons.push_back(new PconRefiller(L"Scroll of Resurrection", ItemID::ResScrolls, 5));
    pcons.back()->ini = "resscroll";
    pcons.push_back(new PconRefiller(L"Lockpick", ItemID::Lockpick, 50));
    pcons.back()->ini = "lockpick";
    pcons.push_back(new PconRefiller(L"Powerstone of Courage", ItemID::Powerstone, 5));
    pcons.back()->ini = "pstone";
    pcons.push_back(new PconRefiller(L"Seal of the Dragon Empire", ItemID::SealOfTheDragonEmpire, 5));

    pcons.push_back(new PconRefiller(L"Honeycomb", ItemID::Honeycomb));
    pcons.push_back(new PconRefiller(L"Pumpkin Cookie", ItemID::PumpkinCookie));
    pcons.push_back(new PconRefiller(L"Ghost in the Box", ItemID::GhostInTheBox));
    pcons.push_back(new PconRefiller(L"Everlasting Mobstopper", ItemID::Mobstopper));

    // Summoning Stones
    pcons.push_back(new PconRefiller(L"Tengu Support Flare", ItemID::TenguSummon));
    pcons.push_back(new PconRefiller(L"Imperial Guard Reinforcement Order", ItemID::ImperialGuardSummon));
    pcons.push_back(new PconRefiller(L"Shining Blade War Horn", ItemID::WarhornSummon));
    pcons.push_back(new PconRefiller(L"Ghastly Summoning Stone", ItemID::GhastlyStone));
    // @Cleanup: Add these items to GWCA or something
    pcons.push_back(new PconRefiller(L"Chitinous Summoning Stone", 30959));
    pcons.push_back(new PconRefiller(L"Amber Summoning Stone", 30961));
    pcons.push_back(new PconRefiller(L"Arctic Summoning Stone", 30962));
    pcons.push_back(new PconRefiller(L"Celestial Summoning Stone", 34176));
    pcons.push_back(new PconRefiller(L"Mystical Summoning Stone", 30960));
    pcons.push_back(new PconRefiller(L"Demonic Summoning Stone", 30963));
    pcons.push_back(new PconRefiller(L"Gelatinous Summoning Stone", 30964));
    pcons.push_back(new PconRefiller(L"Fossilized Summoning Stone", 30965));
    pcons.push_back(new PconRefiller(L"Jadeite Summoning Stone", 30966));
    pcons.push_back(new PconRefiller(L"Mischievous Summoning Stone", 31022));
    pcons.push_back(new PconRefiller(L"Frosty Summoning Stone", 31023));
    pcons.push_back(new PconRefiller(L"Mercantile Summoning Stone", 31154));
    pcons.push_back(new PconRefiller(L"Mysterious Summoning Stone", 31155));
    pcons.push_back(new PconRefiller(L"Zaishen Summoning Stone", 31156));
}

void PconsWindow::Initialize()
{
    ToolboxWindow::Initialize();
    AlcoholWidget::Instance().Initialize(); // Pcons depend on alcohol widget to track current drunk level.

    const GW::UI::UIMessage ui_messages[] = {
        GW::UI::UIMessage::kAgentSpeechBubble,
        GW::UI::UIMessage::kVanquishComplete,
        GW::UI::UIMessage::kDungeonComplete,
        GW::UI::UIMessage::kMissionComplete,
        GW::UI::UIMessage::kPostProcessingEffect,
        GW::UI::UIMessage::kObjectiveComplete,
        GW::UI::UIMessage::kEffectAdd
    };
    for (auto message_id : ui_messages) {
        GW::UI::RegisterUIMessageCallback(&OnUIMessage_HookEntry, message_id, OnUIMessage);
    }

    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::GenericValue>(&GenericValue_Entry, &OnGenericValue);
    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::AgentState>(&AgentState_Entry, &OnAgentState);
    GW::Chat::CreateCommand(&ChatCmd_HookEntry, L"pcons", &CmdPcons);
}

void PconsWindow::Terminate()
{
    ToolboxWindow::Terminate();
    GW::UI::RemoveUIMessageCallback(&OnUIMessage_HookEntry);
    for (Pcon* pcon : pcons) {
        delete pcon;
    }
    pcons.clear();
    GW::Chat::DeleteCommand(&ChatCmd_HookEntry);
}

void PconsWindow::OnAddExternalBond(GW::HookStatus* status, const GW::Packet::StoC::AddExternalBond* pak)
{
    if (PconAlcohol::suppress_lunar_skills
        && pak->caster_id == GW::Agents::GetObservingId()
        && pak->receiver_id == 0
        && (pak->skill_id == static_cast<DWORD>(SkillID::Spiritual_Possession) || pak->skill_id == static_cast<DWORD>(SkillID::Lucky_Aura))) {
        // printf("blocked skill %d\n", pak->skill_id);
        status->blocked = true;
    }
}

void PconsWindow::OnGenericValue(GW::HookStatus*, GW::Packet::StoC::GenericValue* pak)
{
    if (PconAlcohol::suppress_drunk_emotes && pak->agent_id == GW::Agents::GetObservingId() && pak->value_id == 22) {
        if (pak->value == 0x33E807E5) {
            pak->value = 0; // kneel
        }
        if (pak->value == 0x313AC9D1) {
            pak->value = 0; // bored
        }
        if (pak->value == 0x3033596A) {
            pak->value = 0; // moan
        }
        if (pak->value == 0x305A7EF2) {
            pak->value = 0; // flex
        }
        if (pak->value == 0x74999B06) {
            pak->value = 0; // fistshake
        }
        if (pak->value == 0x30446E61) {
            pak->value = 0; // roar
        }
    }
}

void PconsWindow::OnAgentState(GW::HookStatus*, GW::Packet::StoC::AgentState* pak)
{
    if (PconAlcohol::suppress_drunk_emotes && pak->agent_id == GW::Agents::GetObservingId() && pak->state & 0x2000) {
        pak->state ^= 0x2000;
    }
}

void CHAT_CMD_FUNC(PconsWindow::CmdPcons)
{
    if (argc <= 1) {
        Instance().ToggleEnable();
    }
    else if (argc >= 2) {
        const std::wstring arg1 = TextUtils::ToLower(argv[1]);
        if (arg1 != L"on" && arg1 != L"off" && arg1 != L"toggle" && arg1 != L"refill") {
            Log::Error("Invalid argument '%ls', please use /pcons [on|off|toggle] [pcon]", argv[1]);
        }
        if (argc == 2) {
            if (arg1 == L"on") {
                Instance().SetEnabled(true);
            }
            else if (arg1 == L"off") {
                Instance().SetEnabled(false);
            }
            else if (arg1 == L"toggle") {
                Instance().ToggleEnable();
            }
            else if (arg1 == L"refill") {
                Instance().Refill();
            }
        }
        else {
            std::wstring argPcon = TextUtils::ToLower(argv[2]);
            for (auto i = 3; i < argc; i++) {
                argPcon.append(L" ");
                argPcon.append(TextUtils::ToLower(argv[i]));
            }
            const std::vector<Pcon*>& pcons = Instance().pcons;
            const std::string compare = TextUtils::WStringToString(argPcon);
            const unsigned int compareLength = compare.length();
            Pcon* bestMatch = nullptr;
            unsigned int bestMatchLength = 0;
            for (size_t i = 0; i < pcons.size(); i++) {
                Pcon* pcon = pcons[i];
                const std::string pconName(pcon->chat);
                std::string pconNameSanitized = TextUtils::ToLower(pconName);
                const unsigned int pconNameLength = pconNameSanitized.length();
                if (compareLength > pconNameLength) {
                    continue;
                }
                if (pconNameSanitized.rfind(compare) == std::string::npos) {
                    continue;
                }
                if (bestMatchLength < pconNameLength) {
                    bestMatchLength = pconNameLength;
                    bestMatch = pcon;
                    if (bestMatchLength == pconNameLength) {
                        break;
                    }
                }
            }
            if (bestMatch == nullptr) {
                Log::Error("Could not find pcon %ls", argPcon.c_str());
                return;
            }
            if (arg1 == L"on") {
                bestMatch->SetEnabled(true);
            }
            else if (arg1 == L"off") {
                bestMatch->SetEnabled(false);
            }
            else if (arg1 == L"toggle") {
                bestMatch->Toggle();
            }
            else if (arg1 == L"refill") {
                bestMatch->Refill();
            }
        }
    }
}

bool PconsWindow::DrawTabButton(const bool show_icon, const bool show_text, const bool center_align_text)
{
    const bool clicked = ToolboxWindow::DrawTabButton(show_icon, show_text, center_align_text);

    ImGui::PushStyleColor(ImGuiCol_Text, enabled ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button(enabled ? "Enabled###pconstoggle" : "Disabled###pconstoggle", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        ToggleEnable();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    return clicked;
}

void PconsWindow::Draw(IDirect3DDevice9* device)
{
    if (!visible) {
        return;
    }
    ImGui::SetNextWindowCenter(ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(Name(), GetVisiblePtr(), GetWinFlags())) {
        return ImGui::End();
    }
    if (show_enable_button) {
        ImGui::PushStyleColor(ImGuiCol_Text, enabled ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1));
        if (ImGui::Button(enabled ? "Enabled###pconstoggle" : "Disabled###pconstoggle", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            ToggleEnable();
        }
        ImGui::PopStyleColor();
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(1, 1));

    if (ImGui::BeginTable("#pcons_table", std::max(items_per_row, 1))) {
        for (auto pcon : pcons) {
            if (!pcon->IsVisible())
                continue;
            ImGui::TableNextColumn();
            pcon->Draw(device);
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleVar(4);

    if (instance_type == InstanceType::Explorable && show_auto_disable_pcons_tickbox) {
        if (!current_objectives_to_check.empty()) {
            ImGui::Checkbox("Off @ end", &disable_cons_on_objective_completion);
            ImGui::ShowHelp(disable_cons_on_objective_completion_hint);
        }
        if (!(current_final_room_location == GW::Vec2f(0, 0))) {
            ImGui::Checkbox("Off @ boss", &disable_cons_in_final_room);
            ImGui::ShowHelp(disable_cons_in_final_room_hint);
        }
        if (in_vanquishable_area) {
            ImGui::Checkbox("Off @ end", &disable_cons_on_vanquish_completion);
            ImGui::ShowHelp(disable_cons_on_vanquish_completion_hint);
        }
    }
    ImGui::End();
}

void PconsWindow::Update(const float)
{
    if (instance_type != GW::Map::GetInstanceType() || map_id != GW::Map::GetMapID()) {
        MapChanged(); // Map changed.
    }
    if (!player && instance_type == InstanceType::Explorable) {
        player = GW::Agents::GetControlledCharacter(); // Won't be immediately able to get player ptr on map load, so put here.
    }
    if (!Pcon::map_has_effects_array) {
        Pcon::map_has_effects_array = GW::Effects::GetPlayerEffectsArray() != nullptr;
    }
    in_vanquishable_area = GW::Map::GetFoesToKill() != 0;
    CheckBossRangeAutoDisable();
    for (Pcon* pcon : pcons) {
        pcon->Update();
    }
}

void PconsWindow::MapChanged()
{
    elite_area_check_timer = TIMER_INIT();
    map_id = GW::Map::GetMapID();
    Pcon::map_has_effects_array = false;
    if (instance_type != InstanceType::Loading) {
        previous_instance_type = instance_type;
    }
    instance_type = GW::Map::GetInstanceType();
    // If we've just come from an explorable area then disable pcons.
    if (disable_pcons_on_map_change && previous_instance_type == InstanceType::Explorable) {
        SetEnabled(false);
    }

    player = nullptr;
    elite_area_disable_triggered = false;
    // Find out which objectives we need to complete for this map.
    const auto map_objectives_it = objectives_to_complete_by_map_id.find(map_id);
    if (map_objectives_it != objectives_to_complete_by_map_id.end()) {
        objectives_complete.clear();
        current_objectives_to_check = map_objectives_it->second;
    }
    else {
        current_objectives_to_check.clear();
    }
    // Find out if we need to check for boss range for this map.
    const auto map_location_it = final_room_location_by_map_id.find(map_id);
    if (map_location_it != final_room_location_by_map_id.end()) {
        current_final_room_location = map_location_it->second;
    }
    else {
        current_final_room_location = GW::Vec2f(0, 0);
    }
}

void PconsWindow::Refill(const bool do_refill) const
{
    for (Pcon* pcon : pcons) {
        pcon->Refill(do_refill && pcon->IsEnabled());
    }
}

bool PconsWindow::GetEnabled() const
{
    return enabled;
}

void PconsWindow::ToggleEnable() { SetEnabled(!enabled); }

bool PconsWindow::SetEnabled(const bool b)
{
    if (enabled == b) {
        return enabled; // Do nothing - already enabled/disabled.
    }
    enabled = b;
    Refill(enabled && Pcon::refill_if_below_threshold);
    switch (GW::Map::GetInstanceType()) {
        case InstanceType::Outpost:
            if (tick_with_pcons) {
                GW::PartyMgr::Tick(enabled);
            }
        case InstanceType::Explorable: {
            if (HotkeysWindow::CurrentHotkey() && !HotkeysWindow::CurrentHotkey()->show_message_in_emote_channel) {
                break; // Selected hotkey doesn't allow a message.
            }
            const ImGuiWindow* main = ImGui::FindWindowByName(MainWindow::Instance().Name());
            const ImGuiWindow* pcon = ImGui::FindWindowByName(Name());
            if ((pcon == nullptr || pcon->Collapsed || !visible) && (main == nullptr || main->Collapsed || !MainWindow::Instance().visible)) {
                Log::Flash("Pcons %s", enabled ? "enabled" : "disabled");
            }
            break;
        }
        default:
            break;
    }
    return enabled;
}

void PconsWindow::RegisterSettingsContent()
{
    ToolboxUIElement::RegisterSettingsContent();
    ToolboxModule::RegisterSettingsContent(
        "Game Settings", nullptr,
        [this](const std::string&, const bool is_showing) {
            if (!is_showing) {
                return;
            }
            DrawLunarsAndAlcoholSettings();
        },
        1.1f);
}

void PconsWindow::DrawLunarsAndAlcoholSettings()
{
    ImGui::NewLine();
    ImGui::Text("Lunars and Alcohol");
    ImGui::Indent();
    ImGui::Text("Current drunk level: %d", Pcon::alcohol_level);
    ImGui::StartSpacedElements(380.f);
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Suppress lunar and drunk post-processing effects", &Pcon::suppress_drunk_effect);
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Suppress lunar and drunk text", &Pcon::suppress_drunk_text);
    ImGui::ShowHelp("Will hide drunk and lunars messages on top of your and other characters");
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Suppress air of superiority text", &Pcon::suppress_air_of_superiority_text);
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Suppress drunk emotes", &Pcon::suppress_drunk_emotes);
    ImGui::ShowHelp("Important:\n"
        "This feature is experimental and might crash your game.\n"
        "Using level 1 alcohol instead of this is recommended for preventing drunk emotes.\n"
        "This will prevent kneel, bored, moan, flex, fistshake and roar.\n");
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Hide Spiritual Possession and Lucky Aura", &Pcon::suppress_lunar_skills);
    ImGui::ShowHelp("Will hide the skills in your effect monitor");
    ImGui::Unindent();
}

void PconsWindow::CheckBossRangeAutoDisable()
{
    // Trigger Elite area auto disable if applicable
    if (!enabled || elite_area_disable_triggered || instance_type != InstanceType::Explorable) {
        return; // Pcons disabled, auto disable already triggered, or not in explorable area.
    }
    if (!disable_cons_in_final_room || current_final_room_location == GW::Vec2f(0, 0) || !player || TIMER_DIFF(elite_area_check_timer) < 1000) {
        return; // No boss location to check for this map, player ptr not loaded, or checked recently already.
    }
    elite_area_check_timer = TIMER_INIT();
    const float d = GetDistance(GW::Vec2f(player->pos), current_final_room_location);
    if (d > 0 && d <= Range::Spirit) {
        elite_area_disable_triggered = true;
        SetEnabled(false);
        Log::Flash("Cons auto-disabled in range of boss");
    }
}

void PconsWindow::LoadSettings(ToolboxIni* ini)
{
    ToolboxWindow::LoadSettings(ini);


    for (Pcon* pcon : pcons) {
        pcon->LoadSettings(ini, Name());
    }

    LOAD_BOOL(tick_with_pcons);
    LOAD_BOOL(show_enable_button);
    items_per_row = ini->GetLongValue(Name(), VAR_NAME(items_per_row), 3);
    Pcon::pcons_delay = ini->GetLongValue(Name(), VAR_NAME(pcons_delay), 5000);
    Pcon::lunar_delay = ini->GetLongValue(Name(), VAR_NAME(lunar_delay), 500);
    Pcon::size = static_cast<float>(ini->GetDoubleValue(Name(), "pconsize", 46.0));
    Pcon::enabled_bg_color = Colors::Load(ini, Name(), VAR_NAME(enabled_bg_color), Pcon::enabled_bg_color);
    Pcon::disable_when_not_found = ini->GetBoolValue(Name(), VAR_NAME(disable_when_not_found), Pcon::disable_when_not_found);
    Pcon::suppress_drunk_effect = ini->GetBoolValue(Name(), VAR_NAME(suppress_drunk_effect), Pcon::suppress_drunk_effect);
    Pcon::suppress_drunk_text = ini->GetBoolValue(Name(), VAR_NAME(suppress_drunk_text), Pcon::suppress_drunk_text);
    Pcon::suppress_air_of_superiority_text = ini->GetBoolValue(Name(), VAR_NAME(suppress_air_of_superiority_text), Pcon::suppress_air_of_superiority_text);
    Pcon::suppress_drunk_emotes = ini->GetBoolValue(Name(), VAR_NAME(suppress_drunk_emotes), Pcon::suppress_drunk_emotes);
    Pcon::suppress_lunar_skills = ini->GetBoolValue(Name(), VAR_NAME(suppress_lunar_skills), Pcon::suppress_lunar_skills);
    Pcon::pcons_by_character = ini->GetBoolValue(Name(), VAR_NAME(pcons_by_character), Pcon::pcons_by_character);
    Pcon::hide_city_pcons_in_explorable_areas = ini->GetBoolValue(Name(), VAR_NAME(hide_city_pcons_in_explorable_areas), Pcon::hide_city_pcons_in_explorable_areas);
    Pcon::refill_if_below_threshold = ini->GetBoolValue(Name(), VAR_NAME(refill_if_below_threshold), Pcon::refill_if_below_threshold);
    LOAD_BOOL(show_auto_refill_pcons_tickbox);
    LOAD_BOOL(show_auto_disable_pcons_tickbox);

    LOAD_BOOL(show_storage_quantity);
    LOAD_BOOL(shift_click_toggles_category);

    LOAD_BOOL(disable_pcons_on_map_change);
    LOAD_BOOL(disable_cons_on_vanquish_completion);
    LOAD_BOOL(disable_cons_on_dungeon_completion);
    LOAD_BOOL(disable_cons_on_mission_completion);
    LOAD_BOOL(disable_cons_in_final_room);
    LOAD_BOOL(disable_cons_on_objective_completion);

    std::string order = ini->GetValue(Name(), "order", "");
    std::vector<std::string_view> order_vec;
    for (const auto str : std::views::split(order, ';') | std::views::transform([](auto&& rng) {
        if (rng.begin() == rng.end()) {
            return std::string_view("");
        }
        return std::string_view(&*rng.begin(), std::ranges::distance(rng));
    })) {
        if (str.empty()) {
            continue;
        }
        order_vec.push_back(str);
    }
    std::ranges::sort(pcons, [&order_vec](const Pcon* a, const Pcon* b) {
        const auto a_it = std::ranges::find(order_vec, a->ini);
        const auto b_it = std::ranges::find(order_vec, b->ini);
        return a_it != order_vec.end() && (b_it == order_vec.end() || a_it < b_it);
    });
}

void PconsWindow::SaveSettings(ToolboxIni* ini)
{
    ToolboxWindow::SaveSettings(ini);

    for (const Pcon* pcon : pcons) {
        pcon->SaveSettings(ini, Name());
    }

    SAVE_BOOL(tick_with_pcons);
    SAVE_UINT(items_per_row);
    ini->SetLongValue(Name(), VAR_NAME(pcons_delay), Pcon::pcons_delay);
    ini->SetLongValue(Name(), VAR_NAME(lunar_delay), Pcon::lunar_delay);
    ini->SetDoubleValue(Name(), "pconsize", Pcon::size);
    ini->SetBoolValue(Name(), VAR_NAME(disable_when_not_found), Pcon::disable_when_not_found);
    Colors::Save(ini, Name(), VAR_NAME(enabled_bg_color), Pcon::enabled_bg_color);
    SAVE_BOOL(show_enable_button);

    ini->SetBoolValue(Name(), VAR_NAME(suppress_drunk_effect), Pcon::suppress_drunk_effect);
    ini->SetBoolValue(Name(), VAR_NAME(suppress_drunk_text), Pcon::suppress_drunk_text);
    ini->SetBoolValue(Name(), VAR_NAME(suppress_air_of_superiority_text), Pcon::suppress_air_of_superiority_text);
    ini->SetBoolValue(Name(), VAR_NAME(suppress_drunk_emotes), Pcon::suppress_drunk_emotes);
    ini->SetBoolValue(Name(), VAR_NAME(suppress_lunar_skills), Pcon::suppress_lunar_skills);
    ini->SetBoolValue(Name(), VAR_NAME(pcons_by_character), Pcon::pcons_by_character);
    ini->SetBoolValue(Name(), VAR_NAME(hide_city_pcons_in_explorable_areas), Pcon::hide_city_pcons_in_explorable_areas);

    ini->SetBoolValue(Name(), VAR_NAME(refill_if_below_threshold), Pcon::refill_if_below_threshold);
    SAVE_BOOL(show_auto_refill_pcons_tickbox);
    SAVE_BOOL(show_auto_disable_pcons_tickbox);
    SAVE_BOOL(shift_click_toggles_category);
    SAVE_BOOL(show_storage_quantity);

    SAVE_BOOL(disable_pcons_on_map_change);
    SAVE_BOOL(disable_cons_on_vanquish_completion);
    SAVE_BOOL(disable_cons_on_dungeon_completion);
    SAVE_BOOL(disable_cons_on_mission_completion);
    SAVE_BOOL(disable_cons_in_final_room);
    SAVE_BOOL(disable_cons_on_objective_completion);

    auto ss = std::stringstream();
    std::ranges::for_each(pcons, [&ss](const auto pcon) {
        ss << pcon->ini << ";";
    });
    const auto str = ss.str();
    ini->SetValue(Name(), "order", str.c_str());
}

void PconsWindow::DrawSettingsInternal()
{
    ImGui::Separator();
    ImGui::Text("Functionality:");
    ImGui::StartSpacedElements(275.f);
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Toggle Pcons per character", &Pcon::pcons_by_character);
    ImGui::ShowHelp("Tick to remember pcon enable/disable per character.\nUntick to enable/disable regardless of current character.");
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Tick with pcons", &tick_with_pcons);
    ImGui::ShowHelp("Enabling or disabling pcons will also Tick or Untick in party list");
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Disable when not found", &Pcon::disable_when_not_found);
    ImGui::ShowHelp("Toolbox will disable a pcon if it is not found in the inventory");
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Refill from storage", &Pcon::refill_if_below_threshold);
    ImGui::ShowHelp("Toolbox will refill pcons from storage if below the threshold");
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Show storage quantity in outpost", &show_storage_quantity);
    ImGui::ShowHelp("Display a number on the bottom of each pcon icon, showing total quantity in storage.\n"
        "This only displays when in an outpost.");
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Shift + Click toggles category", &shift_click_toggles_category);
    ImGui::ShowHelp("If this is ticked, clicking on a pcon while holding shift will enable/disable all of the same category\n"
        "Categories: Conset, Rock Candies, Kabob+Soup+Salad, Pie+Cupcake+Apple+Corn+Egg+Warsup");
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Show Enable/Disable button", &show_enable_button);
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Show auto disable pcons checkboxes", &show_auto_disable_pcons_tickbox);
    ImGui::ShowHelp("Will show a tickbox in the pcons window when in an elite area");
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Hide city Pcons in explorable areas", &Pcon::hide_city_pcons_in_explorable_areas);

    ImGui::SliderInt("Pcons delay", &Pcon::pcons_delay, 100, 5000, "%d milliseconds");
    ImGui::ShowHelp("After using a pcon, toolbox will not use it again for this amount of time.\n"
        "It is needed to prevent toolbox from using a pcon twice, before it activates.\n"
        "Decrease the value if you have good ping and you die a lot.");
    ImGui::SliderInt("Lunars delay", &Pcon::lunar_delay, 100, 500, "%d milliseconds");

    ImGui::Separator();
    ImGui::Text("Interface:");
    ImGui::SliderInt("Items per row", &items_per_row, 1, 18);
    ImGui::DragFloat("Pcon Size", &Pcon::size, 1.0f, 10.0f, 0.0f);
    ImGui::ShowHelp("Size of each Pcon icon in the interface");
    Colors::DrawSettingHueWheel("Enabled-Background", &Pcon::enabled_bg_color);
    if (Pcon::size <= 1.0f) {
        Pcon::size = 1.0f;
    }
    if (ImGui::TreeNodeEx("Visibility & Thresholds", ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth)) {
        if (ImGui::BeginTable("pcons_visibility", 3)) {
            const auto font_size = ImGui::GetFontSize();
            const auto threshold_hint = "When you have less than this amount:\n-The number in the interface becomes yellow.\n-Warning message is displayed when zoning into outpost.";

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextDisabled("Label");
            ImGui::TableNextColumn();
            ImGui::TextDisabled("Threshold");
            ImGui::TableNextColumn();
            ImGui::TextDisabled("Order");

            for (auto it = pcons.begin(); it != pcons.end(); ++it) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                Pcon* pcon = *it;
                ImGui::PushID(pcon->ini.c_str());
                ImGui::Checkbox("###checked", &pcon->visible);
                ImGui::SameLine();
                const auto tex = pcon->GetTexture();
                if (tex) {
                    ImGui::ImageFit(*tex, ImVec2(font_size, font_size));
                    ImGui::SameLine();
                }
                ImGui::TextUnformatted(pcon->chat.c_str());
                
                ImGui::TableNextColumn();
                if (ImGui::InputInt("###threshold", &pcon->threshold, 1, 10)) {
                    if (pcon->threshold < 0) {
                        pcon->threshold = 0;
                    }
                    if (pcon->threshold > 250) {
                        pcon->threshold = 250;
                    }
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(threshold_hint);
                }

                ImGui::TableNextColumn();
                if (it != pcons.begin()) {
                    if (ImGui::Button(ICON_FA_ARROW_UP)) {
                        std::iter_swap(it, it - 1);
                    }
                    if (it != pcons.end() - 1) {
                        ImGui::SameLine();
                    }
                }
                else {
                    ImGui::SameLine(62);
                }
                if (it != pcons.end() - 1) {
                    if (ImGui::Button(ICON_FA_ARROW_DOWN)) {
                        std::iter_swap(it, it + 1);
                    }
                }

                ImGui::PopID();
            }
        }
        ImGui::EndTable();
        ImGui::TreePop();
    }

    ImGui::Separator();
    DrawLunarsAndAlcoholSettings();
    ImGui::Separator();
    ImGui::Text("Auto-Disabling Pcons");
    ImGui::StartSpacedElements(380.f);
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Auto Disable on Vanquish completion", &disable_cons_on_vanquish_completion);
    ImGui::ShowHelp(disable_cons_on_vanquish_completion_hint);
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Auto Disable on Dungeon completion", &disable_cons_on_dungeon_completion);
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Auto Disable on Mission completion", &disable_cons_on_mission_completion);
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Auto Disable in final room of Urgoz/Deep", &disable_cons_in_final_room);
    ImGui::ShowHelp(disable_cons_in_final_room_hint);
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Auto Disable on final objective completion", &disable_cons_on_objective_completion);
    ImGui::ShowHelp(disable_cons_on_objective_completion_hint);
    ImGui::NextSpacedElement();
    ImGui::Checkbox("Disable on map change", &disable_pcons_on_map_change);
    ImGui::ShowHelp("Toolbox will disable pcons when leaving an explorable area");
}
