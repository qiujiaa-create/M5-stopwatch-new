# 当前功能与系统组成

文档版本：0.7；最后更新：2026-09-18 13:30。

本文描述当前源码实现。安装和实机验收状态以 [PROJECT_STATE.md](../PROJECT_STATE.md) 为准。

## 两个固件，一个 Codex App

| 系统 | 启动槽 | 功能 | 切换 |
| --- | --- | --- | --- |
| StopWatch 主固件 | `factory` | Launcher、设置、Codex 页面、独立 Focus 计时、BLE HID、BLE 麦克风 | Launcher 或 Setup 的小智入口检查镜像后写入启动槽并重启 |
| 小智 v2.2.6 板卡适配 | `ota_0` | 独立的小智语音系统 | 长按 B 约 3 秒或调用小智侧 MCP 返回工具，重启到 `factory` |

双固件分区布局见 `firmware-stopwatch-idf/partitions.csv` 与 `firmware-xiaozhi/partitions/stopwatch_dual.csv`。两边不能同时运行；切换不是 Codex 页面内的主题切换。小智板卡实现见 `firmware-xiaozhi/main/boards/m5stack/stopwatch/stopwatch.cc`。该实现和槽位存在不等于当前设备已完成小智语音对话验收。

## Codex 页面

主固件 Codex App 只保留 `Codex Micro` 与 `OpenWatcher V2`，通过 `Setup → Device → Codex Theme` 选择。旧 `official_v1` 等设置在加载时回退到 Codex Micro。Official V1 / Classic Pet 及其逐帧图片不再编入当前 App；旧素材和定制文档是历史参考。

- **Codex Micro**：原生 Agent 状态、语音状态、5H 与周额度。5H 数据未知时保留等待/未知提示。
- **OpenWatcher V2**：周额度大数字、Today 消耗、独立的 `5H` 百分比与 `RESET` 倒计时、最近四小时 24 格活动、四个 Agent 点、连接与语音状态。顶部标题表达 Codex Ready/Linking/Offline；`COMPACT SOON` 是收到上游字段后的文字提示。底部线点是视觉装饰，不是第二页指示器。
- 两套主题共用按键和语音状态机。四个 Agent 点有触摸预览和 280 ms 长按确认；Working 状态的小圆点持续闪动，任务执行期间暂时阻止自动变暗、息屏和关机；顶部左右滑动发送原生 Encoder；中心长按 280 ms 后发送四向 Radial，松手归中。

显示按实际 466×466 圆屏设计。V2 Agent 可见圆点已放大，透明触摸区与可见点分开。灯效状态只控制显示；是否能发送 Agent 操作取决于 Vendor HID 传输是否就绪。

## Focus 本地专注计时

Launcher 的 Focus 应用提供 A 正计时和 B 倒计时，互斥运行。正计时显示到十分之一秒。倒计时默认 25 分钟，点击分钟数字可在 1–60 分钟间选择并保存；保存新时长会重置当前倒计时到待开始状态。在当前模式按另一按键只切换页面，进入目标页面后再按一次对应键才开始、暂停或继续；仅在 `PAUSED` 时长按对应键才复位。双击不触发结束动作；A+B 长按返回 Launcher。

正计时下方保留触屏主操作与暂停时的 `RESET`。倒计时模式右上角整块弧形半圆填充为 `#46D1E0`。倒计时下方改为带小喇叭图标的铃声点击按钮，不提供 B 的触屏开始、暂停、继续或复位；B 实体键保持这些控制。铃声默认关闭，开启时归零以最大提示音幅度播放约 4 秒；归零始终显示 `DONE` 并振动一次。时长和铃声开关保存在 NVS，运行计时仅在内存中，设备重启后重新初始化。Focus 不需要 TickTick 账号、Mac Bridge、Wi-Fi 或新的 BLE 协议；Codex 页面里的 A/B 语音操作仍按原状态机执行。操作表见 [Focus 使用说明](FOCUS.md)，安装和实机验证范围见 [项目状态](../PROJECT_STATE.md)。

## 三条独立链路

1. **标准 BLE HID**：A/B 和摇晃对应的系统按键；Bridge 退出后已配对的基本按键仍可工作。
2. **Codex Vendor HID**：原生 Agent、Encoder 与 Radial 事件；不以虚拟鼠标或键盘替代。
3. **BLE Companion 与麦克风**：Bridge 接收设备状态，推送额度/活动面板，控制按需音频；音频经虚拟设备交给 macOS 应用。

默认 A 为 `F19`，B 在录音或识别流程中临时代替 A，Ready 后恢复确认/发送。Bridge 支持 Typeless 与微信输入法的输入配置，包含绑定同步、状态观察和可选的 Typeless 启动协调。改变按键行为需同时检查两套主题及 Mac 端实际结果。

## 额度和活动

Bridge 可选读取本机 Codex 登录状态取得官方 usage 摘要，按 604800 秒周窗口与可选 18000 秒 5H 窗口分别推送。5H 缺失、过期或数值异常时显示 `--`，不换算成周额度或 0%。录音中面板推送暂存，回到空闲后再发送。Today 表示从北京时间 08:00 起的周额度百分点消耗，不是另一种官方日额度。最近四小时有 24 个 10 分钟格；可选云同步汇合多台 Mac 的统计。Wi-Fi panel fallback 在 StopWatch 主固件中默认关闭。详见 [额度说明](QUOTA.md) 与 [跨设备同步](stopwatch-cloud-sync.md)。

额度为被动显示，当前实现没有针对 5H 低额的声音或振动提醒。

## 音频发送和故障策略

StopWatch 麦克风按需唤醒，采集数据重采样至 16 kHz 单声道，按 20 ms 帧编码成 IMA-ADPCM，经 BLE Notify 实时传输。Bridge 解码 PCM 并固定输出到虚拟音频设备；不生成 WAV 或把录音上传到云端。虚拟设备已验证设置为 48 kHz。

启动前的就绪握手要求音频通知、按需采集及 Bridge 虚拟输出/路由健康。尚未就绪时设备短暂保留一次请求并显示 `MIC LINKING`；超时则中止。发送端在 NimBLE mbuf 不足时最多等待 80 ms，并为 HID、状态及订阅流量保留至少 4 个 mbuf；统计记录发送、丢帧及通知错误。Bridge 对短缺口填静音以保持时间位置，但明确断线或持续断流会中止当前听写并提示重新录制，不把缺失中段悄悄拼接成完整语音。详见 [BLE 麦克风](stopwatch-ble-microphone.md)。

## 验收边界

源码存在、构建成功、镜像写入和设备最终操作是四种不同证据。当前目录的主固件曾完成 factory 应用刷写与串口/BLE 启动检查；后续提交及当前两套页面的完整操作仍需复核。小智的当前启动槽、语音对话和切回操作也应单独验收。具体记录和下一步见 [PROJECT_STATE.md](../PROJECT_STATE.md)。
