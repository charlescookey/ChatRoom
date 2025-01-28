#pragma once

//#define _CRT_SECURE_NO_WARNINGS
//remove later from prepocessor direcitves

#include <ctype.h>          // toupper
#include "imgui.h"

#include <string>
#include <unordered_map>

struct User {
    std::string name;
    bool selected = false;
};

const char delimiter = '\x1F';

class DM {
public:
    char                  InputBuf[256];
    ImVector<std::string>       Items;

;    // -1: new line, 0..History.Size-1 browsing history.
    ImGuiTextFilter       Filter;
    bool                  AutoScroll;
    bool                  ScrollToBottom;
    bool                  sendMessage = false;

    DM()
    {
        //IMGUI_DEMO_MARKER("Examples/Console");
        ClearLog();
        memset(InputBuf, 0, sizeof(InputBuf));
        AutoScroll = true;
        ScrollToBottom = false;
    }
    ~DM()
    {
        ClearLog();
    }

    void    ClearLog()
    {
        Items.clear();
    }

    void    Draw(std::string name, User& user, Network& net)
    {
        std::string title = "Private Chat With " + user.name;
        ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin(title.c_str(), &user.selected))
        {
            ImGui::End();
            return;
        }

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Close Console")) {
                user.selected = false;
            }
            ImGui::EndPopup();
        }

        // TODO: display items starting from the bottom

        // Reserve enough left-over height for 1 separator + 1 input text
        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y* 2 + ImGui::GetFrameHeightWithSpacing() ;
        //ImGuiWindowFlags_HorizontalScrollbar
        if (ImGui::BeginChild("ChildRegion", ImVec2(0, -footer_height_to_reserve), true, ImGuiWindowFlags_None))
        //if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), ImGuiWindowFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
        {
            if (ImGui::BeginPopupContextWindow())
            {
                if (ImGui::Selectable("Clear")) ClearLog();
                ImGui::EndPopup();
            }

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

            for (std::string item : Items)
            {
                ImGui::TextUnformatted(item.c_str());
            }

            // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
            // Using a scrollbar or mouse-wheel will take away from the bottom edge.
            if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
                ImGui::SetScrollHereY(1.0f);
            ScrollToBottom = false;

            ImGui::PopStyleVar();
        }
        ImGui::EndChild(); 

        // Command-line
        bool reclaim_focus = false;
        //These flags are useful
        ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
        if (ImGui::InputText("Private Message", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags, &TextEditCallbackStub, (void*)this))
        {
            
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Send Private")) {
            sendMessage = true;
        }
        else {
            sendMessage = false;
        }

        // Auto-focus on window apparition
        ImGui::SetItemDefaultFocus();
        if (reclaim_focus)
            ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

        if (sendMessage) {
            std::string s(InputBuf);
            if (!s.empty()) {
                s = std::string("3") + delimiter + user.name + delimiter + name + delimiter +  name + ": " + s;
                net.sendMessage(s);
            }
            memset(InputBuf, 0, sizeof(InputBuf));
            reclaim_focus = true;
        }

        ImGui::End();
    }

    // In C++11 you'd be better off using lambdas for this sort of forwarding callbacks
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data)
    {
        DM* console = (DM*)data->UserData;
        return console->TextEditCallback(data);
    }

    int     TextEditCallback(ImGuiInputTextCallbackData* data)
    {
        //we can use this fpr @ later
        return 0;
    }
};