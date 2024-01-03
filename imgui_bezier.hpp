#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include "interpolation.hpp"
#include <vector>
#include <time.h>

namespace ImGui
{
    // Global variables necessary for the widget loop
    int steps = 512;
    bool grabbing = false;
    int grabbing_index = 0;

    // Get the index of the tiniest value in a float vector
    int tiniest_value_index(std::vector<float> vec) {
        float dist = vec.at(0);
        int index = 0;
        
        for (int i = 0; i < vec.size(); i++) {
            if (dist > vec.at(i)) {
                dist = vec.at(i);
                index = i;
            }
        }

        return index;
    }

    // Return true if a point is inside a rectangle
    bool get_collision(ImRect rect, ImVec2 point) {
        if ((point.x < rect.Max.x) and (point.x > rect.Min.x) and (point.y > rect.Min.y) and (point.y < rect.Max.y)) return true;

        return false;
    }

    // Get a any size quantity of points and set its start and end at 0 and 1
    std::vector<ImVec2> BezierValue(std::vector<ImVec2> &Points, int steps) {
        std::vector<ImVec2> P;

        ImVec2 start = {0,0}, end = {1,1};

        P.insert(P.begin(), start);
        for (ImVec2 point : Points) P.push_back(point);
        P.push_back(end);

        std::vector<ImVec2> results;
        results = vector_interpolation(P, steps);
        results.push_back({1,1});
        return results;
    }

    // The widget function
    bool Bezier(const char *label, std::vector<ImVec2> &Points, std::vector<ImVec2> &Results) {

        // bezier widget
        ImGuiContext& g = *GImGui;
        const ImGuiStyle& Style = GetStyle();
        const ImGuiIO& IO = GetIO();
        ImDrawList* DrawList = GetWindowDrawList();
        ImGuiWindow* Window = GetCurrentWindow();
        // Can be changed (if the size, is too tiny can generate bugs)
        ImGui::SetWindowSize(ImVec2(600, 400));


        if (Window->SkipItems)
            return false;

        // header and spacing
        int hovered = IsItemActive() || IsItemHovered();

        // prepare canvas
        ImVec2 Canvas = GetContentRegionAvail();
        Canvas.y = Canvas.y - 50;
        Canvas.x = Canvas.x - 50;

        // Create the rectangle frame
        ImRect bb(Window->DC.CursorPos, Window->DC.CursorPos + Canvas);
        ItemSize(bb);
        if (!ItemAdd(bb, NULL))
            return false;

        // Handle the moving of the window
        const ImGuiID id = Window->GetID(label);
        hovered |= 0 != ItemHoverable(ImRect(bb.Min, bb.Min + Canvas), id, g.LastItemData.InFlags);

        // Render the bb rect frame
        RenderFrame(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg, 1), true, Style.FrameRounding);

        // background grid
        for (int i = 0; i <= Canvas.x; i += (Canvas.x / 4)) {
            DrawList->AddLine(
                ImVec2(bb.Min.x + i, bb.Min.y),
                ImVec2(bb.Min.x + i, bb.Max.y),
                GetColorU32(ImGuiCol_TextDisabled));
        }
        for (int i = 0; i <= Canvas.y; i += (Canvas.y / 4)) {
            DrawList->AddLine(
                ImVec2(bb.Min.x, bb.Min.y + i),
                ImVec2(bb.Max.x, bb.Min.y + i),
                GetColorU32(ImGuiCol_TextDisabled));
        }

        // eval curve
        SliderInt("Smoothness", &steps, 512, 1024, "%d", 0);
        std::vector<ImVec2> results = ImGui::BezierValue(Points, steps);
        Results = results;

        // Let the user handle the close
        if (Button("Close", {80,20})) return false;
        
        // handle grabbers
        ImVec2 mouse = GetIO().MousePos;
        std::vector<ImVec2> pos;
        std::vector<float> distance;
        
        for (int i = 0; i < Points.size(); ++i) {
            pos.push_back(ImVec2(Points.at(i).x, 1 - Points.at(i).y) * (bb.Max - bb.Min) + bb.Min);

            distance.push_back((pos.at(i).x - mouse.x)/Canvas.x * (pos.at(i).x - mouse.x)/Canvas.x + (pos.at(i).y - mouse.y)/Canvas.y * (pos.at(i).y - mouse.y)/Canvas.y);
        }

        ImVec2 mouse_canvas = (mouse - bb.Min)/(bb.Max - bb.Min);
        mouse_canvas.y = (mouse_canvas.y - 1) * -1;

        if (IsMouseClicked(1) and get_collision(bb, mouse)) {
            int grab_index = tiniest_value_index(distance);

            if ((distance.at(grab_index) < 0.001) and (Points.size() > 2)) {
                Points.erase(Points.begin() + grab_index);
            } 
            else {
                Points.insert(Points.begin() + nearest_val_index(Points, mouse_canvas), mouse_canvas);
            }
        }
        
        if (IsMouseClicked(0) and get_collision(bb, mouse) and (!grabbing)) {
            grabbing_index = tiniest_value_index(distance);
            grabbing = true;
        }
        else if (IsMouseDown(0) and get_collision(bb, mouse)) {
            if (distance.at(grabbing_index) < 0.005) {
                Points.at(grabbing_index).x = mouse_canvas.x;
                Points.at(grabbing_index).y = mouse_canvas.y;
            }
            
        } else grabbing = false;
    
        
        // draw curve
        const int curve_width = 4;
        ImColor color(GetStyle().Colors[ImGuiCol_PlotLines]);
        for (int i = 1; i < steps; i++) {
            ImVec2 p0 = { results.at(i - 1).x, 1 - results.at(i - 1).y };
            ImVec2 p1 = { results.at(i).x, 1 - results.at(i).y };            

            p0 = p0 * (bb.Max - bb.Min) + bb.Min;
            p1 = p1 * (bb.Max - bb.Min) + bb.Min;

            DrawList->AddLine(p0, p1, color, curve_width);
        }

        float grab_radius = ((bb.Max.x - bb.Min.x) + (bb.Max.y - bb.Min.y))/100.;

        // draw preview (cycles every 1s)
        static clock_t epoch = clock();
        ImVec4 white(GetStyle().Colors[ImGuiCol_Text]);
        for (int i = 0; i < 3; ++i) {
            double now = ((clock() - epoch) / (double)CLOCKS_PER_SEC);
            float delta = ((int) (now * 1000) % 1000) / 1000.f; delta += i / 3.f; if (delta > 1) delta -= 1;
            int idx = (int) (delta * steps);
            float evalx = results.at(idx).x; 
            float evaly = results.at(idx).y;
            ImVec2 p0 = ImVec2(evalx, 1) * (bb.Max - bb.Min) + bb.Min;
            ImVec2 p1 = ImVec2(0, 1 - evaly) * (bb.Max - bb.Min) + bb.Min;
            ImVec2 p2 = ImVec2(evalx, 1 - evaly) * (bb.Max - bb.Min) + bb.Min;
            DrawList->AddCircleFilled(p0, grab_radius / 2, ImColor(white));
            DrawList->AddCircleFilled(p1, grab_radius / 2, ImColor(white));
            DrawList->AddCircleFilled(p2, grab_radius / 2, ImColor(white));
        }

        // draw lines and grabbers
        float luma = IsItemActive() || IsItemHovered() ? 0.5f : 1.0f;
        ImVec4 blue(0.0f, 0.00f, 1.0f, luma), cyan(0.00f, 0.75f, 1.00f, luma);
        for (int i = 0; i < Points.size(); i++) {
            blue.z = 1 - Points.at(i).y;
            blue.x = Points.at(i).y;
            
            // Get the value in the curve
            ImVec2 p0 = results.at(nearest_val_index(results, Points.at(i)));
            p0.y = 1 - p0.y;
            p0 = p0 * (bb.Max - bb.Min) + bb.Min;

            ImVec2 p1 = ImVec2(Points.at(i).x, 1 - Points.at(i).y) * (bb.Max - bb.Min) + bb.Min;
            DrawList->AddLine(p0, p1, ImColor(white), 2);
            DrawList->AddCircleFilled(p1, grab_radius, ImColor(white));
            DrawList->AddCircleFilled(p1, grab_radius - (grab_radius/5.), ImColor(blue));
        }

        return true;
    }

    // Not necessary to use
    bool Envelope(std::vector<ImVec2> &Points, std::vector<ImVec2> &Results) {
        bool cond = true;
        Begin("Envelope", NULL, ImGuiWindowFlags_NoResize);
        cond = Bezier("Envelope", Points, Results);
        End();
        return cond;
    }
}