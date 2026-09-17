#pragma once

#include "focus_timer.h"
#include "view.h"
#include <apps/common/key_manager/key_manager.h>
#include <mooncake.h>
#include <memory>

class AppFocus : public mooncake::AppAbility {
public:
    AppFocus();
    void onCreate() override;
    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    void actSingle(focus::Mode mode, uint64_t nowMs);
    void actReset(focus::Mode mode);

    std::unique_ptr<input::KeyManager> _keyManager;
    std::unique_ptr<focus::FocusView> _view;
    bool _chordActive = false;
};
