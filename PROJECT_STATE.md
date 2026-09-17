# M5 StopWatch 当前状态

文档版本：0.5；最后更新：2026-09-17 14:26。

## 本目录是什么

本目录从 `../M5-stopwatch/source-snapshot-2026-09-16/source/` 完整复制而来，是新的**开发工作目录**。原快照保留不动，原 `vibecoding` 工作区也未修改。导入时，快照清单中的 4094 个源码与参考配置文件逐个通过 SHA-256 比对；其中 4092 个位于源码树，另外两个 `sdkconfig` 放在未纳入 Git 的 `local-reference/`。

本目录中的首次 Git 记录表示“2026-09-16 现状导入”，**不表示稳定版本或已经从这里刷入手表**。原主仓库基准为 `bba454ab720ee14548450cb74f87d7418ad2062c`，原小智嵌套仓库基准为 `49ac8a6da399f27a9546d4f73640b7f86c24bac6`；两者当时的未提交文件内容都已进入本目录。文件列表见 `migration/CHANGES_INCLUDED.md`，原盘点见 `migration/ORIGINAL_SNAPSHOT_README.md`。

## 代码与运行对象

| 路径 | 作用 | 状态 |
| --- | --- | --- |
| `firmware-stopwatch-idf/` | factory 槽的 StopWatch 主固件；Codex Micro 与 OpenWatcher V2、BLE HID 与 Companion | 本目录构建并刷入 factory；新固件的页面与输入操作尚待用户实机验收 |
| `tools/typeless_bridge/` | macOS Bridge；本机额度采集、BLE 推送、Typeless 协调 | 本目录构建成功；本次迁移未替换当前安装的 App |
| `firmware-xiaozhi/` | ota_0 槽的小智 v2.2.6 及本机板卡适配 | 源码已导入；当前设备槽位需在再次切换后重核 |
| `server/cloud-sync/` | 可选同步服务 | 本机盘点时未启用 |
| `local-reference/` | 两套本机 `sdkconfig` 参考 | 未纳入 Git；转移机器时重新核对 |

Git 版本不包含 ChatGPT.app、Typeless.app、已安装的音频驱动、登录资料、运行日志、固件构建产物或手表闪存。构建产物只留在本机的忽略目录中。新目录不会自动改变 Mac 的 LaunchAgent 路径，也不会改变手表上的固件。

主固件的第三方 `components/` 已作为本机文件复制过来，但按项目原有 `.gitignore` 不纳入新 Git 仓库；其来源和版本记录在 `firmware-stopwatch-idf/repos.json`，新机器需运行 `fetch_repos.py` 恢复。`managed_components/`、`build/`、Bridge 的 `.build/` 也不纳入版本记录。

## 当前功能验收边界

- **已由用户在原安装组合上确认**：OpenWatcher V2 显示 5H 与周额度，并曾自动刷新为 5H 6% / 周 28%。这证明当时 Bridge 到手表的额度链路有效，不证明本目录产物已安装。
- **2026-09-17 在当前安装组合上由用户实机确认**：V2 中心长按后的上、下、左、右四向 Radial，手表反馈和 Codex 动作均正常。此结果仍不代表本目录新构建产物已刷入手表。
- **2026-09-17 在当前安装组合上由用户实机确认**：V2 顶部向左、向右滑动分别降低、提高 Codex 推理等级。故障原因是本机 Codex Micro 的 `encoderMode` 为 `composer-navigation`，原生 Encoder 事件被用作上下导航。已将 `~/.codex/config.toml` 改为 `reasoning` 并保留原配置备份；运行中的 Codex 还需在「设置 → Codex Micro → Knob」选择 `Reasoning only` 才立即生效。Bridge 的 `codex_reasoning:native_sync` 日志本身不能代替 Codex 界面的操作结果。
- **V2 页面说明**：底部的两个小圆点是 BLE 与 Wi-Fi 状态；线与圆点组成的图形是同步装饰。V2 当前没有第二页或页切换手势。Codex Micro 与 OpenWatcher V2 通过手表 Setup → Device → Codex Theme 选择。
- **2026-09-17 早先任务的构建与刷写记录**：从 Codex App 移除 Official V1 及其专属图片、逐帧宠物动画；原有 `official_v1` 设置回退到 Codex Micro。ESP-IDF 5.5.4 构建通过，应用镜像从 5,904,864 降至 4,025,456 字节，SHA-256 `8778bf86dfde834b6460f75e9774c9f41924b495eabfc1d1876bb1eae3a3cdf8`。当时设备分区表与构建完全一致、otadata 空白且从 factory 启动；仅执行 `app-flash` 并通过写后 hash 校验与串口启动、BLE 重新订阅。此后仓库已有新提交，不能把该哈希当成当前 HEAD 的构建产物；当时未安装本目录 Bridge、未改小智槽与持久数据分区。
- **待实机操作验证**：确认设置只列出两套主题、当前 Codex 页面能正常打开；再回归 V2 的 A/B 语音、四 Agent、推理滑动与四向 Radial。串口启动成功不代替这些操作结果。小智固件未在本次迁移中构建。
- **容量边界**：缩小的是 factory 应用镜像，在 6 MiB factory 分区内增加约 1.79 MiB 余量；4 MiB FAT `storage` 分区容量未变化。
- **小智**：独立启动槽，不是 Codex 页面。factory 的 Launcher/Setup 提供切到 ota_0 的确认入口，并先检查目标镜像；小智板卡实现提供 B 键长按 3 秒或 MCP 工具切回 factory。当前设备槽位及小智语音对话仍需单独复核。
- **音频发送策略**：factory 仅在录音时采集并发送 20 ms IMA-ADPCM 帧；发送前最多等待 80 ms 的 NimBLE mbuf 恢复，保留至少 4 个 mbuf 给控制流量。丢帧由序号和计数暴露；Bridge 对明确中断结束本次听写并提示重录。此项是代码现状，不等同于本次新产物的实机音频验收。

## 从这里开始

1. 先阅读本文件和 `WORKFLOW.md`。一个任务只由一个 AI 在一个工作目录里修改；其他 AI 用独立 worktree 审查或处理不同任务。
2. V2 推理滑动与四向 Radial 已在当前安装组合上通过用户实机验证。后续新构建产物仍要逐层查触摸反馈、固件事件、BLE HID 与 Mac/Codex 最终效果。
3. 本目录曾通过 Bridge 与主固件构建，且主固件曾刷入 factory。其后仓库仍有提交；下次部署前须重新构建并记录产物哈希、确认设备分区、端口和当前启动槽。
4. 只有手表和 Codex 端的操作结果都通过，才将该任务标为“实机通过”。再考虑把这套组合标成稳定基线。

构建入口与 ESP-IDF 版本见 `migration/ORIGINAL_SNAPSHOT_README.md`。本机参考配置在 `local-reference/`；它们不应直接作为公开配置提交。
