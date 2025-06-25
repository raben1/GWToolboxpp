#pragma once

using Color = ImU32;

constexpr uint32_t ImGuiButtonFlags_AlignTextLeft = 1 << 20;

namespace ImGui {
    // Return false to close the context menu
    using ImGuiContextMenuCallback = std::function<bool(void* wparam)>;

    using ImGuiConfirmDialogCallback = std::function<void(bool result, void* wparam)>;

    IMGUI_API void SetContextMenu(ImGuiContextMenuCallback callback, void* wparam = nullptr);

    IMGUI_API bool ShowingContextMenu();

    IMGUI_API void DrawContextMenu();

    // Similar to InputPassword, but has a button to show/hide the value
    IMGUI_API bool InputTextSecret(const char* label, char* buf, size_t buf_size, bool* show_password = nullptr, const char* hint = nullptr);

    IMGUI_API void DrawConfirmDialog();

    // Push font, but with a different size to the default one.
    IMGUI_API void PushFont(ImFont* font, float font_size);

    // If we're using a draw list that isn't the current window one, we need to explicitly state it
    IMGUI_API void PushFont(ImFont* font, ImDrawList* draw_list, float font_size = -1.f);
    // Remove a font added to an explicit draw list
    IMGUI_API void PopFont(ImDrawList* draw_list);

    // InputText using a std::string - make sure you set the capacity for the string first, otherwise it won't be able to be filled.
    IMGUI_API bool InputText(const char* label, std::string& buf, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);

    IMGUI_API void SetTooltip(std::function<void()> tooltip_callback);

    // Shorthand for ImGui::GetIO().GlobalFontScale
    IMGUI_API const float& FontScale();
    // Initialise available width etc for adding spaced elements. Must be called before calling NextSpacedElement()
    IMGUI_API void StartSpacedElements(float width, bool include_font_scaling = true);
    // Called before adding an imgui control that needs to be spaced. Call StartSpacedElements() before this.
    IMGUI_API void NextSpacedElement();

    // Shows '(?)' and the helptext when hovered
    IMGUI_API void ShowHelp(const char* help);
    // Shows current text with a drop shadow
    IMGUI_API void TextShadowed(const char* label, ImVec2 offset = {1, 1}, const ImVec4& shadow_color = {0, 0, 0, 1});

    IMGUI_API void SetNextWindowCenter(ImGuiWindowFlags flags);

    IMGUI_API const std::vector<ImGuiKey>& GetPressedKeys();

    IMGUI_API bool MyCombo(const char* label, const char* preview_text, int* current_item,
                           bool (*items_getter)(void* data, int idx, const char** out_text), void* data, int items_count);

    // Show a popup on-screen with a message and yes/no buttons. Returns true if an option has been chosen, with *result as true/false for yes/no
    IMGUI_API void ConfirmDialog(const char* message, ImGui::ImGuiConfirmDialogCallback callback, void* wparam = nullptr);

    IMGUI_API bool SmallConfirmButton(const char* label, const char* confirm_content, ImGui::ImGuiConfirmDialogCallback callback, void* wparam = nullptr);
    IMGUI_API bool ChooseKey(const char* label, char* buf, size_t buf_len, long* key_code);

    IMGUI_API bool ConfirmButton(const char* label, bool* confirm_bool, const char* confirm_content = "Are you sure you want to continue?");

    // Button with single icon texture
    IMGUI_API bool IconButton(const char* label, ImTextureID icon, const ImVec2& size, ImGuiButtonFlags flags = ImGuiButtonFlags_None, const ImVec2& icon_size = {0.f, 0.f});

    IMGUI_API void ClosePopup(const char* popup_id);

    // Button with 1 or more icon textures overlaid
    IMGUI_API bool CompositeIconButton(const char* label, const ImTextureID* icons, size_t icons_len, const ImVec2& size, ImGuiButtonFlags flags = ImGuiButtonFlags_None, const ImVec2& icon_size = {0.f, 0.f}, const ImVec2& uv0 = {0.f, 0.f},
                                       ImVec2 uv1 = {0.f, 0.f});

    IMGUI_API bool ColorButtonPicker(const char*, Color*, ImGuiColorEditFlags = 0);
    // Add cropped image to current window
    IMGUI_API void ImageCropped(ImTextureID user_texture_id, const ImVec2& size);

    IMGUI_API void ImageFit(const ImTextureID user_texture_id, const ImVec2& size_of_container);

    // Shim to cast
    IMGUI_API bool IsKeyDown(long key);

    // Shim to new ImageButton definition; an frame_padding isn't needed now.
    IMGUI_API bool ImageButton(ImTextureID user_texture_id, const ImVec2& image_size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0),
                               const ImVec4& tint_col = ImVec4(1, 1, 1, 1));

    // Window/context independent check
    IMGUI_API bool IsMouseInRect(const ImVec2& top_left, const ImVec2& bottom_right);
    // Add cropped image to window draw list
    IMGUI_API void AddImageCropped(ImTextureID user_texture_id, const ImVec2& top_left, const ImVec2& bottom_right);
    // Calculate the end position of a crop box for the given texture to fit into the given size
    IMGUI_API ImVec2 CalculateUvCrop(ImTextureID user_texture_id, const ImVec2& size);

    IMGUI_API bool ColorPalette(const char* label, size_t* palette_index, const ImVec4* palette, size_t count, size_t max_per_line, ImGuiColorEditFlags flags);

    // call before ImGui::Render() to clamp all windows to screen - pass false to restore original positions
    // e.g. before saving, or if the user doesn't want the windows clamped
    IMGUI_API void ClampAllWindowsToScreen(bool clamp);

    IMGUI_API bool ButtonWithHint(const char* label, const char* tooltip, const ImVec2& size_arg);

    IMGUI_API void DrawTextWithShadow(const char* text, const ImVec2& pos,
                                      ImU32 textColor = IM_COL32(255, 255, 255, 255),
                                      ImU32 shadowColor = IM_COL32(0, 0, 0, 255),
                                      float shadowOffset = 1.0f);
    IMGUI_API void DrawTextWithShadow(const char* text,
                                      ImU32 textColor = IM_COL32(255, 255, 255, 255),
                                      ImU32 shadowColor = IM_COL32(0, 0, 0, 255),
                                      float shadowOffset = 1.0f);
    IMGUI_API void DrawTextWithOutline(const char* text, const ImVec2& pos,
                                       ImU32 textColor = IM_COL32(255, 255, 255, 255),
                                       ImU32 outlineColor = IM_COL32(0, 0, 0, 255),
                                       float thickness = 1.0f);
    IMGUI_API void DrawTextWithOutline(const char* text,
                                       ImU32 textColor = IM_COL32(255, 255, 255, 255),
                                       ImU32 outlineColor = IM_COL32(0, 0, 0, 255),
                                       float thickness = 1.0f);
    IMGUI_API void DrawTextWithShadow(ImDrawList* draw_list, ImFont* font, const char* text,
                                      const ImVec2& center_pos, ImU32 textColor, ImU32 shadowColor,
                                      float shadowOffset = 1.0f);
    IMGUI_API void DrawTextWithOutline(ImDrawList* draw_list, ImFont* font, const char* text,
                                       const ImVec2& center_pos, ImU32 textColor, ImU32 outlineColor,
                                       float thickness = 1.0f);
}
