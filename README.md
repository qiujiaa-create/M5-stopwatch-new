# M5 StopWatch：Codex 控制器、Focus 计时与小智双固件

文档版本：1.13；最后更新：2026-10-15 20:24。

当前源码版本：**固件 v0.10.7 / macOS Bridge v1.4.2**。版本号是源码元数据，具体设备安装版本以实机验证为准。

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

这是一台设备上的两个可启动系统。小智通过切换启动槽进入，不与 Codex 页面同时运行。主固件 Codex App 只保留 **OpenWatcher V2**，旧 Codex Micro 页面及主题选择项已移除；历史主题设置不再读取，直接使用 V2。`Codex Micro` 输入模式与 Vendor HID 协议继续保留。

StopWatch 主固件每次启动直接进入 Codex V2，不创建 Launcher 首页、不等待首次引导，也不播放 Launcher 开机音。A+B 退出 Codex 后回到 Launcher，仍可进入 Focus、设置和小智确认页。V2 手势轮询间隔从空闲时的 100 ms 缩短到约 33 ms；Agent 保留 280 ms 长按阈值，中心四向改为 200 ms 展开，显示继续按变化刷新。实际启动耗时和操作手感以实机验收为准。

主固件的 Launcher/Setup 提供“切换到小智”确认页，检查 `ota_0` 有可启动镜像后才切换并重启。小智侧可长按 B 约 3 秒，或使用其返回 StopWatch 的 MCP 工具，切回 `factory`。当前硬件上安装的槽位及小智语音对话效果需按 [项目状态](PROJECT_STATE.md) 单独复核。双固件使用相同分区布局；`factory` 为 6 MiB，`ota_0` 为 3 MiB，`storage` 为 4 MiB。刷写前以两侧分区 CSV 和设备实际分区表为准。

Launcher 首页支持 1 秒长按快捷启动：首次使用时 A 进入小智确认页，B 进入 Focus；按满 1 秒后松手打开，A/B 短按仍用于左右切换图标。可在 `Settings → Device → Quick Launch` 分别选择两个长按目标或关闭快捷键，保存后关机不丢失。选择小智只会打开既有确认页，不会绕过确认直接切换启动槽。操作和验收边界见 [功能说明](docs/FEATURES.md)。

## Codex 页面和输入

弧形任务卡表盘已于 2026-10-15 刷入 factory，写后及独立校验通过；后续输入延迟修复版与实机验收状态详见 [项目状态](PROJECT_STATE.md)。

- **OpenWatcher V2** 使用弧形任务卡布局：四个 1–4 号 Agent 卡片显示主机灯效对应的“执行中 / 未读 / 待处理 / 异常”；离线、未知或自定义灯色有独立文字，不猜测任务名称。额度区保留周剩余与 Today；独立 5H 进度条按剩余量填充，条内由红经橙、黄平滑过渡到绿。最近四小时的 24 格活动改为从左到右排列。
- 顶部改为两个无文字方向按钮：左侧松手触发一次左旋，右侧松手触发一次右旋。中间留空；拖动、跨区或移出取消，长按不连发。仍发送原生 Encoder，实际功能跟随 Codex 的 Knob 配置，不修改主机配置。中间四向方向盘长按 200 ms 弹出居中的 304 px 圆形面板，松手后保持展开；再短按四个独立圆弧方向区，按下显示青色反馈并输出原生 Radial、松手归中。可连续选择，中央 × 按下即关闭，面板外点击在松手时关闭；展开的手指不会直接选中方向。面板预建，按钮反馈只做局部刷新。
- Agent 卡片短触仅预览，长按 280 ms 提交；确认前滑向另一卡片会重新计时，同一触摸确认后不再提交第二个槽位。Working 卡边框保持呼吸反馈和原执行期间亮屏策略。灯效分配只影响显示，能否发送仍由 Vendor HID 传输就绪状态决定。
- 底部显示两个小图标：左侧横向电池框内为剩余电量数字（不带 `%`），30% 及以上绿色、低于 30% 红色；右侧蓝牙连接为蓝色，断连为红色加斜线。两个图标缩小、靠近并下移，避开 2/3 号任务卡。原四向提示文字和两个 BLE/Wi-Fi 状态点已移除。图标在主界面、四向面板及语音期间保留，临时动作提示结束后恢复。
- V2 保留现有输入模式：`Codex Micro` 下 A 为原生 Mic `ACT10`、B 松开为原生 Send `ACT12`；`Typeless` 下 A 为 `F19` 听写、B 短按或长按松开均只发送一次已配置确认键（默认 `Return`），建议等文字出现后再按 B。微信模式不变。录音时方向盘位置显示波形；停止录音立即隐藏波形并恢复灰显方向盘，顶部保留 `PROCESSING`，识别完成后恢复触控；中断明确提示重录。操作、构建及实机边界见 [表盘使用说明](docs/CODEX_TASK_DECK.md)。

Codex 当前页面只有**被动显示**的额度状态；5H 低额不会另行发声或振动提醒。Bridge 可在 Mac 本机采集 Codex 额度，经 BLE 推送 5H（18000 秒）与周（604800 秒）窗口。缺失、过期或异常数据保留未知状态。录音期间新面板暂存，回到空闲后补发。Today 是从北京时间 08:00 起的周额度百分点消耗，并非官方独立的每日额度。细节见 [额度机制](docs/QUOTA.md)。

## Focus 本地专注计时

Launcher 中的 **Focus** 是独立应用，计时完全在手表本地进行，不连接 Mac 上的 TickTick。A 键控制正计时，秒数右侧显示十分之一秒；B 键控制倒计时，默认 25 分钟。切换到倒计时后，点击主数字的分钟区域可设置 1–60 分钟，保存后记住上次时长。在当前模式按另一按键时只切换到对应页面；进入目标页面后再按一次对应键才开始、暂停或继续。仅在 `PAUSED` 时长按对应键复位。双击没有结束动作；A+B 长按返回 Launcher。

正计时下方保留触屏主操作和暂停时的 `RESET`；倒计时下方只显示带小喇叭图标的 **RING ON/OFF** 点击按钮，B 实体键继续控制计时。倒计时模式右上角整块弧形半圆填充为 `#46D1E0`。铃声默认关闭，开启后倒计时归零会以最大提示音幅度播放约 4 秒；原有振动仍执行。时长和铃声设置保存在手表中，运行中的计时状态只在内存中保存，设备重启后清空。详细操作见 [Focus 使用说明](docs/FOCUS.md)；当前镜像与实机验收边界见 [项目状态](PROJECT_STATE.md)。

## 实时语音与音频发送

手表麦克风按需启动，采集后重采样成 16 kHz 单声道，以 20 ms 一帧的 4-bit IMA-ADPCM 通过 BLE 实时发送；不先生成 WAV。Bridge 解码后，经固定虚拟音频输出送到 `M5 StopWatch Mic`，供 Typeless 等应用选择。虚拟声卡建议维持已验证的 48 kHz 配置。

启动听写前，固件和 Bridge 检查音频订阅、按需录音和虚拟输出健康度；首次请求可短暂等待并显示 `MIC LINKING`。发送端在 NimBLE 缓冲不足时最多等待 80 ms，至少给 HID 和状态流量保留 4 个 mbuf；丢帧通过序号与统计暴露。明确断线或持续断流会结束当前听写并提示重录，不拼接缺失的中段音频。详见 [BLE 麦克风说明](docs/stopwatch-ble-microphone.md) 和 [二次开发指南](docs/AGENT_DEVELOPMENT_GUIDE.md)。

Bridge v1.4.2 修复虚拟输出重建时的音频锁等待：停止前先静音、阻止新回调查询音频单元，等待已进入的回调退出后再停止与释放。保留转换前后的真实路由校验。`MIC LOST` 表示当前录音已中断，连接恢复后按 A 重新录制；不拼接旧语音。

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
- [Codex 表盘操作说明](docs/CODEX_TASK_DECK.md) · [开发与验收记录](docs/development-log.md)
- [Focus 本地专注计时](docs/FOCUS.md)
- [小智集成的现状与历史方案](docs/xiaozhi-integration-plan.md)
- [额度机制](docs/QUOTA.md) · [跨 Mac 统计](docs/stopwatch-cloud-sync.md)
- [BLE 麦克风](docs/stopwatch-ble-microphone.md) · [隐私与安全](docs/SECURITY_AND_PRIVACY.md)
- [Agent 与二次开发指南](docs/AGENT_DEVELOPMENT_GUIDE.md)

`docs/PET_CUSTOMIZATION.md`、`docs/assets/classic-pet-ui.svg` 与旧发布说明保留为上游历史资料，不描述当前可用页面。原创修改遵循 [MIT License](LICENSE)；虚拟麦克风组件的第三方许可见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
