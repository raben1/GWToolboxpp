#pragma once

#include <ToolboxUIPlugin.h>

class ExamplePlugin : public ToolboxPlugin {
public:
    ExamplePlugin() = default;
    ~ExamplePlugin() override = default;

    const char* Name() const override { return "Example Plugin"; }

    void LoadSettings(const wchar_t*) override;
    void SaveSettings(const wchar_t*) override;
    [[nodiscard]] bool HasSettings() const override { return true; }
    void DrawSettings() override;
    void Initialize(ImGuiContext* ctx, ImGuiAllocFns allocator_fns, HMODULE toolbox_dll) override;
    void SignalTerminate() override;
    bool CanTerminate() override;
    // Draw user interface. Will be called every frame if the element is visible
    void Draw(IDirect3DDevice9* pDevice) override;
};
