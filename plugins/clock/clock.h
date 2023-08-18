#pragma once

#include <ToolboxUIPlugin.h>

class Clock : public ToolboxUIPlugin {
public:
    Clock()
    {
        show_closebutton = false;
        is_resizable = true;
        is_movable = true;
        lock_size = true;
        lock_move = true;
    }

    const char* Name() const override { return "Clock"; }

    void Draw(IDirect3DDevice9*) override;
    void Initialize(ImGuiContext*, ImGuiAllocFns, HMODULE) override;
    void Terminate() override;
};
