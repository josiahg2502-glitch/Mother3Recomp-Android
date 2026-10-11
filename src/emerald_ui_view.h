#pragma once
#include "emerald_extended_view.h"

namespace emerald {
// Move the game's live BG0 window rectangles, including their authored
// borders/text/cursors. No menu reconstruction or guest state changes.
class UiView {
public:
    void prepare(const ViewMemory& memory, const FieldView& field, int width, int height);
    int sample(int bg, int x, int y, int* source_x, int* source_y) const;
private:
    struct Rect { int x, y, w, h, dx, dy; };
    std::array<Rect, 32> windows_{};
    int count_ = 0;
    bool active_ = false;
};
}
