Just need to add the files to your directory and include imgui_bezier.hpp in your main file, and certify that you have imgui.

Can be used this way:

```
// The points to apply the bezier function
std::vector<ImVec2> Points = {{0.2, 0.8}, {0.5, 0.1}};

// Where the resulting discrete function for the points
std::vector<ImVec2> Results;

ImGui::Bezier_Widget(Points, Results);

```

or:

```

std::vector<ImVec2> Points = {{0.2, 0.8}, {0.5, 0.1}};

std::vector<ImVec2> Results;

ImGui::Bezier("Label", Points, Results);

```
