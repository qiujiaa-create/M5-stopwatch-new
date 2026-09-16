# 当前手表源码快照

整理时间：2026-09-16 22:20（中国时间）。本目录是从 `../vibecoding` **当前工作文件**导出的源码副本，包含当时尚未提交的改动；它不是仓库 `bba454a` 的原样检出，也不是已经发布的版本。原工作区未被清理或重置。

## 先看运行关系

```text
手表 ESP32-S3
├─ factory 0x20000：StopWatch 主固件 → Codex 的 Official V1 / OpenWatcher V2 / Codex Micro 三个页面
│  ├─ 标准 BLE HID：A/B 按键、Typeless 快捷键
│  ├─ Codex Vendor HID：Agent、推理 Encoder、Radial
│  └─ BLE Companion：额度面板、状态、可选麦克风音频
└─ ota_0 0x620000：独立的小智固件 v2.2.6，切换时重启

Mac
├─ ChatGPT.app 内的 Codex 桌面端：原生 Codex Micro HID 的接收方
├─ StopWatch BLE Bridge.app：读取本机 Codex 额度并通过 BLE 发给手表；处理 Companion/Typeless
├─ Typeless.app：由手表快捷键控制的输入应用
└─ Core Audio 驱动：M5StopWatchMic 与 BlackHole2ch 已安装；当前 Bridge 虚拟麦克风开关为关闭
```

手表上目前运行的是 **factory 的 StopWatch 主固件**：2026-09-16 17:37 回读的 `otadata` 为擦除态，17:42 只更新了 factory 应用区且刷写哈希通过；同一时段手表与 Bridge 的额度同步经过实机确认。小智源码属于另一个启动槽，并不构成 Codex 三个页面的实现。若之后在手表上切换过系统，需重新核对当前启动槽。

## 这份源码包含什么

| 本目录路径 | 用途 | 对应的当前对象 |
| --- | --- | --- |
| `source/firmware-stopwatch-idf/` | 主固件、三套 Codex 页面、BLE HID/Companion、硬件适配、分区表、测试和项目内第三方组件源码 | 手表 factory 槽；入口 `main/apps/app_codex/`，额度解析 `codex_quota_client.cpp`，页面 `view/view.cpp` |
| `source/firmware-xiaozhi/` | 上游 `xiaozhi-esp32` v2.2.6 加本机 StopWatch 板卡适配与双系统分区配置 | 手表 ota_0 槽；适配入口 `main/boards/m5stack/stopwatch/` |
| `source/tools/typeless_bridge/` | macOS Bridge、LaunchAgent 安装脚本、虚拟麦克风驱动源码与测试 | 正在运行的 `StopWatch BLE Bridge.app`；主入口 `stopwatch_ble_bridge.swift` |
| `source/server/cloud-sync/` | 可选的跨 Mac 同步服务源码 | 当前没有 `cloud-sync.json`，本机未启用这条服务链 |
| `source/docs/`、`source/assets-src/` | 项目说明、设计记录及未提交的图标素材 | 辅助构建与追溯，不是后台进程 |
| `reference-config/` | 两套固件当前的 `sdkconfig` 副本 | 构建参数参考；从源码树分离，避免误认为源文件 |

快照保留原目录结构，方便直接在 `source/` 下开发。主项目原分支是 `feat/codex-micro-dashboard`、基准提交 `bba454ab720ee14548450cb74f87d7418ad2062c`；小智嵌套仓库基准提交为 `49ac8a6da399f27a9546d4f73640b7f86c24bac6`，标签 `v2.2.6`。两者都有尚未提交的本地改动，已经作为文件内容进入本快照。
具体改动路径见 [CHANGES_INCLUDED.md](CHANGES_INCLUDED.md)。同级的 `../repo` 是另一份检出，不是当前手表与 Bridge 的源码来源，因此没有纳入。

## Mac 当前实际在用的程序

| 对象 | 实际位置或启动项 | 当前作用 |
| --- | --- | --- |
| Codex 桌面端 | `/Applications/ChatGPT.app` 内的 Codex 功能 | 接收手表的原生 Vendor HID；不属于本源码包 |
| StopWatch BLE Bridge | `~/Applications/StopWatch BLE Bridge.app/Contents/MacOS/stopwatch-ble-bridge` | 额度采集、BLE 面板推送、Companion 与 Typeless 协调；由 `dev.vibecoding.stopwatch-ble-bridge` LaunchAgent 启动 |
| Bridge watchdog | `com.liptox.stopwatch-ble-bridge.watchdog` LaunchAgent | 检查并恢复 Bridge；不是第二个额度采集进程 |
| Typeless | `/Applications/Typeless.app` | 接收手表的快捷键；应用本身及用户配置不在源码包内 |
| `M5StopWatchMic.driver`、`BlackHole2ch.driver` | `/Library/Audio/Plug-Ins/HAL/` | 已安装的音频驱动；当前 Bridge 配置 `virtualMicrophoneEnabled=false` |
| `codex-watch-companion --watch` | 当前没有对应运行进程或启动项 | 不参与本机现行额度同步 |

2026-09-16 22:14 的 Bridge 配置检查：`enableCodexQuota=true`、`quotaRefreshSeconds=120`、`virtualMicrophoneEnabled=false`、无 `cloud-sync.json`。额度来自本机 Codex 登录状态，经 Bridge → BLE 面板到手表；登录文件、令牌、运行配置、日志均**没有**复制进本目录。运行 PID、连接状态和额度百分比会随时间变化，不应当作源码版本号。

## 构建与复现

1. 安装 ESP-IDF **5.5.4** 及 ESP32-S3 工具链。原工作区的 `../esp-idf` 和 `../.espressif` 是本机工具环境，体积较大，未放进源码快照。
2. 主固件：将 `reference-config/stopwatch-sdkconfig` 复制为 `source/firmware-stopwatch-idf/sdkconfig`，进入该目录运行 `idf.py build`。项目内的 `components/` 已包含；`managed_components/` 由 ESP-IDF 依照清单下载或恢复。
3. 小智固件：将 `reference-config/xiaozhi-sdkconfig` 复制为 `source/firmware-xiaozhi/sdkconfig`，进入该目录运行 `idf.py -B build.sw build`。板卡与分区配置分别见 `main/boards/m5stack/stopwatch/`、`partitions/stopwatch_dual.csv`。同样需恢复 `managed_components/`。
4. Mac Bridge：在 `source/` 下运行 `tools/typeless_bridge/build_stopwatch_ble_bridge.sh`。其安装脚本会生成本机 App 与 LaunchAgent；单独的虚拟麦克风源码在 `tools/typeless_bridge/virtual_mic_driver/`。

构建输出不能直接证明设备行为。刷写前要核对设备当前启动槽和分区表；当前 factory 是 6 MiB、ota_0 是 3 MiB，二者是独立应用。`reference-config/` 记录的是本次本机参数，转移到其他机器后仍需确认工具链路径与硬件端口。

## 已核实状态与待处理项

- **已核实**：主固件应用镜像在 2026-09-16 17:42 刷入 factory 并通过哈希校验；Bridge 重新安装后运行。手表 V2 额度曾从 **5H 11% / 周 29%** 自动更新为 **5H 6% / 周 28%**，用户实机确认。
- **待修复**：用户随后报告 V2 的顶部推理等级滑动和中心四向 Radial 触控不可用。Bridge 收到过 `codex_reasoning:native_sync`，只说明设备发出了同步事件，**不能证明 Codex 的推理等级确实改变**；四向控制的最终效果也未完成实机验收。不要把源码里“已实现”或旧 README 的“已完成”理解为当前功能验收通过。
- **小智边界**：源码和双启动槽机制已存在；项目原记录显示小智曾启动并联网，但账号激活后的语音对话未在本次盘点中验证。当前手表运行的是 StopWatch factory。
- **版本边界**：原项目仍标 `StopWatch 0.10.7`、`Bridge 1.4.1`，而工作树包含这些版本号之后的未提交修改；应以本快照清单与实际构建哈希识别内容，不只看版本文本。

本次本机参考镜像的 SHA-256：factory 构建 `3bcaa60668a66bf7dbbdf0aec3a31db7bad02bb3aee3a553be12cd7c57eeaa38`；已安装 Bridge 可执行文件 `ef6106dbeb8684b8bad004d78781bda563ac25dc37f9945c358bb83a05ba1da0`。本目录不含这些可执行文件；小智本地 `build.sw` 镜像也未回读核对当前 ota_0，因此不将其哈希写成设备实装证明。

## 导出规则

保留了可编辑源码、项目内第三方源码、资源和说明。排除了各级 `.git/`、`.backup/`、`build/`、`build.sw/`、`.build/`、`managed_components/`、`node_modules/`、本机专用 `idf_env.sh`、运行凭据与日志。小智方案文档副本中的本机 Wi-Fi 名称和设备标识已遮盖；原工作区文件未修改。`SOURCE_MANIFEST.sha256` 用于核对本快照文件内容；它不表示设备固件已经由这个新目录重新构建。
