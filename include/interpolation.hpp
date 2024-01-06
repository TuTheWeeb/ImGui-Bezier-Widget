// Copyright (c) 2024 TuTheWeeb
// See LICENSE.txt for copyright and licensing details (standard MIT License).

#pragma once
#include <vector>
#include "imgui.h"
#include "imgui_internal.h"

// Lerp functions
double lerp(double p0, double p1, double t) {
  return p0 + t * (p1 - p0);
}

ImVec2 vec_lerp(ImVec2 p0, ImVec2 p1, double t) {
  ImVec2 temp;
  
  temp.x = lerp(p0.x, p1.x, t);
  temp.y = lerp(p0.y, p1.y, t);

  return temp;
}

// Interpolate all points in the layer
std::vector<ImVec2> layer_interpolation(std::vector<ImVec2> vec, float t) {
    std::vector<ImVec2> points;

    while(vec.size() >= 2) {
        for (int i = 1; i < vec.size(); i++) {
            ImVec2 p0 = vec.at(i - 1);
            ImVec2 p1 = vec.at(i);
            ImVec2 p2 = vec_lerp(p0, p1, t);
            points.push_back(p2);
        }
        vec = points;
        points.clear();
    }
    
    return vec;
}

// Get all interpolated points
std::vector<ImVec2> vector_interpolation(std::vector<ImVec2> vec, int steps) {
    std::vector<ImVec2> points;

    for (int i = 0; i < steps; i++) {
        float t = i;
        t = t/(steps + 1);
        
        ImVec2 point = layer_interpolation(vec, t).at(0);
        
        points.push_back(point);
    }
    return points;
}
