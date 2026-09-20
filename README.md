# M5 StopWatch：Codex 控制器、Focus 计时与小智双固件

文档版本：0.9；最后更新：2026-09-20 09:47。

当前源码版本：**固件 v0.10.7 / macOS Bridge v1.4.1**。版本号是源码元数据，具体设备安装版本以实机验证为准。

![Firmware v0.10.7](https://img.shields.io/badge/firmware-v0.10.7-6f5cff)

本仓库整合了 M5Stack StopWatch 主固件、独立的小智固件、macOS BLE Bridge 和 `M5 StopWatch Mic` 虚拟麦克风。设备使用 466×466 圆形 AMOLED。当前开发与实机验收边界见 [PROJECT_STATE.md](PROJECT_STATE.md)；历史版本记录见 [CHANGELOG.md](CHANGELOG.md)。

## 当前组成

| 路径 | 运行位置 | 当前作用 |
| --- | --- | --- |
| `firmware-stopwatch-idf/` | 手表 `factory` 槽 | Launcher、设置、Codex App、独立 Focus 计时、标准 BLE HID、Codex Vendor HID、BLE 麦克风 |
| `firmware-xiaozhi/` | 手表 `ota_0` 槽 | 基于小智 v2.2.6 的 StopWatch 板卡适配和独立语音系统 |
| `tools/typeless_bridge/` | macOS | BLE Companion、额度/状态同步、Typeless 协调、音频解码与虚拟输出 |
| `tools/typeless_bridge/virtual_mic_driver/` | macOS | `M5 StopWatch Mic` Core Audio 设备 |
| `server/cloud-sync/` | 自行部署，可选 | 跨 Mac 额度和活动统计 |

这是一台设备上的两个可启动系统。小智通过切换启动槽进入，不与 Codex 页面同时运行。主固件中，Codex Micro 和 OpenWatcher V2 是 **同一个 Codex App 的两套主题**，可在 `Setup → Device → Codex Theme` 选择。Official V1 / Classic Pet 及其逐帧动画已从当前主固件移除；旧主题设置回退到 Codex Micro。

主固件的 Launcher/Setup 提供“切换到小智”确认页，检查 `ota_0` 有可启动镜像后才切换并重启。小智侧可长按 B 约 3 秒，或使用其返回 StopWatch 的 MCP 工具，切回 `factory`。当前硬件上安装的槽位及小智语音对话效果需按 [项目状态](PROJECT_STATE.md) 单独复核。双固件使用相同分区布局；`factory` 为 6 MiB，`ota_0` 为 3 MiB，`storage` 为 4 MiB。刷写前以两侧分区 CSV 和设备实际分区表为准。

## Codex 页面和输入

- **Codex Micro** 显示原生 Agent 状态、会话状态、5H 与周额度；5H 缺失时显示等待或 `--`，不会拿周额度冒充。
- **OpenWatcher V2** 显示周剩余大数字、Today 消耗、独立 5H 剩余与重置倒计时；5H 文字上方的横向进度条按剩余百分比填充，并从低额红色平滑过渡到高额绿色。Today 的引导折线向上指向周额度剩余数字，明确 Today 表示全周额度的当日消耗比例。页面同时显示最近四小时的 24 格活动、四个 Agent 点和连接状态。录音、处理、中断和 `COMPACT SOON` 使用状态文字；底部线点是装饰，BLE/Wi-Fi 点表示连接状态。V2 没有第二页切换手势。
- 两套主题共用 A/B 语音状态机。默认 A 对应 `F19`，B 在录音/识别过程中执行停止，回到 Ready 才执行确认/发送。
- 四个 Agent 点支持触碰预览与 280 ms 长按提交；执行中的 Working 小圆点持续闪动，任务执行期间保持屏幕不自动变暗、息屏或关机。顶部左右滑动发送原生 Encoder 事件；中心长按 280 ms 后可输出四向 Radial，松手归中。Agent 灯效用于显示，能否发送由 Vendor HID 就绪状态决定。

Codex 当前页面只有**被动显示**的额度状态；5H 低额不会另行发声或振动提醒。Bridge 可在 Mac 本机采集 Codex 额度，经 BLE 推送 5H（18000 秒）与周（604800 秒）窗口。缺失、过期或异常数据保留未知状态。录音期间新面板暂存，回到空闲后补发。Today 是从北京时间 08:00 起的周额度百分点消耗，并非官方独立的每日额度。细节见 [额度机制](docs/QUOTA.md)。

## Focus 本地专注计时

Launcher 中的 **Focus** 是独立应用，计时完全在手表本地进行，不连接 Mac 上的 TickTick。A 键控制正计时，秒数右侧显示十分之一秒；B 键控制倒计时，默认 25 分钟。切换到倒计时后，点击主数字的分钟区域可设置 1–60 分钟，保存后记住上次时长。在当前模式按另一按键时只切换到对应页面；进入目标页面后再按一次对应键才开始、暂停或继续。仅在 `PAUSED` 时长按对应键复位。双击没有结束动作；A+B 长按返回 Launcher。

正计时下方保留触屏主操作和暂停时的 `RESET`；倒计时下方只显示带小喇叭图标的 **RING ON/OFF** 点击按钮，B 实体键继续控制计时。倒计时模式右上角整块弧形半圆填充为 `#46D1E0`。铃声默认关闭，开启后倒计时归零会以最大提示音幅度播放约 4 秒；原有振动仍执行。时长和铃声设置保存在手表中，运行中的计时状态只在内存中保存，设备重启后清空。详细操作见 [Focus 使用说明](docs/FOCUS.md)；当前镜像与实机验收边界见 [项目状态](PROJECT_STATE.md)。

## 实时语音与音频发送

手表麦克风按需启动，采集后重采样成 16 kHz 单声道，以 20 ms 一帧的 4-bit IMA-ADPCM 通过 BLE 实时发送；不先生成 WAV。Bridge 解码后，经固定虚拟音频输出送到 `M5 StopWatch Mic`，供 Typeless 等应用选择。虚拟声卡建议维持已验证的 48 kHz 配置。

启动听写前，固件和 Bridge 检查音频订阅、按需录音和虚拟输出健康度；首次请求可短暂等待并显示 `MIC LINKING`。发送端在 NimBLE 缓冲不足时最多等待 80 ms，至少给 HID 和状态流量保留 4 个 mbuf；丢帧通过序号与统计暴露。明确断线或持续断流会结束当前听写并提示重录，不拼接缺失的中段音频。详见 [BLE 麦克风说明](docs/stopwatch-ble-microphone.md) 和 [二次开发指南](docs/AGENT_DEVELOPMENT_GUIDE.md)。

标准 BLE HID、Codex Vendor HID 与 BLE 麦克风/Companion 是独立链路。诊断时分别确认按键、原生 Agent/Encoder/Radial、音频与面板，不用“蓝牙已连接”推断全部功能正常。

## 构建与验证

主固件使用 ESP-IDF v5.5.4：

```bash
cd firmware-stopwatch-idf
python3 ./fetch_repos.py
idf.py build
```

Bridge：

```bash
tools/typeless_bridge/build_stopwatch_ble_bridge.sh
```

小智的板卡入口在 `firmware-xiaozhi/main/boards/m5stack/stopwatch/`，分区表在 `firmware-xiaozhi/partitions/stopwatch_dual.csv`。两套固件需分别构建、记录镜像哈希和安装槽位。只在设备现有分区与构建分区完全兼容时使用 `idf.py app-flash`；分区或 Bootloader 变化需要完整刷写。刷写前确认具体 USB 串口和运行槽位。构建通过、刷写通过、实机操作通过应分别记录，不能相互替代。

当前仓库源码对应的部分功能已有历史实机使用基础；[PROJECT_STATE.md](PROJECT_STATE.md) 列出本目录产物尚待完成的页面、A/B 语音、Agent、推理滑动、Radial 和小智验收。开发约定见 [WORKFLOW.md](WORKFLOW.md) 和 [AGENTS.md](AGENTS.md)。

## 文档导航

- [功能与系统组成](docs/FEATURES.md)
- [Focus 本地专注计时](docs/FOCUS.md)
- [小智集成的现状与历史方案](docs/xiaozhi-integration-plan.md)
- [额度机制](docs/QUOTA.md) · [跨 Mac 统计](docs/stopwatch-cloud-sync.md)
- [BLE 麦克风](docs/stopwatch-ble-microphone.md) · [隐私与安全](docs/SECURITY_AND_PRIVACY.md)
- [Agent 与二次开发指南](docs/AGENT_DEVELOPMENT_GUIDE.md)

`docs/PET_CUSTOMIZATION.md`、`docs/assets/classic-pet-ui.svg` 与旧发布说明保留为上游历史资料，不描述当前可用页面。原创修改遵循 [MIT License](LICENSE)；虚拟麦克风组件的第三方许可见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
