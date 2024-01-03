#pragma once
#include <vector>
#include "imgui.h"
#include "imgui_internal.h"

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

// Interpolate all points in the layer (used recursion but can be substituted for a loop and a vector that is copied every interation)
std::vector<ImVec2> layer_interpolation(std::vector<ImVec2> vec, float t) {
    std::vector<ImVec2> points;

    if (vec.size() < 2) return vec;

    for (int i = 1; i < vec.size(); i++) {
        ImVec2 p0 = vec.at(i - 1);
        ImVec2 p1 = vec.at(i);
        ImVec2 p2 = vec_lerp(p0, p1, t);
        points.push_back(p2);
    }

    if (points.size() >= 2) points = layer_interpolation(points, t);
    
    return points;
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