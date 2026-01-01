#pragma once

// ============================================================================
// Genesis Engine GUI Widget Library
// Windows 7 style widgets with dark violet/purple theme and red accents
// ============================================================================

#include "Widget.h"
#include "Button.h"
#include "Panel.h"
#include "Label.h"
#include "Checkbox.h"
#include "Slider.h"
#include "TextField.h"

namespace Genesis {
namespace GUI {

// ============================================================================
// Widget Factory - Convenience functions for creating widgets
// ============================================================================
namespace WidgetFactory {

    inline std::shared_ptr<Button> CreateButton(const std::string& text, float x, float y,
                                                 float width = 100, float height = 28) {
        return std::make_shared<Button>(text, x, y, width, height);
    }

    inline std::shared_ptr<Panel> CreatePanel(const std::string& title, float x, float y,
                                               float width = 300, float height = 200) {
        return std::make_shared<Panel>(title, x, y, width, height);
    }

    inline std::shared_ptr<Label> CreateLabel(const std::string& text, float x, float y,
                                               float scale = 1.0f) {
        return std::make_shared<Label>(text, x, y, scale);
    }

    inline std::shared_ptr<Checkbox> CreateCheckbox(const std::string& label, float x, float y,
                                                     bool checked = false) {
        return std::make_shared<Checkbox>(label, x, y, checked);
    }

    inline std::shared_ptr<Slider> CreateSlider(float x, float y, float width,
                                                 float minVal = 0.0f, float maxVal = 1.0f,
                                                 float value = 0.5f) {
        return std::make_shared<Slider>(x, y, width, minVal, maxVal, value);
    }

    inline std::shared_ptr<TextField> CreateTextField(float x, float y, float width,
                                                        const std::string& text = "") {
        return std::make_shared<TextField>(x, y, width, text);
    }

} // namespace WidgetFactory

} // namespace GUI
} // namespace Genesis

