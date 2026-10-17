# M5 StopWatch 当前状态

文档版本：2.22；最后更新：2026-10-16 16:51。

## 本目录是什么

本目录从 `../M5-stopwatch/source-snapshot-2026-09-16/source/` 完整复制而来，是新的**开发工作目录**。原快照保留不动，原 `vibecoding` 工作区也未修改。导入时，快照清单中的 4094 个源码与参考配置文件逐个通过 SHA-256 比对；其中 4092 个位于源码树，另外两个 `sdkconfig` 放在未纳入 Git 的 `local-reference/`。

本目录中的首次 Git 记录表示“2026-09-16 现状导入”，**不表示稳定版本或已经从这里刷入手表**。原主仓库基准为 `bba454ab720ee14548450cb74f87d7418ad2062c`，原小智嵌套仓库基准为 `49ac8a6da399f27a9546d4f73640b7f86c24bac6`；两者当时的未提交文件内容都已进入本目录。文件列表见 `migration/CHANGES_INCLUDED.md`，原盘点见 `migration/ORIGINAL_SNAPSHOT_README.md`。

## 代码与运行对象

| 路径 | 作用 | 状态 |
| --- | --- | --- |
| `firmware-stopwatch-idf/` | factory 槽的 StopWatch 主固件；OpenWatcher V2、独立 Focus、BLE HID 与 Companion | V2 单主题与开机直达已刷入；紧凑电池/蓝牙图标版已刷入 factory、独立校验通过；无状态点、电池无百分号，新间距待实机确认 |
| `tools/typeless_bridge/` | macOS Bridge；本机额度采集、BLE 推送、Typeless 协调 | v1.4.2 已安装本机 App；语音启停、Ready 与 5H 显示由用户确认恢复；连接与定时额度推送核验通过 |
| `firmware-xiaozhi/` | ota_0 槽的小智 v2.2.6 及本机板卡适配 | 源码已导入；当前设备槽位需在再次切换后重核 |
| `server/cloud-sync/` | 可选同步服务 | 本机盘点时未启用 |
| `local-reference/` | 两套本机 `sdkconfig` 参考 | 未纳入 Git；转移机器时重新核对 |

Git 版本不包含 ChatGPT.app、Typeless.app、已安装的音频驱动、登录资料、运行日志、固件构建产物或手表闪存。构建产物只留在本机的忽略目录中。新目录不会自动改变 Mac 的 LaunchAgent 路径，也不会改变手表上的固件。

主固件的第三方 `components/` 已作为本机文件复制过来，但按项目原有 `.gitignore` 不纳入新 Git 仓库；其来源和版本记录在 `firmware-stopwatch-idf/repos.json`，新机器需运行 `fetch_repos.py` 恢复。`managed_components/`、`build/`、Bridge 的 `.build/` 也不纳入版本记录。

## 当前功能验收边界

- **2026-10-16 Recent 8H / 20 分钟分格已构建、尚未安装**：用户发现 Recent 4H 全空并说明最近一小时持续使用 Codex。现场数据确认 Bridge 的额度日志持续变化，但活动历史最后只包含 12:13–12:14 的两段手表录音和四次实体交互；16:49 时均已超过旧 4 小时窗口。当前活动统计只记录手表录音、primary/enter/shake 等少量实体事件，不记录普通桌面 Codex 对话、任务执行或额度下降，因此旧界面发送 24 个 0 是当前口径的真实结果，不是 BLE/解析/绘制故障。
  - **改动**：本地 Bridge 窗口改为 28,800 秒，24 格不变、每格 1,200 秒；固件标题改为 `RECENT 8H`。云同步 Swift 客户端和 Node 示例服务端同步使用 28,800 秒，服务端分桶宽度由 600 秒改为 1,200 秒。强度仍为 70% 录音时长＋30% 启动次数/操作权重；8H 扩大既有手表活动口径，不冒充 Codex 总体使用。
  - **验证**：实际 LVGL/PARTIAL 主机回归通过并生成 `RECENT 8H` 预览；Bridge 完整编译、Swift 云客户端回归、云服务 15 项测试全部通过，其中新增五小时前事件仍保留、八小时后归零的边界。ESP-IDF v5.5.4 全量构建通过，factory 应用 4,027,424 字节，SHA-256 `b05bd33388a2d59f5d7183a213ec85368048254b0c905c43524f710466dbe279`。
  - **安装边界**：该固件、Bridge 后续版和可选云服务变更均未安装或部署；手表仍运行下条 16:22 已独立校验的 `991030...` 镜像，本机 Bridge 仍为已安装 v1.4.2。当前本机未配置 `cloud-sync.json`，云服务变更不影响现有运行态。
- **2026-10-16 5H 大数字共基线修复已刷入 factory，等待实机观感确认**：用户实机指出 `82%` 的个位比十位高。根因是十位直接在额度父容器底部对齐，而个位放在另一个 Flex 子容器内，两条布局路径没有形成同一字体基线。当前源码移除个位 Flex 容器，百位/十位与个位直接使用同一父容器、同一 Commissioner 64 字体和相同底边定位；个位 x 起点仍固定为屏幕中心线 233，`%` 仅在额度文字更新时跟随个位右侧重排，不进入静态画布逐帧路径。
  - **主机验证**：实际 LVGL/PARTIAL 回归新增 Label 同高/同底坐标和最终像素边界断言。示例 `82%` 修复前十位像素 y=60–105、个位 y=57–101；修复后顶部均为 y=60，底部分别为 y=105/104，剩余 1 px 来自 `8` 与 `2` 字形轮廓而非基线偏移。5%、82%、100%、未知值中心锚点及全部既有交互/局部刷新回归继续通过，预览已更新。
  - **构建与安装**：ESP-IDF v5.5.4 全量构建通过，factory 应用 4,027,424 字节，SHA-256 `9910303c5d27246b672e2a897b955bcb36d51c0aded5f3a7d7428f6100e40b6a`。16:22 经用户授权确认唯一端口 `/dev/cu.usbmodem21201` 为 ESP32-S3 rev 0.2；当前应用前 4,027,056 字节哈希匹配上一版 `baf707...` 后，仅用 `app-flash` 写入 `factory @ 0x20000`，工具内部校验和独立 `verify_flash` 均通过。
  - **回退与边界**：写前完整 6 MiB factory 备份位于忽略目录 `firmware-stopwatch-idf/build-v2/device-backup/quota-baseline-20261016-161700/factory.before.bin`，SHA-256 `8bafeaec44c21277131f0b0c2644701d6e9e716667af2fae54132f6875cdc1e1`。分区表写前/写后相同，有效 3072 字节 SHA-256 仍为 `dcefda33354c0a802a15d8357e1e08d79502aced9a944623e94ce3678e7514e0`；otadata 前后相同且全 `FF`。未写 Bootloader、ota_0、NVS、storage 或配对数据。
  - **运行链路**：多次 esptool 复位后，Bridge 最终重新连接 `M5Codex-SU`、订阅设备事件并恢复 `microphone_ready=true`；时间面板 seq=182、额度面板 seq=183 推送成功，虚拟输出 `output=healthy`、路由有效且为 48 kHz。未打开串口日志；实体圆屏数字基线、150 ms 手感、胶囊外提示和完整交互仍待用户确认。
- **2026-10-16 中心四向 150 ms 与 Encoder 外侧定时 `-1/+1` 反馈已随共基线版刷入**：中心四向展开阈值由 200 ms 调整为 150 ms；收到左胶囊按下事件时，在胶囊外侧左上方显示 20 px 的 `-1`，右侧同理在外侧右上方显示 `+1`。提示从按下起保留 0.5 秒，即使提前松手也继续显示，到期后自动清空。实现只预建两个小型文字区并维护一个到期时间，不增加动画。
  - **主机验证**：实际 LVGL/PARTIAL 回归覆盖 120 ms 短触不展开、150 ms 阈值后的可见帧展开、四方向与归中、模态防穿透，以及左右提示位置、按下立即出现、松手后保留、约 500 ms 到期清空和长按期间自动清空。单次反馈完整出现/消失为 4 次刷新、15,844 像素、2 次小裁剪额度画布调用；静止一秒继续零刷新。
  - **安装状态**：4,027,648 字节、SHA-256 `93556d66ea52f84af0ae44739117d93feb9461ce6d337e0ef70f19075c9f93c6` 是加入共基线修复前的中间构建；这些交互现已包含在上条 4,027,424 字节的已刷写镜像中。实体 150 ms 手感、文字可读性和手指遮挡改善仍待用户确认。
- **2026-10-16 V2 5H/week 布局、数字中心锚点与滚轮触控外延已刷入 factory**：上方半圆弧、大数字及其下方重置时间显示 5H，再下方放置左右滚轮胶囊；下方横向渐变条、`WEEK n%` 及下方重置时间显示周额度。根据用户实机反馈，当前源码不再把 `数字 + %` 整体居中，而是以十位/个位分界锁定屏幕中心：百位/十位在左半区右对齐，个位与 `%` 从右半区起排。拆分仅发生在事件驱动的标签更新中，不增加静态画布逐帧运算。Today 统计仍来自周额度；5H 未知时仍显示 `--`，不借用周额度。
  - **主机验证**：实际 LVGL/PARTIAL 回归覆盖 5%、82%、100% 和未知值的中心分界，断言左侧标签结束与右侧个位起点均落在 x=233。左右滚轮可见胶囊仍为 84×34 px，每侧采集区从 90×46 px 扩到 104×62 px，中心保留 8 px 空隙；新增可见边界外的左上/右下触发检查，原松手一次、长按不连发、跨区/移出取消、Radial 防穿透和语音门控继续通过。静止一秒零刷新，录音一秒额度背景零重画，方向反馈仍为 20,572–20,850 像素；预览已更新，为示例数据、非设备实拍。
  - **最新构建与安装**：ESP-IDF v5.5.4 全量构建通过，factory 应用 4,027,056 字节，SHA-256 `baf707aea84cd49710965304988bcbd49bd1b9aedf4887085d574e84ada20899`；factory 6 MiB 可容纳，3 MiB ota_0 提示仍是既有双固件布局边界。15:49 经用户授权确认唯一端口 `/dev/cu.usbmodem21201` 为 ESP32-S3 rev 0.2，并仅用 `app-flash` 写入 `factory @ 0x20000`；写入工具内部校验和随后独立 `verify_flash` 均通过。
  - **回退与边界**：写前完整备份当前 factory 到忽略目录 `firmware-stopwatch-idf/build-v2/device-backup/quota-center-touch-20261016/factory.before.bin`（6,291,456 字节，SHA-256 `a51506cbe4ee163930525243652c290493843f4f42243e823757877898ff33bd`）；其前 4,026,416 字节 SHA-256 为上一版镜像 `5890565634e455e84e0946ea7c89ef806d50a5487450ebc6f6471d9948dfc528`。分区表写前/写后 SHA-256 均为 `dcefda33354c0a802a15d8357e1e08d79502aced9a944623e94ce3678e7514e0`，otadata 写后仍全为 `FF`。没有写 Bootloader、ota_0、NVS、storage 或配对数据，也未安装 Bridge、提交或推送。
  - **运行链路**：最后一次校验复位后，Bridge 重新发现 `M5Codex-SU`、完成原生 HID 交接、订阅设备事件并报告麦克风就绪；时间面板 seq=156、额度面板 seq=157 推送成功，虚拟输出仍为 48 kHz healthy。未打开串口日志；实体屏幕观感和扩大后触控手感仍待用户确认。
  - **上一版安装/运行**：12:08 经用户授权确认唯一端口 `/dev/cu.usbmodem21201` 为 ESP32-S3 rev 0.2；设备分区表与构建逐字节一致，factory 为 6 MiB、ota_0 为 3 MiB，otadata 为全擦除态。写前完整备份 factory 到忽略目录 `firmware-stopwatch-idf/build-v2/device-backup/quota-layout-20261016-120314/factory.before.bin`（6,291,456 字节，SHA-256 `ef35f2e73bdc27236b23e008dc069000d31438de98d675093a54e127d0777eb5`），并独立 `verify_flash` 匹配；随后只用 `app-flash` 写入 4,026,416 字节、SHA-256 `5890565634e455e84e0946ea7c89ef806d50a5487450ebc6f6471d9948dfc528` 的上一版到 `factory @ 0x20000`，写后独立验证匹配，分区表和 otadata 前后复读逐字节不变。没有写 Bootloader、ota_0、NVS、storage 或配对数据，也未安装 Bridge、提交或推送。
  - **启动后链路/实机边界**：为避免历史上打开串口额外触发 USB 复位，本轮不以串口采集启动日志。Mac 蓝牙状态重新列出已连接的 `M5Codex-SU`；Bridge 日志确认重新连接、事件订阅、`microphone_ready=true`、时间面板与额度面板推送完成，后续收到原生 reasoning 同步事件并继续成功推送额度面板 seq=38；48 kHz 虚拟输出 `output=healthy`、`route valid=true`。这些是安装和链路证据，不代替用户对真实圆屏布局、左右 Encoder、A/B、四 Agent、Radial、语音与息屏恢复的实体确认。

- **2026-10-15 Bridge 音频停止死锁已修复并安装，语音及显示由用户确认恢复**：用户报告 Linking、5H 不显示，随后出现 MIC LOST 并要求修复。两次 live 采样均确认 Bridge 主线程停在 AudioOutputUnitStop 的 Core Audio 锁，音频回调则在 CurrentDevice 查询等另一把锁，文件日志停于 19:18。标准 HID、Vendor HID 独立，不以任务切换正常推断 Companion/音频健康。分支 `codex/fix-bridge-microphone-stop-deadlock` 保留全部表盘工作。
  - **修复/构建**：Bridge v1.4.2 在停止前静音并关闭新回调、等待已有回调完成，再停止/释放音频单元；监听器只锁存静音，不递归查询。保留正常转换前后真实设备校验。完整 Swift 构建、并发排空、24 次真实活动回调停止/重建、虚拟输入回环及故障尾音归零、麦克风就绪与云同步回归通过；默认输入/扬声器/48 kHz 未变。
  - **安装/运行**：旧 v1.4.1 App 和两份故障采样备份在忽略目录 `tools/typeless_bridge/.build/device-backup/mic-stop-deadlock-20261015-200555/`，旧二进制 SHA-256 `46e2b2518b93290428f7c90210b8a51870369021768ce89322e18a34585514c2`。紧急重启后真实输出健康、连接额度推送恢复。20:14 新 v1.4.2 已安装，SHA-256 `2918530decd71c1951a540c5e6733352e565874075129716f2a3fc4af70bc1b2`，沿用原有 ad-hoc 签名方式、严格签名核验通过、配置哈希未变。进程实际输出审计只见允许的 M5 虚拟出口（48 kHz）；当前录音日志 215 包、lost=0、dropped=0，有开始与停止事件。连接额度 seq=2 和重连 seq=5 均推送成功；随后完整周期 seq=6、1454 字节、31 片、reason=quota 完成，确认并非只恢复一次性推送。未改驱动、系统扬声器/采样率或手表闪存。
  - **实体交互**：用户明确反馈“已恢复，录入和停止正常”，并确认顶部 Codex Ready 和 5H 数值均恢复。此前中断录音作废、未拼接。串口接收核验意外触发一次 USB_UART_CHIP_RESET（虽然请求 DTR/RTS 为 false）；已向用户说明。随后自动进入 factory/Codex、麦克风就绪配置为 true，设备日志确认收到 seq=5 的 1488 字节额度面板；无新固件构建/刷写。后续仅看 Mac 日志，不再次开启串口。新一轮长期稳定性、息屏/唤醒仍需观察；源码与文档纳入本次集成交付。

- **2026-10-15 19:40 底部紧凑电池/蓝牙图标已刷入，间距与观感待实机确认**：按用户指定，用左横向电池和右蓝牙图标替换四向提示文字，并移除两个旧状态点。电池仅显示电量数字、不带 `%`，≥30% 绿框、<30% 红框；蓝牙连接蓝色，断连红色加斜线。图标在四向面板与语音期间保留，普通 BLE 状态文字由图标替代，其他临时动作提示占用该行后恢复。读取现有 HAL 电量缓存和 BLE 连接真值，不改按键/通信/音频/分区。分支 `codex/v2-battery-bluetooth-footer`，旧工作保留。初版已刷入并独立校验，用户反馈与 2/3 号卡片重叠，继续要求缩小/靠近并去掉 `%`：当前电池 `(190,434,46,22)`、蓝牙 `(254,434,22,22)`，控件间距 18 px、距卡片下沿 7 px；
  - **主机预览/检查**：实际 LVGL 默认/低电/30% 边界/断连划线预览已更新，电池均仅数字。覆盖 0/29/30/31/100/非法电量，边框颜色、蓝牙蓝色/红线、无旧状态点、临时动作信息后恢复、与卡片至少 6 px 间隔（当前 7 px）、圆屏边界。静止一秒零刷新，单次电量变化仅更新 46×22 px、额度零重画；四方向反馈仍 20,572–20,850 像素、零额度重画，本次展开 130,496 / 关闭 99,436，未超过既定 110% 上限。原 Agent/Encoder/Radial、波形与语音门控/Bridge 就绪检查通过；刷新量不是实机端到端延迟测量。
  - **构建**：ESP-IDF v5.5.4 通过，应用 4,025,856 字节，SHA-256 `edaa281ffc7d2b9a792d98ac88317e8e90bffab8041181354ff37d22467c5bb4`。A/B 控制器、BLE Bridge、Vendor HID、BLE 麦克风源码仍与 `7feb001` 逐字节一致，版本和差异检查通过。
  - **安装/运行**：重核实际 ESP32-S3 v0.2、兼容分区和 factory 启动选择；旧应用 `firmware-stopwatch-idf/build-v2/device-backup/status-icons-compact-20261015-194719/application.before.bin` 独立 verify_flash 与设备匹配，SHA-256 `1117a836580a3d5254747636dd4e71c3f1110e8981dffd5eb5fda3d4390c21bb`。仅 app-flash 更新 factory @ 0x20000，写后和独立校验通过；分区表/otadata 复读未变。复位进入 Codex，V2 page ready 2859 ms，无崩溃记录；本轮设备串口确认 BLE 连接/加密与音频 Notify 恢复，当前进程输出审计确认允许的 48 kHz 虚拟出口。Bridge 文件日志停在 19:18，当前 stdout/stderr 为 /dev/null，未将旧日志当作本轮额度推送证据，也未修改 Bridge。
  - **实机边界**：初版图标的遮挡由用户实测反馈，已按要求缩小/靠近/下移、去掉 `%`；紧凑版新间距和显示观感、其他实体交互仍待用户确认。源码与文档纳入本次集成交付。

- **2026-10-15 19:11 圆形分区方向盘已刷入，观感及触控边界待实机复测**：用户确认上一版速度响应良好，再要求优化颜色和形状，并确认圆形分区方案。304 px 石墨黑圆底、四个圆头方向弧（外半径 132 px、厚度 64 px、跨度 40°），深灰/柔白默认、深青/青色按压；取消方框、描边、阴影和发光。中央关闭键可见直径 64 px、触控半径 40 px，方向触控使用 62–140 px 半径、各朝向 ±40° 的扇区，间隙不发方向。保留 200 ms 展开、原生 Radial 和松手归中、按下关闭、防穿透及语音收起。分支 `codex/v2-circular-radial-style`，旧工作保留；
  - **主机预览/验证**：实际 LVGL/PARTIAL 生成默认 `docs/assets/openwatcher-v2-radial-detail.png` 与按压 `docs/assets/openwatcher-v2-radial-detail-pressed.png`，均为示例数据、非设备实拍。四向按下/松手为 20,572–20,850 像素、额度背景零绘制，低于 25,000 上限；展开 124,456、关闭 107,436，与上一版相同，满足不超过 110%。逐像素验证所有实心弧落在对应触控区、每次松手恢复整帧一致；间隙/内外留白无方向、× 快速关闭且不依赖 view.update、原 Agent/Encoder/防穿透/语音收起归中/Processing 显示回归通过。语音启动门控、Bridge 就绪、版本和差异检查通过。指标为主机绘制负担，不代表实物端到端时间。
  - **构建**：ESP-IDF v5.5.4 通过，新应用 4,025,152 字节，SHA-256 `29f9eac308d7273ce5369a7d74fa8962b174f92044c493e3b2547a86f9ee0b93`；A/B 控制器、BLE Bridge、Vendor HID 和 BLE 麦克风源码与 `7feb001` 逐字节一致。
  - **安装/运行**：重核实际 ESP32-S3 v0.2、兼容分区与 factory 启动选择，旧应用 `firmware-stopwatch-idf/build-v2/device-backup/circular-radial-20261015-190822/application.before.bin` 独立 verify_flash 与设备匹配，SHA-256 `d42d4e41f7e6961b48c40168e8e4d150b02805ca4bef3dd4c02e9f2beed47a23`。仅 app-flash 写入 factory @ 0x20000；写后和独立校验通过，分区表与 otadata 复读未变。复位直接进入 Codex、V2 page ready 2843 ms，采集无崩溃；BLE/事件/额度面板恢复，虚拟音频 48 kHz 健康、进程出口审计通过。未写其他分区或替换 Bridge。
  - **实机边界**：用户已经确认上一版响应速度良好、此前麦克风问题解决；本次圆形观感、边缘/间隙触控、实际宿主方向动作及 A/B/A+B 等物理回归仍需用户确认。源码与文档纳入本次集成交付。

- **2026-10-15 18:58 四向面板展开/关闭响应修复已刷入，实际速度待复测**：用户确认面板能弹出，但展开与中央 × 关闭偏慢。定位并复现首次置顶导致父页面失效、单按钮反馈重刷整块 304×304 面板；本次预建固定大小的独立面板、按钮反馈局部刷新，× 改为 LVGL 按下事件即关闭并锁到松手。取消长按期间的逐帧进度环绘制，展开阈值仍为 200 ms；额度弧分段先做裁剪相交检查，避免恢复底图时提交远处的圆弧绘制任务。分支 `codex/v2-radial-response-fix`，此前工作保留；
  - **主机回归**：实际 LVGL/PARTIAL 完整展开刷新量 288,216→124,456 像素（原实现 250 ms 采样还未完整绘完，350 ms 才能计全）；方向按下/松手 184,832→14,112、零额度背景绘制；× 旧按压+关闭约 199,852，新按下关闭约 107,436。测试验证无需 view.update 的 LVGL 输入事件已关闭、按住不重开、四向/跨区/防穿透/录音归中、Idle/Processing 波形与触摸门控；静止与录音刷新指标未退化。关闭恢复的额度区域与打开前一致，新旧渲染器额度区逐像素一致。语音启动门控、Bridge 就绪、版本与差异检查通过。指标衡量渲染负担，不是实机端到端毫秒测量。
  - **构建**：ESP-IDF v5.5.4 通过，应用 4,025,024 字节，SHA-256 `d42d4e41f7e6961b48c40168e8e4d150b02805ca4bef3dd4c02e9f2beed47a23`。A/B 控制器、BLE Bridge、Vendor HID、BLE 麦克风源码仍与 `7feb001` 逐字节一致。
  - **安装/运行**：核对实际 ESP32-S3 v0.2、兼容分区和 factory 启动选择；旧应用 `firmware-stopwatch-idf/build-v2/device-backup/radial-response-20261015-185117/application.before.bin` 经独立 verify_flash 与设备匹配，SHA-256 `71cd0556b5ad82fad85c7cdd646476d6c6421c0c2bb056514f0ead718bf7e496`。仅 app-flash 写 factory @ 0x20000，写后及独立校验通过，分区表与 otadata 复读未变。复位进入 Codex、V2 page ready 2848 ms，采集无崩溃；BLE/事件/面板恢复，虚拟输出 48 kHz 健康、进程输出审计通过。未写其他槽或安装 Bridge。
  - **实机**：此前用户确认麦克风已解决、面板能够展开，但报告本轮修复前的展开与关闭慢。本轮响应速度、实体按键/方向实际动作仍待用户复测；不以主机刷新量或写后校验代替实机验收。源码与文档纳入本次集成交付。

- **2026-10-15 18:45 四向放大面板与短按方向已刷入，等待实机复测**：用户确认麦克风问题已解决；本次只修改视图与相关布局/检查，将中心长按由 280 ms 改为 200 ms，弹出 304×304 px 居中面板；开启手指须先松手，再短按方向。84×84 px 方向按钮使用原生 Radial，按下距离 1、松手归中；方向不随拖动切换，面板保持打开，可通过中央 × 或面板外点击关闭。语音进入自动收起并归中，四 Agent 保留 280 ms，A/B、音频和 Encoder 链路不变。分支 `codex/v2-radial-tap-detail`，此前未提交工作保留；
  - **主机检查**：实际 LVGL/PARTIAL 验证短触不展开、200 ms 阈值（含输入轮询/布局后 250 ms 主机推进可见）、原开启手指不选方向且松手后持续展开、四向按下角度/距离正确与松手归中、按住不重复提交、跨区归中/不重武装、中央/外侧关闭及底层 Agent/Encoder 防穿透、录音打断关闭与归中。展开后静止一秒零刷新；原四 Agent、双向 Encoder、Processing 波形收起/灰显/门控、语音启动门控、Bridge 就绪、版本及差异检查通过。主机时间不是实物触摸端到端测量。
  - **构建**：ESP-IDF v5.5.4 通过，应用 4,021,792 字节，SHA-256 `71cd0556b5ad82fad85c7cdd646476d6c6421c0c2bb056514f0ead718bf7e496`；A/B 控制器、BLE Bridge、Vendor HID、BLE 麦克风四个源文件与 `7feb001` 逐字节一致。
  - **安装/运行**：重核实际 USB 端口为 ESP32-S3 v0.2、分区与 factory 启动选择兼容；旧应用 `firmware-stopwatch-idf/build-v2/device-backup/radial-detail-20261015-183918/application.before.bin` 经独立 verify_flash 与设备匹配，SHA-256 `2a5c6d0aca2b5911567c3c9ec7899fa8bd73380ddfc74433de747af1c86d0f6b`。仅 app-flash 写入 factory @ 0x20000，新应用写后与独立校验通过；分区表和 otadata 复读未变。复位进入 Codex，V2 page ready 2845 ms，无崩溃记录；BLE/事件订阅/额度恢复，48 kHz 虚拟输出健康、进程输出审计通过。没有安装新 Bridge 或写小智/配对等其他分区。
  - **实机边界**：用户对上一安装版的麦克风问题明确反馈已解决；本次 0.2 秒展开手感、四方向实际宿主动作、A/B 与 A+B、息屏/恢复及其余物理回归仍需用户确认。源码与文档纳入本次集成交付。

- **2026-10-15 18:32 停止录音波形反馈修复已刷入 factory，等待实机复测**：用户已确认当前左右切换正常、语音录入基本正常；尚未确认停止录音后波形消失的手感。检查发现视图在 Recording 和 Processing 都显示动画，直到主机空闲或既有保护超时才收起。修复只在 Recording 显示波形，进入 Processing 即恢复灰显方向盘，保留顶部处理状态和触摸门控；不改变 A/B、主机识别时限、音频或 HID。分支 `codex/v2-waveform-stop-feedback`，保留此前未提交工作。
  - **主机验证**：实际 LVGL 在旧代码上复现波形未隐藏的断言失败；修复后 40 ms 主机推进内隐藏、方向盘真实像素灰显，持续 Processing 5.5 秒零动画刷新，Encoder/四 Agent/Radial 均不发动作，Idle 恢复。原触摸/局部刷新回归、语音启动门控、Bridge 麦克风就绪、版本与差异检查通过。A/B 控制器、BLE Bridge、Vendor HID 和 BLE 麦克风源码与 `7feb001` 逐字节一致。主机时间不等于物理按键端到端时间。
  - **构建**：ESP-IDF v5.5.4 通过，应用 4,020,512 字节，SHA-256 `2a5c6d0aca2b5911567c3c9ec7899fa8bd73380ddfc74433de747af1c86d0f6b`。
  - **安装/连接**：重核实际端口对应 ESP32-S3 v0.2，兼容分区和 factory 启动选择；旧应用备份 `firmware-stopwatch-idf/build-v2/device-backup/waveform-stop-20261015-183124/application.before.bin` 经独立 verify_flash 与设备匹配（SHA-256 `8aaafa64be8f8a516b3fcdff2c0a2dc360ccbcc9f31d99ddf5071c0dfce113bb`）。仅 app-flash 写入 factory @ 0x20000；写后与独立校验通过，分区表和 otadata 复读未变。复位进入 Codex，V2 page ready 2844 ms，采集无崩溃；BLE/事件/面板恢复，虚拟音频输出 48 kHz 健康，进程输出路由审计通过。未改 Bridge 或其他固件槽。
  - **实机**：用户对上一安装版的左右切换、基本语音录入反馈已记录；本次波形收起速度、A/B 与其余实体交互仍待复测。没有提交、推送；机器检查不代替实际录入与显示反馈。

- **2026-10-15 18:26 输入延迟修复与左右旋转点击版已刷入 factory，等待响应复测**：用户在上一版实机报告顶部滑动迟钝、A 键开始/停止录音变慢或失灵。当前 Bridge 仍为 Typeless；最近录音连续 3733 包且 lost/dropped 均为 0，不能据此排除启停响应问题。局部刷新复现发现全屏反馈层与未变化进度条造成无效刷新，波形还会反复调用整张额度背景绘制。已隔离动态区域、只更新变化属性；A/B 控制器、语音门控、HID 与音频源文件逐字节对照 `7feb001` 未变，没有改主机配置。
  - **最终交互**：按用户后续要求，上方取消滑动和全部文字，改为左右两个箭头按钮。左侧在原区松手发送一次左旋，右侧同理；移动超过 16 px、跨区、移出或开始录音取消整次触摸，长按不连发；中间保持 16 px 空隙，原生 Encoder 传输及宿主映射不变。分支 `codex/v2-input-latency-fix`，上一版未提交工作保留。
  - **主机检查/构建**：实际 LVGL 使用与设备相同的 PARTIAL/120 行缓冲；静止一秒从 52 次微小刷新（104 像素）降为 0，录音一秒额度图绘制 131→0，最终两个点击合计 25,024 像素且额度图零绘制。覆盖左右松手一次、按住不提交/不连发、跨区/拖动/移出/语音取消、四 Agent、四向/归中及语音门控；版本与差异检查通过。指标是主机绘制工作量，不是设备按键响应时间。ESP-IDF v5.5.4 构建通过，新镜像 4,020,544 字节，SHA-256 `8aaafa64be8f8a516b3fcdff2c0a2dc360ccbcc9f31d99ddf5071c0dfce113bb`。
  - **安装/连接**：重核 ESP32-S3 v0.2、分区和 factory 启动选择，并独立验证回退应用 `firmware-stopwatch-idf/build-v2/device-backup/input-latency-20261015-181630/application.before.bin` 与设备旧应用匹配（SHA-256 `738115b061ccbc6d8e630f1a10cf0402f7a73229aa3ee9a70d4e6f25178a6aa2`）；然后只 `app-flash` 更新 `factory @ 0x20000`。写后 hash 与独立 verify_flash 通过，分区表及 otadata 复读未变。USB 复位进入 Codex，V2 page ready 2850 ms，无崩溃记录；BLE、面板和麦克风就绪恢复，虚拟输出 48 kHz 健康，重连后进程音频路由审计通过。未更新小智、Bootloader、NVS、storage、配对或 Bridge。
  - **仍待用户确认**：顶部左右点击和 A 键启停的真实响应、实际语音识别及其余触摸/按键回归。主机与路由检查不代替此次修复的实机手感确认；源码与文档纳入本次集成交付。

- **2026-10-15 17:50 经用户授权，弧形任务卡表盘已刷入 factory，实机交互待验收**：重核实际端口对应 ESP32-S3 v0.2，设备分区表与构建一致，factory 为 `0x20000 / 6 MiB`、小智 ota_0 为 `0x620000 / 3 MiB`，otadata 全擦除，选择 factory。先完整备份旧 factory 到本机忽略目录 `firmware-stopwatch-idf/build-v2/device-backup/task-deck-20261015-174305/factory.before.bin`（6,291,456 字节，SHA-256 `eac8ec60a8f3f7207dd4e809d6b1f0e8faaa467b665b5a218daaed3f9e4a047a`）；备份中旧应用前 4,113,472 字节的 SHA-256 与此前安装镜像 `aa8ac2ba214c2963755c71c0c247b03d77b690d5426e3a29bec6241db59b8939` 一致。
  - **刷写**：`idf.py -B build-v2 app-flash` 仅写入 factory 应用，4,020,800 字节，SHA-256 `738115b061ccbc6d8e630f1a10cf0402f7a73229aa3ee9a70d4e6f25178a6aa2`。写后 hash 与独立 `verify_flash` 均通过；复读确认分区表及 otadata 与写前逐字节一致。未写 Bootloader、小智、NVS、storage 或配对数据，没有安装新 Bridge、改主机 Knob 配置、提交或推送。
  - **启动与连接**：USB 复位串口确认从 `factory @ 0x20000` 启动、`startup target: Codex`、`V2 page ready: 2854 ms since boot`，25 秒采集中没有崩溃/异常复位。Bridge 恢复原生 HID 后重连，设备事件订阅、Typeless 配置和 `microphone_ready=true` 同步完成，额度面板重新推送；虚拟输出 `output=healthy`、`route valid=true`、48 kHz，运行进程输出审计通过且只使用允许的虚拟设备。这些检查不等于真实语音录入或触摸动作验收。
  - **实机待验收**：屏幕布局与文字、四卡短触取消/长按一次/实际任务切换、旋钮双向、自定义映射、四向输出与松手归中、A/B 与 A+B、真实语音、息屏恢复、执行期间亮屏。没有替用户执行这些操作。

- **2026-10-15 弧形任务卡表盘构建阶段记录（同日后续刷写见上）**：按用户确认图将四个 Agent 点改为编号/中文状态任务卡，加入精简旋钮滑动入口和常驻四向方向盘；保留 280 ms 长按、原生 Encoder/Radial、A/B、语音门控与三条 BLE 链路。5H 条内使用红→橙→黄→绿连续渐变，越界、未知或过期不伪造剩余量；24 个活动桶按时间从左到右显示。未就绪显示离线、无灯效状态显示未知，灯效不作为发送门槛；一次触摸只提交一个槽位，确认前滑动换目标重新计时。中文字体为 30 个字形的子集。任务分支 `codex/v2-task-deck-ui`，源码回退比较基线 `7feb001`；没有提交或推送。
  - **主机验证**：实际 `view.cpp`、生产 LVGL 包装类、LVGL 与字体编译为主机渲染器，仅替换 HAL。圆屏四角、触摸区互斥、四槽位、短触取消、一次提交、确认前后滑动、未分配灯效槽位、旋钮双向、四向方向/距离、松手和录音归中、录音禁止触摸、5H 渐变与越界轨道检查通过；生成正常、旋钮、四向、录音、中断、离线画面。语音启动门控、Bridge 麦克风就绪、版本一致性与差异检查通过。示例渲染见 [使用说明](docs/CODEX_TASK_DECK.md)，不视为设备实拍或 BLE 实机验收。
  - **固件构建**：使用现有 ESP-IDF v5.5.4 与本机依赖，`idf.py -B build-v2 build` 通过。`build-v2/StopWatch-UserDemo.bin` 为 4,020,800 字节，SHA-256 `738115b061ccbc6d8e630f1a10cf0402f7a73229aa3ee9a70d4e6f25178a6aa2`，可放入 6 MiB factory。构建提示该镜像大于独立小智 ota_0 的 3 MiB，为双固件布局既有边界；未调整分区。没有安装 Bridge、改主机 Knob 配置、访问设备闪存或写入持久数据。
  - **安装和实机**：本次未刷写。仍待检查圆屏实物观感、短触取消/长按一次、四 Agent 实际切换、旋钮双向、Radial 四向与归中、A/B 与 A+B、BLE 重连、真实语音和虚拟麦克风健康、息屏恢复及任务执行期间亮屏。下方旧安装记录仍描述旧镜像，不能用来接受本次新布局。

- **2026-10-15 经用户授权，V2 单主题与开机直达已刷入 factory**：刷写前核对实际 USB 端口对应 ESP32-S3 v0.2，设备分区表与构建一致，factory 为 6 MiB、偏移 `0x20000`，小智 ota_0 为 3 MiB、偏移 `0x620000`，otadata 为全擦除态，选择 factory。先完整备份旧 factory 6 MiB 到本机忽略目录 `firmware-stopwatch-idf/build-v2/device-backup/factory.before.bin`（SHA-256 `4365fd904781b0f571829bb2e29a8b7011fd371e93ac52def63aa7402b385d32`）；随后 `idf.py -B build-v2 app-flash` 只写入上述 factory 应用，写后 hash 和独立 `verify_flash` 均匹配，复读确认分区表与 otadata 未变化。安装镜像 4,113,472 字节，SHA-256 `aa8ac2ba214c2963755c71c0c247b03d77b690d5426e3a29bec6241db59b8939`。USB 复位后的串口记录显示从 factory 启动、`startup target: Codex`、`V2 page ready: 2865 ms since boot`，没有 Launcher on open 或崩溃/复位异常；这是复位启动初始化时间，不是完整断电冷启动或旧版对照测试。Bridge 已恢复原生 HID 后重连，设备事件与音频订阅完成，Typeless 配置及 microphone_ready=true 同步成功，时间和额度面板重新推送；麦克风 output=healthy、route valid=true、48 kHz，进程输出审计通过且只使用允许的虚拟设备。未安装新 Bridge，未写 Bootloader、小智、NVS、storage 或配对数据。实际屏幕、断电开机、A/B、短触取消/长按一次提交、四 Agent、推理双向、Radial 归中和真实语音录入仍待用户操作确认；不将启动/订阅/路由日志等同于实机交互验收。

- **2026-10-15 V2 单主题与开机直达构建记录（刷写前）**：删除旧 Codex Micro Dashboard、专属绘制/触摸逻辑和 Setup 的 Codex Theme 选择项；固定初始化 OpenWatcher V2，不再读写历史 `theme` / `theme_gen`。保留 Codex Micro 原生输入模式与现有 A/B 状态机。StopWatch 主固件启动时通过 Launcher 原有生命周期直接打开 Codex，不创建 Launcher 首页、不等待首次引导、不播放 Launcher 开机音；Codex 打开时清除 BootLogo，A+B 关闭后回到 Launcher。首次触摸及手势轮询由空闲 100 ms 缩短至约 33 ms（主循环调度影响实际间隔），长按确认仍为 280 ms。主机启动生命周期检查通过：自动进入 Codex、不创建 Launcher UI、退出后正常返回且不自动重入、缺少 Codex 时安全回退；语音门控、麦克风就绪、Launcher 快捷键、Focus、版本与差异检查通过。ESP-IDF v5.5.4 全量构建成功，`build-v2/StopWatch-UserDemo.bin` 为 4,113,472 字节，SHA-256 `aa8ac2ba214c2963755c71c0c247b03d77b690d5426e3a29bec6241db59b8939`；比 2026-10-12 最后记录的 4,121,808 字节镜像减少 8,336 字节。保留原 build 产物，现有依赖未刷新。构建阶段尚未刷写；后续安装证据见同日刷写记录。实际冷启动、画面、实体操作与空闲功耗仍需单独验收。源码回退基线为 `649f274`，任务分支 `codex/v2-only-fast-start`，尚未提交。

- **已由用户在原安装组合上确认**：OpenWatcher V2 显示 5H 与周额度，并曾自动刷新为 5H 6% / 周 28%。这证明当时 Bridge 到手表的额度链路有效，不证明本目录产物已安装。
- **2026-09-17 在当前安装组合上由用户实机确认**：V2 中心长按后的上、下、左、右四向 Radial，手表反馈和 Codex 动作均正常。此结果仍不代表本目录新构建产物已刷入手表。
- **2026-09-17 在当前安装组合上由用户实机确认**：V2 顶部向左、向右滑动分别降低、提高 Codex 推理等级。故障原因是本机 Codex Micro 的 `encoderMode` 为 `composer-navigation`，原生 Encoder 事件被用作上下导航。已将 `~/.codex/config.toml` 改为 `reasoning` 并保留原配置备份；运行中的 Codex 还需在「设置 → Codex Micro → Knob」选择 `Reasoning only` 才立即生效。Bridge 的 `codex_reasoning:native_sync` 日志本身不能代替 Codex 界面的操作结果。
- **V2 页面说明**：底部已用电池和蓝牙图标替代两个小圆点与四向提示文字。历史线与圆点图形为同步装饰。V2 当前没有第二页或页切换手势。当前只保留 OpenWatcher V2，历史主题选择入口已移除。
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
