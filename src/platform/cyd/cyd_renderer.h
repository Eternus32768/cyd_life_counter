#pragma once

// ui::DisplayRenderer の CYD 実装。フォントとピクセル寸法の決定はこの層で行う。

#include "../../ui/display_renderer.h"
#include "lgfx_display.h"

namespace life::platform::cyd {

class CydRenderer : public ui::DisplayRenderer {
public:
    explicit CydRenderer(CydDisplay& display);

    void begin_frame() override;
    void end_frame() override;
    void fill_rect(const ui::Rect& rect, const ui::Color& color) override;
    void draw_border(const ui::Rect& rect, const ui::Color& color) override;
    void draw_text(const ui::Rect& rect, const char* text, ui::TextRole role,
                   const ui::Color& color, ui::Orientation orientation) override;

private:
    CydDisplay& display_;
};

}  // namespace life::platform::cyd
