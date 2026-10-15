# M5 StopWatch 当前状态

文档版本：2.4；最后更新：2026-10-15 12:01。

## 本目录是什么

本目录从 `../M5-stopwatch/source-snapshot-2026-09-16/source/` 完整复制而来，是新的**开发工作目录**。原快照保留不动，原 `vibecoding` 工作区也未修改。导入时，快照清单中的 4094 个源码与参考配置文件逐个通过 SHA-256 比对；其中 4092 个位于源码树，另外两个 `sdkconfig` 放在未纳入 Git 的 `local-reference/`。

本目录中的首次 Git 记录表示“2026-09-16 现状导入”，**不表示稳定版本或已经从这里刷入手表**。原主仓库基准为 `bba454ab720ee14548450cb74f87d7418ad2062c`，原小智嵌套仓库基准为 `49ac8a6da399f27a9546d4f73640b7f86c24bac6`；两者当时的未提交文件内容都已进入本目录。文件列表见 `migration/CHANGES_INCLUDED.md`，原盘点见 `migration/ORIGINAL_SNAPSHOT_README.md`。

## 代码与运行对象

| 路径 | 作用 | 状态 |
| --- | --- | --- |
| `firmware-stopwatch-idf/` | factory 槽的 StopWatch 主固件；OpenWatcher V2、独立 Focus、BLE HID 与 Companion | V2 单主题与开机直达已刷入 factory、独立校验通过；屏幕和实体操作待用户验收 |
| `tools/typeless_bridge/` | macOS Bridge；本机额度采集、BLE 推送、Typeless 协调 | 本目录构建成功；本次迁移未替换当前安装的 App |
| `firmware-xiaozhi/` | ota_0 槽的小智 v2.2.6 及本机板卡适配 | 源码已导入；当前设备槽位需在再次切换后重核 |
| `server/cloud-sync/` | 可选同步服务 | 本机盘点时未启用 |
| `local-reference/` | 两套本机 `sdkconfig` 参考 | 未纳入 Git；转移机器时重新核对 |

Git 版本不包含 ChatGPT.app、Typeless.app、已安装的音频驱动、登录资料、运行日志、固件构建产物或手表闪存。构建产物只留在本机的忽略目录中。新目录不会自动改变 Mac 的 LaunchAgent 路径，也不会改变手表上的固件。

主固件的第三方 `components/` 已作为本机文件复制过来，但按项目原有 `.gitignore` 不纳入新 Git 仓库；其来源和版本记录在 `firmware-stopwatch-idf/repos.json`，新机器需运行 `fetch_repos.py` 恢复。`managed_components/`、`build/`、Bridge 的 `.build/` 也不纳入版本记录。

## 当前功能验收边界

- **2026-10-15 经用户授权，V2 单主题与开机直达已刷入 factory**：刷写前核对实际 USB 端口对应 ESP32-S3 v0.2，设备分区表与构建一致，factory 为 6 MiB、偏移 `0x20000`，小智 ota_0 为 3 MiB、偏移 `0x620000`，otadata 为全擦除态，选择 factory。先完整备份旧 factory 6 MiB 到本机忽略目录 `firmware-stopwatch-idf/build-v2/device-backup/factory.before.bin`（SHA-256 `4365fd904781b0f571829bb2e29a8b7011fd371e93ac52def63aa7402b385d32`）；随后 `idf.py -B build-v2 app-flash` 只写入上述 factory 应用，写后 hash 和独立 `verify_flash` 均匹配，复读确认分区表与 otadata 未变化。安装镜像 4,113,472 字节，SHA-256 `aa8ac2ba214c2963755c71c0c247b03d77b690d5426e3a29bec6241db59b8939`。USB 复位后的串口记录显示从 factory 启动、`startup target: Codex`、`V2 page ready: 2865 ms since boot`，没有 Launcher on open 或崩溃/复位异常；这是复位启动初始化时间，不是完整断电冷启动或旧版对照测试。Bridge 已恢复原生 HID 后重连，设备事件与音频订阅完成，Typeless 配置及 microphone_ready=true 同步成功，时间和额度面板重新推送；麦克风 output=healthy、route valid=true、48 kHz，进程输出审计通过且只使用允许的虚拟设备。未安装新 Bridge，未写 Bootloader、小智、NVS、storage 或配对数据。实际屏幕、断电开机、A/B、短触取消/长按一次提交、四 Agent、推理双向、Radial 归中和真实语音录入仍待用户操作确认；不将启动/订阅/路由日志等同于实机交互验收。

- **2026-10-15 V2 单主题与开机直达构建记录（刷写前）**：删除旧 Codex Micro Dashboard、专属绘制/触摸逻辑和 Setup 的 Codex Theme 选择项；固定初始化 OpenWatcher V2，不再读写历史 `theme` / `theme_gen`。保留 Codex Micro 原生输入模式与现有 A/B 状态机。StopWatch 主固件启动时通过 Launcher 原有生命周期直接打开 Codex，不创建 Launcher 首页、不等待首次引导、不播放 Launcher 开机音；Codex 打开时清除 BootLogo，A+B 关闭后回到 Launcher。首次触摸及手势轮询由空闲 100 ms 缩短至约 33 ms（主循环调度影响实际间隔），长按确认仍为 280 ms。主机启动生命周期检查通过：自动进入 Codex、不创建 Launcher UI、退出后正常返回且不自动重入、缺少 Codex 时安全回退；语音门控、麦克风就绪、Launcher 快捷键、Focus、版本与差异检查通过。ESP-IDF v5.5.4 全量构建成功，`build-v2/StopWatch-UserDemo.bin` 为 4,113,472 字节，SHA-256 `aa8ac2ba214c2963755c71c0c247b03d77b690d5426e3a29bec6241db59b8939`；比 2026-10-12 最后记录的 4,121,808 字节镜像减少 8,336 字节。保留原 build 产物，现有依赖未刷新。构建阶段尚未刷写；后续安装证据见同日刷写记录。实际冷启动、画面、实体操作与空闲功耗仍需单独验收。源码回退基线为 `649f274`，任务分支 `codex/v2-only-fast-start`，尚未提交。

- **已由用户在原安装组合上确认**：OpenWatcher V2 显示 5H 与周额度，并曾自动刷新为 5H 6% / 周 28%。这证明当时 Bridge 到手表的额度链路有效，不证明本目录产物已安装。
- **2026-09-17 在当前安装组合上由用户实机确认**：V2 中心长按后的上、下、左、右四向 Radial，手表反馈和 Codex 动作均正常。此结果仍不代表本目录新构建产物已刷入手表。
- **2026-09-17 在当前安装组合上由用户实机确认**：V2 顶部向左、向右滑动分别降低、提高 Codex 推理等级。故障原因是本机 Codex Micro 的 `encoderMode` 为 `composer-navigation`，原生 Encoder 事件被用作上下导航。已将 `~/.codex/config.toml` 改为 `reasoning` 并保留原配置备份；运行中的 Codex 还需在「设置 → Codex Micro → Knob」选择 `Reasoning only` 才立即生效。Bridge 的 `codex_reasoning:native_sync` 日志本身不能代替 Codex 界面的操作结果。
- **V2 页面说明**：底部的两个小圆点是 BLE 与 Wi-Fi 状态；线与圆点组成的图形是同步装饰。V2 当前没有第二页或页切换手势。当前只保留 OpenWatcher V2，历史主题选择入口已移除。
- **2026-10-12 Codex 两套主题右键发送调整已刷入**：Codex Micro 与 OpenWatcher V2 共用的 Codex Micro 输入模式中，B 键松开动作已从原生 Voice Chat `ACT09` 改为原生 Send `ACT12`；短按和长按均在松开时发送一次，中心 `ACT12` 入口保留，Typeless/微信输入模式不变。语音门控回归、麦克风就绪回归、版本一致性检查和差异检查通过；ESP-IDF v5.5.4 全量构建通过，factory 应用镜像 4,121,696 字节，SHA-256 `2969d612585235403ce6fb8a20ae148c4466d39885b6afc5fba32dac9b132afc`。刷写前确认设备为 ESP32-S3、factory 为 6 MiB 且 `otadata` 为擦除态；已通过 `/dev/cu.usbmodem21201` 的 `app-flash` 只写入 `factory @ 0x20000`，写后 hash 和独立 `verify_flash` 都匹配，复读确认分区表和 `otadata` 未变。Mac 端虚拟麦克风和安全输出路由健康，刷写重启后 BLE 重连以及两套主题的实际按键/Codex 响应仍待用户验收。
- **2026-10-12 后续 Typeless B 键行为修正已刷入，实机部分验收**：当前安装的 Bridge 输入模式为 `Typeless`，会同步到手表；因此上述仅在 `Codex Micro` 输入模式生效的原生 `ACT12` 不是用户之前看到 B 键触发语音的路径。按用户确认，保留 A→Typeless `F19`，Typeless 模式的 B 改为短按/长按松开均只发送一次配置的确认键（当前为 `Return`），不再在录音/识别时临时代替 A。微信模式和 Codex 原生模式保持各自现有逻辑；录音/识别中按 B 可能提前发送，推荐先用 A 结束并等文字出现。语音门控回归、麦克风就绪回归、版本一致性与差异检查通过；ESP-IDF v5.5.4 构建通过，factory 镜像 4,121,808 字节，SHA-256 `fe43602c0e848d12273f5462b291f5a96dc93e94e9ee50ee3a6961f143b903cc`。刷写前重核 `/dev/cu.usbmodem21201` 为 ESP32-S3、设备分区表与构建前缀一致、`factory @ 0x20000` 为 6 MiB 且 `otadata` 全擦除；只执行 `app-flash`，写后 hash 与独立 `verify_flash` 匹配，复读确认分区表和 `otadata` 未变。Bridge 重连后再次同步 `typeless` 模式并报告麦克风就绪、虚拟输出路由有效；Typeless 仍选中 `StopWatch Typeless Input`。用户已实测 OpenWatcher V2 的 A 录音流程和 B 短按：B 不再启动 Typeless，正常发送；Codex Micro 主题、B 长按仅发送一次、四 Agent、推理双向及 Radial 松手归中仍待此次镜像的实机验收。上一条已刷入镜像不包含此修正。
- **2026-09-17 早先任务的构建与刷写记录**：从 Codex App 移除 Official V1 及其专属图片、逐帧宠物动画；原有 `official_v1` 设置回退到 Codex Micro。ESP-IDF 5.5.4 构建通过，应用镜像从 5,904,864 降至 4,025,456 字节，SHA-256 `8778bf86dfde834b6460f75e9774c9f41924b495eabfc1d1876bb1eae3a3cdf8`。当时设备分区表与构建完全一致、otadata 空白且从 factory 启动；仅执行 `app-flash` 并通过写后 hash 校验与串口启动、BLE 重新订阅。此后仓库已有新提交，不能把该哈希当成当前 HEAD 的构建产物；当时未安装本目录 Bridge、未改小智槽与持久数据分区。
- **2026-09-17 Focus 任务的构建与刷写记录**：Launcher 已加入独立的本地 Focus 应用，A 正计时显示十分之一秒，B 固定 25 分钟倒计时；短按开始/暂停/继续，暂停时长按对应键复位，双击不执行结束。运行计时在离开页面或屏幕休眠后继续，设备重启后清空。最终镜像由 ESP-IDF 5.5.4 构建，4,111,648 字节，SHA-256 `25d5a7e919437c12d3b2c3302f9066277df7858fa48e08b64e802ca89e9c1df5`；主机计时测试通过，`app-flash` 仅写入 `factory` 应用偏移 `0x20000`，写后哈希校验通过并自动重启一次。用户反馈初版 Focus 曾正常打开和操作；最新十分之一秒显示与 A/B 暂停长按复位仍待实机确认。操作说明见 [Focus 使用说明](docs/FOCUS.md)。
- **2026-09-18 Focus 铃声调整已刷入**：倒计时完成铃声改为六个音符、总时长约 3 秒，并使用音频接口支持的最大提示音幅度；全局扬声器音量设置不变。此前的 1–60 分钟设置、NVS 保存与 `RING ON/OFF` 点击按钮保持不变。主机状态测试和 ESP-IDF 5.5.4 构建已通过；镜像 4,116,000 字节，SHA-256 `8f410119fa2354ecde53e2286a55bdb524bf010b13d0dc29026f6da07062b66e`。已通过 `/dev/cu.usbmodem21201` 的 `app-flash` 写入 factory 应用偏移 `0x20000`，命令成功完成并自动硬复位一次；未再访问串口。铃声音量和 3 秒持续时间仍待实机验收。
- **2026-09-18 Focus 铃声改为 4 秒版本**：保留音符序列、最大提示音幅度、振动和铃声开关，仅将每个音符改为约 0.667 秒，总时长约 4 秒。主机状态测试和 ESP-IDF 5.5.4 构建已通过；新镜像 4,116,000 字节，SHA-256 `23015afe67368363d36f7a9a84d3d6ff0e635a60d1ec44cd9e5a354bf9071fee`。尚待刷入 factory 和实机验收。
- **2026-09-18 Focus 4 秒版本已刷入**：上述镜像已通过 `/dev/cu.usbmodem21201` 的 `app-flash` 写入 factory 应用偏移 `0x20000`，命令成功完成并自动硬复位一次；未再访问串口。4 秒铃声仍待实机听感确认。
- **2026-09-18 Focus 倒计时弧形颜色调整已构建**：B 倒计时主页面及设置页右上整块弧形半圆使用不透明填充色 `#46D1E0`，正计时仍使用原珊瑚色。主机计时测试、差异检查与 ESP-IDF 5.5.4 构建已通过；新镜像 4,116,128 字节，SHA-256 `921e30aff211d4a9b4f94115a86deb6cf2eb83d689224aa9228ba3ec400fc1ce`。尚待刷入 factory 和实机验收。
- **2026-09-18 Focus 按键两步切换已构建**：在 A 页面按 B 或在 B 页面按 A 时，第一次只切换页面；进入目标页面后再按对应键才启动、暂停或继续计时。暂停长按复位、触屏主按钮和其它计时逻辑不变。主机计时测试、差异检查和 ESP-IDF 5.5.4 构建已通过；新镜像 4,116,128 字节，SHA-256 `7fa9d34dbc9d403b0b50e912a50485114a082d8b2d078b7f561156f5d93e243a`。尚待刷入 factory 和实机验收。
- **2026-09-18 Focus 按键两步切换已刷入**：上述镜像已通过 `/dev/cu.usbmodem21201` 的 `app-flash` 写入 factory 应用偏移 `0x20000`，命令成功完成并自动硬复位一次；未再访问串口。两步切换行为仍待实机确认。
- **2026-09-18 V2 Agent 交互调整已构建**：四个 Agent 点长按提交阈值改为 280ms；颜色为 Working（`0x304FFE`）的执行中小圆点使用持续脉冲闪动；执行期间 App 持续刷新活动计时，阻止自动变暗、息屏和自动关机，任务结束后恢复原有空闲策略。主机计时测试、差异检查与 ESP-IDF 5.5.4 构建已通过；新镜像 4,116,352 字节，SHA-256 `8cc1645d1c8538c404ddfd4f7ef7734fec13978c2fee7f3168b62c8591a9deaa`。尚待刷入 factory 和实机验收。
- **2026-09-18 V2 中心 Radial 长按调整已刷入**：中心四向 Radial 进入阈值由 480ms 改为 280ms，方向映射、连续事件和松手归中逻辑不变。镜像 4,116,352 字节，SHA-256 `79e6c41ee47159b2ac76d8b283874bc0a8b859f7e002853d327b43a8f078a7ec`，已通过 `/dev/cu.usbmodem21201` 的 `app-flash` 写入 factory 应用偏移 `0x20000`，写后 hash 校验通过并自动硬复位一次；中心长按行为仍待实机确认。
- **2026-09-20 V2 5H 进度条已刷入**：OpenWatcher V2 在 `5H` / `RESET` 文字上方增加横向剩余额度进度条，填充长度按 5H 剩余百分比变化，颜色从低额红色平滑过渡到高额绿色。缺失、过期或异常数据只显示暗色轨道，继续保留 `--`，不借用周额度。ESP-IDF v5.5.4 全量构建和版本检查通过；factory 应用镜像 4,116,656 字节，SHA-256 `770c92c102acdbbd631802fe1faedeb9a94ddaf5574f00d7f3c57b5c0b43fa41`。刷写前确认设备为 ESP32-S3、factory 为 6 MiB 且 `otadata` 为擦除态；随后通过 `/dev/cu.usbmodem21201` 的 `app-flash` 只写入 `factory @ 0x20000`，写后 hash 和独立 `verify_flash` 均匹配，并完成硬复位。实机显示与 5H 动态刷新仍待确认。
- **2026-09-20 V2 TODAY 引导线已刷入**：TODAY 区域右侧折线由向下改为向上，终点指向周额度剩余数字的下沿，避免与 5H 进度条产生语义混淆。仅调整静态画布几何，数据、进度条和交互链路不变。ESP-IDF v5.5.4 增量构建、版本检查和差异检查通过；factory 镜像 4,116,656 字节，SHA-256 `77c94539582386cc24096e89bafc2f6c80c7c8034a15a6eb2467895351d8b727`。已通过 `/dev/cu.usbmodem21201` 的 `app-flash` 只写入 `factory @ 0x20000`，写后 hash 和独立 `verify_flash` 均匹配，并完成硬复位；实机显示仍待确认。
- **2026-09-20 Launcher 长按快捷启动已刷入**：Launcher 首页保留 A/B 短按左右导航，增加 1 秒单键长按快捷启动；默认 A 打开小智确认入口，默认 B 打开 Focus。`Settings → Device → Quick Launch` 可分别选择目标或 Disabled，并通过 NVS 保存稳定目标字符串。达到阈值后松手才打开目标，避免按住状态传入新 APP；松手前形成 A+B 会取消单键目标。主机手势测试、Focus 回归测试、ESP-IDF v5.5.4 构建、版本检查和差异检查通过；factory 镜像 4,121,712 字节，SHA-256 `edbeb52ce0a9830ce02a1511f13dae92b85ca4f976f6c637dabd7023164fe9e2`。2026-09-20 通过 `/dev/cu.usbmodem21201` 的 `app-flash` 写入 `factory @ 0x20000`，写后 digest 与独立 `verify_flash` 均匹配；写后分区表和 otadata 未变化。设置页圆屏布局、实体按键时序、重启后配置保留和小智二次确认仍待实机验收。
- **待实机操作验证**：确认开机直达 V2、设置不再列出 Codex Theme，A+B 退出回到 Launcher；再回归 V2 的 A/B 语音、四 Agent、推理滑动与四向 Radial。串口启动成功不代替这些操作结果。小智固件未在本次迁移中构建。
- **容量边界**：缩小的是 factory 应用镜像，在 6 MiB factory 分区内增加约 1.79 MiB 余量；4 MiB FAT `storage` 分区容量未变化。
- **小智**：独立启动槽，不是 Codex 页面。factory 的 Launcher/Setup 提供切到 ota_0 的确认入口，并先检查目标镜像；小智板卡实现提供 B 键长按 3 秒或 MCP 工具切回 factory。当前设备槽位及小智语音对话仍需单独复核。
- **音频发送策略**：factory 仅在录音时采集并发送 20 ms IMA-ADPCM 帧；发送前最多等待 80 ms 的 NimBLE mbuf 恢复，保留至少 4 个 mbuf 给控制流量。丢帧由序号和计数暴露；Bridge 对明确中断结束本次听写并提示重录。此项是代码现状，不等同于本次新产物的实机音频验收。

## 从这里开始

1. 先阅读本文件和 `WORKFLOW.md`。一个任务只由一个 AI 在一个工作目录里修改；其他 AI 用独立 worktree 审查或处理不同任务。
2. V2 推理滑动与四向 Radial 已在当前安装组合上通过用户实机验证。后续新构建产物仍要逐层查触摸反馈、固件事件、BLE HID 与 Mac/Codex 最终效果。
3. 本目录曾通过 Bridge 与主固件构建，且主固件曾刷入 factory。其后仓库仍有提交；下次部署前须重新构建并记录产物哈希、确认设备分区、端口和当前启动槽。
4. 只有手表和 Codex 端的操作结果都通过，才将该任务标为“实机通过”。再考虑把这套组合标成稳定基线。

构建入口与 ESP-IDF 版本见 `migration/ORIGINAL_SNAPSHOT_README.md`。本机参考配置在 `local-reference/`；它们不应直接作为公开配置提交。
