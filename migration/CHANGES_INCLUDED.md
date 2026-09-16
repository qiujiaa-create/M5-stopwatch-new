# 快照中包含的未提交改动

以下是导出时原工作区的 Git 状态，用于区分本快照与上游提交。`M` 是已有文件修改，`??` 是新增且尚未纳入 Git 的文件或目录。快照复制的是这些路径下的实际文件内容。

## StopWatch 主仓库（`vibecoding`）

```text
 M docs/AGENT_DEVELOPMENT_GUIDE.md
 M docs/QUOTA.md
 M docs/requirements/codex-micro-dashboard-redesign-discussion-log.md
 M firmware-stopwatch-idf/main/CMakeLists.txt
 M firmware-stopwatch-idf/main/apps/app_codex/app_codex.cpp
 M firmware-stopwatch-idf/main/apps/app_codex/app_codex.h
 M firmware-stopwatch-idf/main/apps/app_codex/codex_quota_client.cpp
 M firmware-stopwatch-idf/main/apps/app_codex/codex_quota_client.h
 M firmware-stopwatch-idf/main/apps/app_codex/view/view.cpp
 M firmware-stopwatch-idf/main/apps/app_codex/view/view.h
 M firmware-stopwatch-idf/main/apps/app_setup/app_setup.cpp
 M firmware-stopwatch-idf/main/apps/app_setup/workers/device.cpp
 M firmware-stopwatch-idf/main/apps/app_setup/workers/workers.h
 M firmware-stopwatch-idf/main/apps/apps.h
 M firmware-stopwatch-idf/main/assets/assets.h
 M firmware-stopwatch-idf/main/hal/ble_bridge.cpp
 M firmware-stopwatch-idf/main/hal/ble_bridge.h
 M firmware-stopwatch-idf/main/hal/codex_micro_hid.cpp
 M firmware-stopwatch-idf/main/hal/codex_micro_hid.h
 M firmware-stopwatch-idf/main/hal/hal_ioe.cpp
 M firmware-stopwatch-idf/main/main.cpp
 M firmware-stopwatch-idf/partitions.csv
 M tools/typeless_bridge/README.md
 M tools/typeless_bridge/install_launch_agent.sh
 M tools/typeless_bridge/stopwatch_ble_bridge.swift
?? assets-src/
?? docs/xiaozhi-integration-plan.md
?? firmware-stopwatch-idf/main/apps/app_setup/workers/switch_system.cpp
?? firmware-stopwatch-idf/main/apps/app_xiaozhi/
?? firmware-stopwatch-idf/main/assets/images/icon_xiaozhi.c
?? firmware-xiaozhi/
?? tools/draw_xiaozhi_icon.py
?? tools/gen_lvgl_icon.py
```

## 小智嵌套仓库（`vibecoding/firmware-xiaozhi`）

```text
 M main/CMakeLists.txt
 M main/Kconfig.projbuild
?? main/boards/m5stack/
?? partitions/stopwatch_dual.csv
?? sdkconfig.defaults.stopwatch
```

`firmware-xiaozhi/` 在主仓库中显示为未跟踪目录，但本快照已展开复制其源码。原工作区的 `.backup/`、`idf_env.sh` 和小智 `build.sw/` 被明确排除。
