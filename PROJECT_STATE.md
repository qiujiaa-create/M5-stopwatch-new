# M5 StopWatch 当前状态

文档版本：0.2；最后更新：2026-09-16 22:39（中国时间）。

## 本目录是什么

本目录从 `../M5-stopwatch/source-snapshot-2026-09-16/source/` 完整复制而来，是新的**开发工作目录**。原快照保留不动，原 `vibecoding` 工作区也未修改。导入时，快照清单中的 4094 个源码与参考配置文件逐个通过 SHA-256 比对；其中 4092 个位于源码树，另外两个 `sdkconfig` 放在未纳入 Git 的 `local-reference/`。

本目录中的首次 Git 记录表示“2026-09-16 现状导入”，**不表示稳定版本或已经从这里刷入手表**。原主仓库基准为 `bba454ab720ee14548450cb74f87d7418ad2062c`，原小智嵌套仓库基准为 `49ac8a6da399f27a9546d4f73640b7f86c24bac6`；两者当时的未提交文件内容都已进入本目录。文件列表见 `migration/CHANGES_INCLUDED.md`，原盘点见 `migration/ORIGINAL_SNAPSHOT_README.md`。

## 代码与运行对象

| 路径 | 作用 | 状态 |
| --- | --- | --- |
| `firmware-stopwatch-idf/` | factory 槽的 StopWatch 主固件；三套 Codex 页面、BLE HID 与 Companion | 本目录构建成功；尚未刷写或实机验收 |
| `tools/typeless_bridge/` | macOS Bridge；本机额度采集、BLE 推送、Typeless 协调 | 本目录构建成功；本次迁移未替换当前安装的 App |
| `firmware-xiaozhi/` | ota_0 槽的小智 v2.2.6 及本机板卡适配 | 源码已导入；当前设备槽位需在再次切换后重核 |
| `server/cloud-sync/` | 可选同步服务 | 本机盘点时未启用 |
| `local-reference/` | 两套本机 `sdkconfig` 参考 | 未纳入 Git；转移机器时重新核对 |

Git 版本不包含 ChatGPT.app、Typeless.app、已安装的音频驱动、登录资料、运行日志、固件构建产物或手表闪存。构建产物只留在本机的忽略目录中。新目录不会自动改变 Mac 的 LaunchAgent 路径，也不会改变手表上的固件。

主固件的第三方 `components/` 已作为本机文件复制过来，但按项目原有 `.gitignore` 不纳入新 Git 仓库；其来源和版本记录在 `firmware-stopwatch-idf/repos.json`，新机器需运行 `fetch_repos.py` 恢复。`managed_components/`、`build/`、Bridge 的 `.build/` 也不纳入版本记录。

## 当前功能验收边界

- **已由用户在原安装组合上确认**：OpenWatcher V2 显示 5H 与周额度，并曾自动刷新为 5H 6% / 周 28%。这证明当时 Bridge 到手表的额度链路有效，不证明本目录产物已安装。
- **待修复并重新实机验收**：V2 顶部推理等级滑动、中心长按后的四向 Radial。设备发出事件或 Bridge 写日志均不能代替 Codex 界面的实际响应。
- **本目录已验证**：Bridge 独立编译成功；主固件使用 ESP-IDF 5.5.4 和 `local-reference/stopwatch-sdkconfig` 构建成功。主固件镜像约 5.9 MB，构建时提示 ota_0 的 3 MB 空间不足；主固件目标是 factory 槽。本次没有安装 Bridge 或刷写手表。
- **待在新目录验证**：两者的配套运行，以及刷入后手表三页和 A/B 语音链路的回归。小智固件未在本次迁移中构建。
- **小智**：独立启动槽；不要把它与主固件的三个 Codex 页面混为一项验收。

## 从这里开始

1. 先阅读本文件和 `WORKFLOW.md`。一个任务只由一个 AI 在一个工作目录里修改；其他 AI 用独立 worktree 审查或处理不同任务。
2. 第一个任务建议只修复 V2 推理滑动与四向 Radial。先复现触摸反馈，再逐层查固件事件、BLE HID、Mac/Codex 最终效果。
3. 本目录已通过 Bridge 与主固件构建。准备部署前重新构建并记录产物哈希，再确认设备分区、端口和当前启动槽，才决定是否刷写。
4. 只有手表和 Codex 端的操作结果都通过，才将该任务标为“实机通过”。再考虑把这套组合标成稳定基线。

构建入口与 ESP-IDF 版本见 `migration/ORIGINAL_SNAPSHOT_README.md`。本机参考配置在 `local-reference/`；它们不应直接作为公开配置提交。
