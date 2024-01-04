#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include "interpolation.hpp"
#include <vector>

namespace ImGui
{
    // Global variables necessary for the widget loop
    int steps = 512;
    bool grabbing = false;
    int grabbing_index = 0;

    std::vector<float> get_distance_from_mouse(std::vector<ImVec2> Points, ImVec2 mouse, ImVec2 Canvas, ImRect rect_frame) {
        std::vector<float> distance;
        
        // Get the distance from the mouse to all points
        for (int i = 0; i < Points.size(); ++i) {
            ImVec2 pos = ImVec2(Points.at(i).x, 1 - Points.at(i).y) * (rect_frame.Max - rect_frame.Min) + rect_frame.Min;

            distance.push_back((pos.x - mouse.x)/Canvas.x * (pos.x - mouse.x)/Canvas.x + (pos.y - mouse.y)/Canvas.y * (pos.y - mouse.y)/Canvas.y);
        }

        return distance;
    }

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

    // Get the nearest point in a points vector, * consider the point that have a x component tiniest than the point
    int nearest_val_index(std::vector<ImVec2> points, ImVec2 point) {
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

    // Return true if a point is inside a rectangle
    bool get_collision(ImRect rect, ImVec2 point) {
        if ((point.x < rect.Max.x) and (point.x > rect.Min.x) and (point.y > rect.Min.y) and (point.y < rect.Max.y)) return true;

        return false;
    }

    // Get a any size quantity of points and set its start and end at 0 and 1
    std::vector<ImVec2> get_bezier_results(std::vector<ImVec2> Points, int steps) {
        // Set the start and end point
        ImVec2 start = {0,0}, end = {1,1};

        Points.insert(Points.begin(), start);
        Points.push_back(end);

        std::vector<ImVec2> results;
        results = vector_interpolation(Points, steps);
        
        // Compensate for possible incompletness
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
        
        // Can be changed (if the size, is too small can generate bugs)
        SetWindowSize(ImVec2(600, 400));

        if (Window->SkipItems) return false;

        // header and spacing
        int hovered = IsItemActive() || IsItemHovered();

        // prepare canvas
        ImVec2 Canvas = GetContentRegionAvail();
        Canvas.y = Canvas.y - 50;
        Canvas.x = Canvas.x - 50;

        // Create the rectangle frame
        ImRect rect_frame(Window->DC.CursorPos, Window->DC.CursorPos + Canvas);
        ItemSize(rect_frame);
        if (!ItemAdd(rect_frame, NULL))
            return false;

        // Handle the moving of the window
        const ImGuiID id = Window->GetID(label);
        hovered |= 0 != ItemHoverable(ImRect(rect_frame.Min, rect_frame.Min + Canvas), id, g.LastItemData.InFlags);

        // Render the rect_frame
        RenderFrame(rect_frame.Min, rect_frame.Max, GetColorU32(ImGuiCol_FrameBg, 1), true, Style.FrameRounding);

        // Add background grid to the drawlist
        for (int i = 0, j = 0; i < Canvas.x; i += (Canvas.x / 4), j += (Canvas.y / 4)) {
            // X component
            DrawList->AddLine(
                ImVec2(rect_frame.Min.x + i, rect_frame.Min.y),
                ImVec2(rect_frame.Min.x + i, rect_frame.Max.y),
                GetColorU32(ImGuiCol_TextDisabled)
            );

            // Y component
            DrawList->AddLine(
                ImVec2(rect_frame.Min.x, rect_frame.Min.y + j),
                ImVec2(rect_frame.Max.x, rect_frame.Min.y + j),
                GetColorU32(ImGuiCol_TextDisabled)
            );
        }

        // Evaluate the curve and set the smoothness
        SliderInt("Smoothness", &steps, 512, 2048, "%d", 0);
        std::vector<ImVec2> results = get_bezier_results(Points, steps);
        Results = results;

        // Let the programmer handle the close
        if (Button("Close", {80,20})) return false;
        
        // Mouse translation
        ImVec2 mouse = GetIO().MousePos;

        ImVec2 mouse_canvas = (mouse - rect_frame.Min)/(rect_frame.Max - rect_frame.Min);
        mouse_canvas.y = (mouse_canvas.y - 1) * -1;

        std::vector<float> distance = get_distance_from_mouse(Points, mouse, Canvas, rect_frame);

        // Handle Points / grabbers
        // add a new Point / grabber or remove an already existing one
        if (IsMouseClicked(1) and get_collision(rect_frame, mouse)) {
            int grabb_index = tiniest_value_index(distance);

            if ((distance.at(grabb_index) < 0.001) and (Points.size() > 2)) {
                Points.erase(Points.begin() + grabb_index);
            } 
            else {
                Points.insert(Points.begin() + nearest_val_index(Points, mouse_canvas), mouse_canvas);
            }
        }

        // Move points / grabbers
        if (IsMouseClicked(0) and get_collision(rect_frame, mouse) and (!grabbing)) {
            grabbing_index = tiniest_value_index(distance);
            grabbing = true;
        }
        else if (IsMouseDown(0) and get_collision(rect_frame, mouse)) {
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

            p0 = p0 * (rect_frame.Max - rect_frame.Min) + rect_frame.Min;
            p1 = p1 * (rect_frame.Max - rect_frame.Min) + rect_frame.Min;

            DrawList->AddLine(p0, p1, color, curve_width);
        }

        // Get a percentage of the rect_frame
        float grab_radius = ((rect_frame.Max.x - rect_frame.Min.x) + (rect_frame.Max.y - rect_frame.Min.y))/100.;

        // draw lines and grabbers
        float brightness = IsItemActive() || IsItemHovered() ? 0.5f : 1.0f;
        ImColor white(GetStyle().Colors[ImGuiCol_Text]);
        ImVec4 grabber_color(0.0f, 0.00f, 1.0f, brightness);
        for (int i = 0; i < Points.size(); i++) {
            grabber_color.z = 1 - Points.at(i).y;
            grabber_color.x = Points.at(i).y;
            
            // Get the value in the curve
            ImVec2 p0 = results.at(nearest_val_index(results, Points.at(i)));
            p0.y = 1 - p0.y;
            p0 = p0 * (rect_frame.Max - rect_frame.Min) + rect_frame.Min;

            ImVec2 p1 = ImVec2(Points.at(i).x, 1 - Points.at(i).y) * (rect_frame.Max - rect_frame.Min) + rect_frame.Min;
            DrawList->AddLine(p0, p1, ImColor(white), 2);
            DrawList->AddCircleFilled(p1, grab_radius, white);
            DrawList->AddCircleFilled(p1, grab_radius - (grab_radius/5.), ImColor(grabber_color));
        }

        return true;
    }

    // Not necessary to use
    bool Bezier_Widget(std::vector<ImVec2> &Points, std::vector<ImVec2> &Results) {
        bool cond = true;

        Begin("Bezier_Widget", NULL, ImGuiWindowFlags_NoResize);
        
        cond = Bezier("Bezier_Widget", Points, Results);
        
        End();
        
        return cond;
    }
}
