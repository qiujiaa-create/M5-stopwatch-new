#include "workers.h"

#include <apps/common/launcher_shortcuts/launcher_shortcuts.h>
#include <assets/assets.h>
#include <hal/hal.h>
#include <mooncake_log.h>
#include <vector>

using namespace smooth_ui_toolkit::lvgl_cpp;
using namespace setup_workers;

namespace {

constexpr std::string_view kTag = "Setup-QuickLaunch";

void applySelectorStyle(Roller& roller)
{
    roller.setBgColor(lv_color_hex(0x171717), LV_PART_MAIN);
    roller.setBgOpa(LV_OPA_COVER, LV_PART_MAIN);
    roller.setBorderWidth(0, LV_PART_MAIN);
    roller.setRadius(24, LV_PART_MAIN);
    lv_obj_set_style_text_font(roller.get(), &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(roller.get(), lv_color_hex(0x808080), LV_PART_MAIN);
    lv_obj_set_style_text_align(roller.get(), LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    roller.setBgColor(lv_color_hex(0x255B4A), LV_PART_SELECTED);
    roller.setBgOpa(LV_OPA_COVER, LV_PART_SELECTED);
    roller.setBorderWidth(0, LV_PART_SELECTED);
    lv_obj_set_style_text_font(roller.get(), &lv_font_montserrat_24, LV_PART_SELECTED);
    lv_obj_set_style_text_color(roller.get(), lv_color_hex(0xFFFFFF), LV_PART_SELECTED);
    lv_obj_set_style_text_align(roller.get(), LV_TEXT_ALIGN_CENTER, LV_PART_SELECTED);
}

std::vector<std::string> targetOptions()
{
    std::vector<std::string> options;
    options.reserve(launcher_shortcuts::SelectableTargets.size());
    for (const auto target : launcher_shortcuts::SelectableTargets) {
        options.emplace_back(launcher_shortcuts::displayName(target));
    }
    return options;
}

}  // namespace

namespace setup_workers {

class QuickLaunchWorker::QuickLaunchView {
public:
    explicit QuickLaunchView(const launcher_shortcuts::Config& initialConfig) : _config(initialConfig)
    {
        _panel = std::make_unique<Container>(lv_screen_active());
        _panel->align(LV_ALIGN_CENTER, 0, 0);
        _panel->setSize(466, 466);
        _panel->setRadius(0);
        _panel->setBorderWidth(0);
        _panel->setPaddingAll(0);
        _panel->setBgColor(lv_color_hex(0x000000));
        _panel->setBgOpa(LV_OPA_COVER);
        _panel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

        _title = std::make_unique<Label>(_panel->get());
        _title->setText("Quick Launch");
        _title->setTextFont(&MontserratSemiBold26);
        _title->setTextColor(lv_color_hex(0xFFFFFF));
        _title->align(LV_ALIGN_TOP_MID, 0, 42);

        _a_label = createLabel("Hold A", -96);
        _b_label = createLabel("Hold B", 96);

        const auto options = targetOptions();
        _a_selector = createSelector(options, -96, launcher_shortcuts::targetIndex(_config.longA));
        _b_selector = createSelector(options, 96, launcher_shortcuts::targetIndex(_config.longB));

        _a_selector->onValueChanged().connect([this](uint32_t value) {
            _config.longA = launcher_shortcuts::targetAt(value);
        });
        _b_selector->onValueChanged().connect([this](uint32_t value) {
            _config.longB = launcher_shortcuts::targetAt(value);
        });

        _ok_button = std::make_unique<Button>(_panel->get());
        _ok_button->align(LV_ALIGN_CENTER, 0, 178);
        _ok_button->setSize(300, 86);
        _ok_button->setRadius(43);
        _ok_button->setBorderWidth(0);
        _ok_button->setShadowWidth(0);
        _ok_button->setBgColor(lv_color_hex(0x4AD78C));
        _ok_button->label().setText("SAVE");
        _ok_button->label().setTextFont(&lv_font_montserrat_28);
        _ok_button->label().setTextColor(lv_color_hex(0x0F5831));
        _ok_button->label().align(LV_ALIGN_CENTER, 0, 0);
        _ok_button->onClick().connect([this]() { _save_requested = true; });
    }

    const launcher_shortcuts::Config& config() const
    {
        return _config;
    }

    bool consumeSaveRequested()
    {
        const bool requested = _save_requested;
        _save_requested = false;
        return requested;
    }

private:
    std::unique_ptr<Label> createLabel(const char* text, int x)
    {
        auto label = std::make_unique<Label>(_panel->get());
        label->setText(text);
        label->setTextFont(&lv_font_montserrat_22);
        label->setTextColor(lv_color_hex(0xB8B8B8));
        label->align(LV_ALIGN_TOP_MID, x, 86);
        return label;
    }

    std::unique_ptr<Roller> createSelector(const std::vector<std::string>& options, int x,
                                           std::size_t selectedIndex)
    {
        auto selector = std::make_unique<Roller>(_panel->get());
        selector->align(LV_ALIGN_CENTER, x, -8);
        selector->setSize(178, 220);
        selector->setOptions(options, LV_ROLLER_MODE_NORMAL);
        selector->setVisibleRowCount(3);
        selector->setSelected(static_cast<uint32_t>(selectedIndex), LV_ANIM_OFF);
        applySelectorStyle(*selector);
        return selector;
    }

    std::unique_ptr<Container> _panel;
    std::unique_ptr<Label> _title;
    std::unique_ptr<Label> _a_label;
    std::unique_ptr<Label> _b_label;
    std::unique_ptr<Roller> _a_selector;
    std::unique_ptr<Roller> _b_selector;
    std::unique_ptr<Button> _ok_button;
    launcher_shortcuts::Config _config;
    bool _save_requested = false;
};

}  // namespace setup_workers

QuickLaunchWorker::QuickLaunchWorker()
{
    mclog::tagInfo(kTag, "start quick launch worker");
    _view = std::make_unique<QuickLaunchView>(launcher_shortcuts::loadConfig());
}

void QuickLaunchWorker::update()
{
    if (consumeGlobalExitShortcut()) {
        return;
    }

    if (_view && _view->consumeSaveRequested()) {
        launcher_shortcuts::saveConfig(_view->config());
        GetHAL().vibrate(35, 100);
        _is_done = true;
    }
}

QuickLaunchWorker::~QuickLaunchWorker() = default;
