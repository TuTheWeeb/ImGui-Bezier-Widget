<h2>ImGui Bezier Widget</h2>

Just need to add the files to your directory and include imgui_bezier.hpp in your main file, and certify that you have imgui.

![Alt Text](https://github.com/TuTheWeeb/ImGui-Bezier-Widget/blob/main/BezierWidget.gif)

<h3>Usability:</h3>
    You can change the smoothness of the curve by changing the smoothness slider, if you click with the right mouse button you can add or exclude a Point/Grabber, if you click and press with left mouse button you can change the grabber position, the close button only makes the function returns false.

To incorporate in your code you need to use the ImGui::Bezier_Widget function or create a window and add the curve editor with ImGui::Bezier.

<h3>With ImGui::Bezier_Widget:</h3>

```CPP
// The points to apply the bezier function
std::vector<ImVec2> Points = {{0.2, 0.8}, {0.5, 0.1}};

// Where the resulting discrete function for the points is stored
std::vector<ImVec2> Results;

ImGui::Bezier_Widget(Points, Results);

```

<h3>With ImGui::Bezier:</h3>

```CPP

std::vector<ImVec2> Points = {{0.2, 0.8}, {0.5, 0.1}};

std::vector<ImVec2> Results;

ImGui::Begin("Window);
...

ImGui::Bezier("Label", Points, Results);

...
ImGui::End();

```

