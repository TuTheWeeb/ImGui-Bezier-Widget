// ImGui Bezier widget. @r-lyeh, public domain
// v1.03: improve grabbing, confine grabbers to area option, adaptive size, presets, preview.
// v1.02: add BezierValue(); comments; usage
// v1.01: out-of-bounds coord snapping; custom border width; spacing; cosmetics
// v1.00: initial version
//
// [ref] http://robnapier.net/faster-bezier
// [ref] http://easings.net/es#easeInSine
//
// Usage:
// {  static float v[5] = { 0.390f, 0.575f, 0.565f, 1.000f }; 
//    ImGui::Bezier( "easeOutSine", v );       // draw
//    float y = ImGui::BezierValue( 0.5f, v ); // x delta in [0..1] range
// }

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include "imgui_vec2.hpp"
#include "interpolation.hpp"
#include <vector>
#include <time.h>

namespace ImGui
{
    float control[1] = {0.f};
    int steps[1] = {512};
    bool grabbing = false;
    int grabing_index = 0;

    int lesser_value_index(std::vector<float> vec) {
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

    int get_nearest_val_index(std::vector<ImVec2> points, ImVec2 point) {
        for (int i = 0; i < points.size(); i++) {
            if (point.x > points.at(i).x) {
                continue;
            }
            else {
                return i;
            }
        }
        return 0;
    }

    bool get_collision(ImRect rect, ImVec2 point) {
        if ((point.x < rect.Max.x) and (point.x > rect.Min.x) and (point.y > rect.Min.y) and (point.y < rect.Max.y)) return true;
        return false;
    }

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

    bool Bezier(const char *label, std::vector<ImVec2> &Points, std::vector<ImVec2> &Results) {//float P[4]) {
        // visuals
        enum { SMOOTHNESS = 256 }; // curve smoothness: the higher number of segments, the smoother curve
        enum { CURVE_WIDTH = 4 }; // main curved line width
        enum { LINE_WIDTH = 2 }; // handlers: small lines width
        enum { GRAB_RADIUS = 16 }; // handlers: circle radius
        enum { GRAB_BORDER = 2 }; // handlers: circle border width
        enum { AREA_CONSTRAINED = true }; // should grabbers be constrained to grid area?
        enum { AREA_WIDTH = 128 }; // area width in pixels. 0 for adaptive size (will use max avail width)

        // curve presets
        /*static struct { const char *name; float points[4]; } presets [] = {
            { "Linear", 0.000f, 0.000f, 1.000f, 1.000f },

            { "In Sine", 0.470f, 0.000f, 0.745f, 0.715f },
            { "In Quad", 0.550f, 0.085f, 0.680f, 0.530f },
            { "In Cubic", 0.550f, 0.055f, 0.675f, 0.190f },
            { "In Quart", 0.895f, 0.030f, 0.685f, 0.220f },
            { "In Quint", 0.755f, 0.050f, 0.855f, 0.060f },
            { "In Expo", 0.950f, 0.050f, 0.795f, 0.035f },
            { "In Circ", 0.600f, 0.040f, 0.980f, 0.335f },
            { "In Back", 0.600f, -0.28f, 0.735f, 0.045f },

            { "Out Sine", 0.390f, 0.575f, 0.565f, 1.000f },
            { "Out Quad", 0.250f, 0.460f, 0.450f, 0.940f },
            { "Out Cubic", 0.215f, 0.610f, 0.355f, 1.000f },
            { "Out Quart", 0.165f, 0.840f, 0.440f, 1.000f },
            { "Out Quint", 0.230f, 1.000f, 0.320f, 1.000f },
            { "Out Expo", 0.190f, 1.000f, 0.220f, 1.000f },
            { "Out Circ", 0.075f, 0.820f, 0.165f, 1.000f },
            { "Out Back", 0.175f, 0.885f, 0.320f, 1.275f },

            { "InOut Sine", 0.445f, 0.050f, 0.550f, 0.950f },
            { "InOut Quad", 0.455f, 0.030f, 0.515f, 0.955f },
            { "InOut Cubic", 0.645f, 0.045f, 0.355f, 1.000f },
            { "InOut Quart", 0.770f, 0.000f, 0.175f, 1.000f },
            { "InOut Quint", 0.860f, 0.000f, 0.070f, 1.000f },
            { "InOut Expo", 1.000f, 0.000f, 0.000f, 1.000f },
            { "InOut Circ", 0.785f, 0.135f, 0.150f, 0.860f },
            { "InOut Back", 0.680f, -0.55f, 0.265f, 1.550f },

            // easeInElastic: not a bezier
            // easeOutElastic: not a bezier
            // easeInOutElastic: not a bezier
            // easeInBounce: not a bezier
            // easeOutBounce: not a bezier
            // easeInOutBounce: not a bezier
        };*/

        // preset selector

        bool reload = false;

        int size = Points.size();

        // bezier widget
        ImGuiContext& g = *GImGui;
        const ImGuiStyle& Style = GetStyle();
        const ImGuiIO& IO = GetIO();
        ImDrawList* DrawList = GetWindowDrawList();
        ImGuiWindow* Window = GetCurrentWindow();
        ImGui::SetWindowSize(ImVec2(600, 400));


        if (Window->SkipItems)
            return false;

        std::vector<float> temp;
        for (ImVec2 point : Points) {
            temp.push_back(point.x);
            temp.push_back(point.y);
        }


        // header and spacing
        //int changed = false;
        // SliderFloat4(label, temp.data(), 0, 1, "%.3f", 1.0f);
        float min = 0, max = 1;
        int changed = 0;
        //int changed = SliderScalarN(label, ImGuiDataType_Float, temp.data(), temp.size(), &min, &max, "%.3f", 1.0f);

        int hovered = IsItemActive() || IsItemHovered(); // IsItemDragged() ?
        Dummy(ImVec2(0, 3));

        if (changed > 0) {
            for (int i = 0; i < Points.size(); i++) {
                //std::cout << temp.at(i) << " ";
                //if (Points.size() == temp.size()) std::cout << "Same size" << std::endl;
                Points.at(i).x = temp.at(i * 2);
                Points.at(i).y = temp.at(i * 2 + 1);
            }
        }
        //std::cout << std::endl;

        // prepare canvas
        ImVec2 Canvas = GetContentRegionAvail();
        Canvas.y = Canvas.y - 50;
        Canvas.x = Canvas.x - 50;
        //const float dim = avail;//steps[0] > 0 ? steps[0] : avail; //AREA_WIDTH > 0 ? AREA_WIDTH : avail;
        //ImVec2 Canvas(avail);

        ImRect bb(Window->DC.CursorPos, Window->DC.CursorPos + Canvas);
        ItemSize(bb);
        if (!ItemAdd(bb, NULL))
            return false;

        const ImGuiID id = Window->GetID(label);
        //hovered |= 0 != ItemHoverable(ImRect(bb.Min, bb.Min + ImVec2(avail, dim)), id, g.LastItemData.InFlags);
        hovered |= 0 != ItemHoverable(ImRect(bb.Min, bb.Min + Canvas), id, g.LastItemData.InFlags);

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
        SliderInt("Smoothness", steps, 512, 1024, "%d", 0);
        std::vector<ImVec2> results = ImGui::BezierValue(Points, steps[0]);
        Results = results;

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
            int grab_index = lesser_value_index(distance);

            if ((distance.at(grab_index) < 0.001) and (Points.size() > 2)) {
                Points.erase(Points.begin() + grab_index);
            } 
            else {
                Points.insert(Points.begin() + get_nearest_val_index(Points, mouse_canvas), mouse_canvas);
            }
        }
        
        if (IsMouseClicked(0) and get_collision(bb, mouse) and (!grabbing)) {
            grabing_index = lesser_value_index(distance);
            grabbing = true;
        }
        else if (IsMouseDown(0) and get_collision(bb, mouse)) {
            if (distance.at(grabing_index) < 0.005) {
                Points.at(grabing_index).x = mouse_canvas.x;
                Points.at(grabing_index).y = mouse_canvas.y;
                changed = true;
            }
            
            //grabbing = false;
        } else grabbing = false;
    
        
        // draw curve
        ImColor color(GetStyle().Colors[ImGuiCol_PlotLines]);
        for (int i = 1; i < steps[0]; i++) {
            ImVec2 p0 = { results.at(i - 1).x, 1 - results.at(i - 1).y };
            ImVec2 p1 = { results.at(i).x, 1 - results.at(i).y };            

            //if (i == steps[0] - 1) std::cout << results.at(i).y << std::endl;

            p0 = p0 * (bb.Max - bb.Min) + bb.Min;
            p1 = p1 * (bb.Max - bb.Min) + bb.Min;

            DrawList->AddLine(p0, p1, color, CURVE_WIDTH);
        }

        float grab_radius = ((bb.Max.x - bb.Min.x) + (bb.Max.y - bb.Min.y))/100.;

        // draw preview (cycles every 1s)
        static clock_t epoch = clock();
        ImVec4 white(GetStyle().Colors[ImGuiCol_Text]);
        for (int i = 0; i < 3; ++i) {
            double now = ((clock() - epoch) / (double)CLOCKS_PER_SEC);
            float delta = ((int) (now * 1000) % 1000) / 1000.f; delta += i / 3.f; if (delta > 1) delta -= 1;
            int idx = (int) (delta * steps[0]);
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
            ImVec2 p0 = results.at(get_nearest_val_index(results, Points.at(i)));
            p0.y = 1 - p0.y;
            p0 = p0 * (bb.Max - bb.Min) + bb.Min;

            ImVec2 p1 = ImVec2(Points.at(i).x, 1 - Points.at(i).y) * (bb.Max - bb.Min) + bb.Min;
            //DrawList->AddLine(p0, p1, ImColor(white), 2);
            DrawList->AddCircleFilled(p1, grab_radius, ImColor(white));
            DrawList->AddCircleFilled(p1, grab_radius - (grab_radius/5.), ImColor(blue));
        }

        return true;
    }

    bool Envelope(std::vector<ImVec2> &Points, std::vector<ImVec2> &Results) {
        bool cond = true;
        Begin("Envelope", NULL, ImGuiWindowFlags_NoResize);
        cond = Bezier("Envelope", Points, Results);
        End();
        return cond;
    }
}