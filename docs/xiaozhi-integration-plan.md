# 小智 AI 接入方案（历史设计稿）

文档版本：0.2；最后更新：2026-09-17 14:26。

**当前状态：**本文件保留 2026-09-16 的方案推演、风险和当时的上游比较，以下“尚未实施”、旧分区尺寸、固件体积和待办顺序均为历史快照，不能作为现在的构建或刷写指引。现已采用**双固件启动槽**：StopWatch/Codex 在 `factory`（6 MiB），小智 v2.2.6 板卡适配在 `ota_0`（3 MiB），`storage` 为 4 MiB。主固件通过 Launcher/Setup 检查小智镜像并切换，小智可长按 B 约 3 秒或调用 MCP 返回工具切回。当前设备安装/语音验收以 [项目状态](../PROJECT_STATE.md) 为准；实际分区以 `firmware-stopwatch-idf/partitions.csv` 和 `firmware-xiaozhi/partitions/stopwatch_dual.csv` 为准。

> 历史状态：此处记录当时的方案评审稿；现在已有双固件实现。
> 依据：本仓库现有固件 `firmware-stopwatch-idf`（v0.10.7 / ESP-IDF v5.5.4）、上游 `78/xiaozhi-esp32`。
> 日期：2026-09-16

---

## 0. 结论速览（TL;DR）

| 问题 | 结论 |
| --- | --- |
| 能不能加？ | **能**，且成本比预期低——现有固件已经用了小智同款 `78/esp-wifi-connect` 配网、同为 LVGL 9.5.0、同为 ESP32-S3 + ES8311 音频架构 |
| 走哪条路？ | **修订结论：方案 B（双固件 + 运行时切换）作为第一阶段**——故障域隔离、对现有功能零回归风险、总工作量反而更小；方案 A（单固件融合）降级为"若确实需要共存"的第二阶段（板卡适配代码可复用）。方案 C（另一台设备）作零风险预热 |
| 版本必须锁死 | **`78/xiaozhi-esp32` v2.2.6**（要求 IDF ≥ 5.5.2，LVGL ~9.5.0）——与我们的 IDF 5.5.4 / LVGL 9.5.0 **完全对齐** |
| ⚠️ 不要用最新版 | v2.4.2 主线转向 IDF 6.0，v2.5.0 **强制 IDF 6.0.1+ 且明确不再支持 5.x**。用最新版＝必须整体升级 IDF，会牵动 M5GFX / mooncake / esp_codec_dev / 全部 HAL，风险不可接受 |
| 最大的三个技术风险 | ① LVGL 双所有者冲突 ② ES8311 采样率与 I2S 占用仲裁 ③ WiFi + BLE 共存下的功耗与续航 |
| 日程/待办/笔记 | **双通道**：语音走小智「MCP 接入点」（`wss://api.xiaozhi.me/mcp/?token=…`）→ Mac 上的 MCP Server → SQLite/Markdown；**手表展示另走现有 BLE 桥**，不依赖云端、不额外耗手表 WiFi |
| 分区表要不要动？ | **可以不动**。16MB 尾部还剩约 2.8MB 未分配，唤醒词 `model` 分区可以塞在尾部，`factory`/`storage`/`nvs` 全部保持原位（不会丢已配网凭据和用户上传的 badge 图） |
| 启动时选固件？ | 机制**官方支持**（`esp_ota_set_boot_partition`），但**不建议做"开机选择菜单"**——需要一个额外的 selector 程序分区 + 每次开机都要等待选择。建议做成**运行时入口**：设置里点"切换到小智" → 一次重启到位 |
| 第一步建议 | P0：**完全不碰手表固件**，先在 Mac 上把日程/待办/笔记 MCP Server + 接入点跑通（用手机小智 App 或一块现成小板验证），确认闭环后再动固件 |

---

## 1. 现状盘点

### 1.1 硬件（M5Stack StopWatch）

| 项 | 参数 | 对小智的意义 |
| --- | --- | --- |
| SoC | ESP32-S3, 240MHz 双核 | 小智主力平台，官方支持 |
| Flash | 16MB（`CONFIG_ESPTOOLPY_FLASHSIZE_16MB`） | 充足，尾部有 ~2.8MB 未分配 |
| PSRAM | 8MB Octal @80MHz | 远超小智最低要求（2MB 即可跑） |
| 显示 | CO5300 AMOLED，466×466 圆屏（M5GFX `Panel_AMOLED`） | 需要自写板卡适配 |
| 音频 | **ES8311**（I2S0：MCLK18/BCLK17/DIN16/WS15/DOUT21），44100Hz，麦克风+喇叭共用 | 小智的 `Es8311AudioCodec` 直接对口 |
| 触摸 | CST820（经 M5IOE1） | 按键/触摸唤醒可用 |
| 其他 | BMI270 IMU、RX8130 RTC、M5PM1 PMIC、震动马达、BLE(NimBLE) | 提醒/震动/省电可复用 |
| 按键 | `btnA` / `btnB` / `btnPwr` | **按 B 说话**是小智天然入口 |

### 1.2 固件架构（关键前提）

- 应用框架：`mooncake` + `smooth_ui_toolkit`，App 通过 `GetMooncake().installApp(...)` 注册，`AppLauncher` 以磁贴形式启动（`main.cpp`、`app_launcher.cpp`）。
- 现有 App：`AppCodex`、`AppLauncher`、`AppWatchFace`、`AppLuckyWheel`、`AppMaze`、`AppSetup`。
- 硬件抽象：`main/hal/*`（`hal_audio.cpp` 封装 ES8311 + `esp_codec_dev`；`hal_display.cpp` 封装 CO5300；`hal_bluetooth.cpp` 管 NimBLE）。
- 配网：`78/esp-wifi-connect` 3.1.3（`WifiManager` + `SsidManager`）——**与小智同源**，小智的 `WifiBoard` 用的就是这套 API。
- 语音链路（现有，来自 vibecoding 分支）：BLE 麦克风，16kHz IMA-ADPCM 实时推到 Mac 虚拟麦克风 `M5 StopWatch Mic`；BLE HID 发送 A/B 键事件。
- 云端数据链路：Mac 上 Swift 桥 `stopwatch_ble_bridge` 通过 BLE 把 panel JSON（Codex 额度）推给手表，固件 `app_codex` 解析显示。
- 当前固件体积：**5.55MB / `factory` 分区 9MB**（余量 3.45MB）。

### 1.3 分区表现状

```
nvs       0x9000     16K
otadata   0xd000     8K
phy_init  0xf000     4K
factory   0x20000    9M      ← 现有固件（无 ota_0/ota_1，USB 升级）
storage   auto(4M)   FAT     ← badge 图片 / 用户资源
coredump  auto       64K
                          ← 0xD30000 ~ 0x1000000 约 2.8MB 未分配
```

---

## 2. 上游版本选择（唯一低风险组合）

`78/xiaozhi-esp32` 的 ESP-IDF 要求随版本抬高，这是本方案的第一约束：

| 版本 | 发布时间 | ESP-IDF 要求 | 与本项目 |
| --- | --- | --- | --- |
| **v2.2.6** | 2026-04-19 | **≥ 5.5.2**；`lvgl ~9.5.0`；`78/esp-wifi-connect ~3.1.2` | ✅ **完全匹配**（我们 IDF 5.5.4 / LVGL 9.5.0 / wifi-connect 3.1.3） |
| v2.4.2 | 2026-08-06 | 主线 IDF 6.0（5.5 仅保留给遗留板卡） | ⚠️ 勉强可行，迁移面大 |
| v2.5.0 | 2026-09-10 | **IDF ≥ 6.0.1，5.x 不再支持** | ❌ 需整体升 IDF 6 |
| v2.1.0 / v2.0.5 / v1.9.4 | — | IDF ≥ 5.4 | 兼容但功能较旧 |

**决定：锁定 v2.2.6**，并以 git submodule / 固定 tag 的方式引入，避免上游漂移。

> 事实核对（`v2.2.6/main/idf_component.yml` 末行）：`idf: version: '>=5.5.2'`。

### 2.1 v2.2.6 需要引入的依赖（新增部分）

`esp_audio_codec`（Opus 编解码）、`esp_websocket_client`、`esp_lvgl_port`、`78/xiaozhi-fonts`、`esp-sr`（可选，仅唤醒词/AEC 用）、`esp_lcd_co5300`（若不复用 M5GFX）。

已有、无需重复引入：`lvgl`、`esp_codec_dev`、`78/esp-wifi-connect`、`ArduinoJson`、`esp_http_client`、`mbedtls`、`fatfs`。

小智 v2.2.6 的 `idf_component.yml` 为了支持 138 块板卡引入了大量 LCD/Touch 驱动，融合时必须**裁剪 manifest**，只保留本机所需，否则依赖解析和体积都会失控。

---

## 3. 三条集成路线对比

| | **方案 A：单固件融合**（推荐） | **方案 B：双固件 A/B 切换** | **方案 C：独立设备先验证** |
| --- | --- | --- | --- |
| 做法 | 小智核心（audio / protocols / mcp_server / application）移植进现有工程，作为 `AppXiaozhi` | 保留现有固件在 `factory`；另建独立工程用 IDF 6.x 编译 xiaozhi 到 `ota_0`，两边加"切换系统"入口写 otadata | 买一块小智官方支持板卡（CoreS3 / AtomS3R+Echo Base / ESP32-S3-BOX-3），刷官方固件，先跑 MCP |
| 保留现有功能 | ✅ 全部（Codex 表盘、BLE 麦克风、HID、省电、配网共用） | ✅ 全部，但**不共存**：切到小智时 BLE 桥/Codex 表盘不可用 | ✅ 完全不动手表 |
| 切换体验 | 磁贴点开即用，秒级 | 需重启（~5s）+ 两套 UI 风格割裂 | 双设备 |
| WiFi 凭据 / NVS | 共用一套 | 两套，要配两次 | 不涉及 |
| 工作量 | 中高（3-6 天，风险集中在音频/LVGL） | 中（2-4 天，但需重排分区、FAT 要缩容 → 用户 badge 图会丢） | 低（0.5-1 天） |
| 技术风险 | LVGL 所有权、采样率仲裁、共存功耗 | 低（零耦合） | 极低 |
| 长期维护 | 一份工程、一次 OTA | 两份工程、两套依赖、同步上游两遍 | 无 |

### 推荐组合

> ⚠️ 初稿此处推荐 A 为主线。**经 3.5 节评估后已修订**，以 3.5.8 的路线为准：

```
P0  方案 C（可选）：Mac 侧 MCP Server + 接入点先跑通 —— 把"服务端不确定性"清零
P1  方案 B（主线）：双固件骨架 —— 分区重排 + 小智板卡适配 + 两侧"切换系统"入口
P2  方案 B 续：日程/待办/笔记展示（小智侧走 WiFi 内网，表盘侧走 BLE 桥）
P3  决策点：若"两套系统不互通"确实难受，再用已验证的板卡适配转做方案 A 融合
```

---

## 3.5 方案 B 深入评估：双固件 + 运行时切换（修订结论）

> 本节为 2026-09-16 追加，结论**推翻了初稿的路线排序**：双固件不是兜底，而是更合适的起点。

### 3.5.1 机制：完全基于 ESP-IDF 原生能力（已在源码逐条核实）

| 动作 | 实现 | 出处（`components/app_update/esp_ota_ops.c`） |
| --- | --- | --- |
| 切到 OTA 槽（小智） | `esp_ota_set_boot_partition(ota_0)` → `esp_rewrite_ota_data()` 重写 otadata 的 `ota_seq` | `:599-637` |
| 切回 `factory`（现有表盘固件） | 直接**擦除整个 otadata 分区**，引导器随即回到 factory | `:609-620` |
| otadata 无效时的引导顺序 | **factory 优先**；无 factory 则取第一个 OTA 槽 | `find_default_boot_partition()` `:644-668` |
| 目标槽镜像不合法 | `image_validate(ESP_IMAGE_VERIFY)` 失败 → `ESP_ERR_OTA_VALIDATE_FAILED`，**不会变砖** | `:605-607` |

三条直接推论：

1. **切换零 hack** —— 不改二级引导器、不需要自定义 bootloader、不需要 GPIO 跳线。
2. **天然救援通道** —— 擦掉 otadata 就回到现有固件，等于手表内置一个"保底系统"。
3. **未安装的槽会安全报错** —— UI 可以提示"目标系统未安装"，而不是切进黑屏。

配套约束（需在 sdkconfig 明确）：

- `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE` **保持关闭**。开启后槽可能被标记 `PENDING_VERIFY`/`INVALID`，与"手动切换"的语义打架；
- 若将来必须开启，两个固件每次启动都要调用 `esp_ota_mark_app_valid_cancel_rollback()` 自证可用。

### 3.5.2 但不建议做"开机选择菜单"

| 做法 | 代价 |
| --- | --- |
| 独立 selector 程序放 `factory` | 多占 1MB+ flash；每次开机先过选择界面；总重启 2 次 |
| 改 bootloader 读 GPIO / NVS 选择 | 动二级引导器，出错即砖；收益仅是"少点一次" |
| **运行时入口（推荐）** | "设置 → 切换到小智" → 写 otadata → 重启一次到位。切换是低频动作，不该拖慢每一次开机 |

若要保留"开机可选"的手感，等价折中是：两个系统里都做**开机 3 秒内按住 A 键进入切换确认**。不需要第三个分区。

### 3.5.3 稳定性：方案 B 明显更好

| 维度 | 方案 A 融合 | 方案 B 双固件 |
| --- | --- | --- |
| 故障域 | 共用——小智侧内存泄漏 / 看门狗 / 协议栈崩溃会波及 Codex | **完全隔离** |
| 音频 | ES8311 采样率仲裁 + 消费者互斥锁 | 各自独占，零冲突 |
| 显示 | LVGL 双所有者，需自写适配层 | 各自独占，零冲突 |
| 射频 | WiFi + BLE 共存调度与功耗权衡 | 各自独占 |
| Flash | 两个功能挤 9MB（估 7.5~8.5MB，偏紧） | 两个 6MB 槽，各有余量 |
| 回滚 | 出问题只能 USB 重刷 | **另一个槽就是回滚目标** |
| 对现有功能的回归风险 | 中（动到 LVGL / 音频 / 电源） | **接近零**（现有固件只加一个菜单项） |

### 3.5.4 代价（必须接受的四条）

1. **小智系统里没有 Codex 能力**：BLE 桥、额度显示、语音输入到 Mac、HID 按键都在另一个固件内。
   → 出路：(a) 接受"两套系统各管一段"；(b) 日程展示在小智侧改为 **WiFi 内网直连 Mac 的 HTTP 服务**（小智侧本就有 WiFi，规避 BLE 依赖）。
2. **切换要重启**：约 3~8 秒回到界面，WiFi 可用再等 5~15 秒。无法做到"随口叫小智"。
3. **两套 UI 风格**：小智原生界面 vs 现有 Codex 表盘体系，观感上像两块表。
4. **维护两份构建**：两个 IDF 环境（5.5.4 + 6.x）、两份依赖清单、升级分别烧写。

### 3.5.5 分区预算（最终落地方案：只缩 factory，storage 一字不动）

```
nvs        0x9000    0x4000      16K    ← 与原固件完全一致（配网/配对/BLE bond 保留）
otadata    0xd000    0x2000       8K    ← 一致
phy_init   0xf000    0x1000       4K    ← 一致
factory    0x20000   0x600000     6M    ← 现有 Codex 固件（实测 5.55MB，余 ~465KB）
ota_0      0x620000  0x300000     3M    ← 小智固件（实测 2.826MB，余 ~178KB）
storage    0x920000  0x400000     4M    ← 【offset 与 size 完全不变】FAT + wear levelling
coredump   0xd20000  0x10000     64K    ← 一致
（尾部 0xd30000 ~ 0x1000000，2.81MB 未分配，预留给以后的 assets / 唤醒词 model）
```

关键点：

1. **`storage` 的 offset 与 size 都没变**，所以 wear levelling 不会重格式化，badge 图不丢，**也不需要搬移任何数据**。
   WL 的全部状态都按 `start_addr + 相对偏移` 寻址（`components/wear_levelling/WL_Flash.cpp:84-86`），只在分区 **size** 变化时才校验失败。
2. **`nvs` 位置不变** → 配网凭据（NVS namespace `"wifi"`）与 BLE bond 保留，两个固件共用同一份 WiFi 配置。
3. **只缩 `factory`（9M → 6M）**：它只是 app 槽、不存任何持久数据，缩小安全。
4. **为什么必须缩 factory**：初稿打算把 `ota_0` 塞在 16MB 尾部，但尾部只剩 2.81MB，而小智 v2.2.6
   实测 app = **2.826MB（2962976B）**，会**溢出约 13.5KB**。缩掉 factory 的 3MB 后，`ota_0` 涨到 3MB，余量 178KB。
5. `ota_0` 只要 3MB 而非初稿的 6MB：小智 app 是固定体积（本项目没有 assets/model 分区），3MB 足够；
   尾部 2.81MB 留白，二期要加 `assets` / `model` 时直接用，**不必再动 storage**。
6. **两个槽都获得 OTA 能力**（原有单 `factory` 布局根本无法 OTA，因为 M5 的 BootLoader 是自定义的）——
   小智的 `self.upgrade_firmware` 也因此可用，这是双固件的额外收益。

### 3.5.6 一次性刷写成本（零数据损失）

1. **必须刷一次分区表**（`partition-table.bin` @0x8000）。用**现有 StopWatch 工程**产出的那一份：
   该工程 `CONFIG_PARTITION_TABLE_MD5=y`，与原引导器配置匹配（两份 csv 内容完全一致）。
2. **引导器不换**：现有 bootloader 是通用 ESP-IDF 引导器，运行期读分区表，且
   `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE` 已是 `n` → 直接把 xiaozhi 烧到 ota_0 即可。
   ⚠️ **绝对不要烧小智自带的 bootloader**（它的 rollback 配置不同）。
3. **`storage` 不会被重新格式化**（size 未变）→ badge 图、`ACTIVE_SLOT` 全部保留，**无需备份/重传**。
   实测 4MB storage 备份里确有 `BADGE/SLOT_0.JPG`、`ACTIVE~1.TXT`；备份留存于
   `vibecoding/.backup/flash-20260916/storage_orig_0x920000_4M.bin` 作为保险。
4. **配网凭据保留，且两边共享**：`78/esp-wifi-connect` 的凭据存在 NVS 命名空间 `"wifi"`（`ssid_manager.cc:8`），nvs 分区未动 → **只需配一次网**。
5. **NVS 命名空间约定**：现有固件自用 `"system"`，小智用自带前缀；约定两者不共用自定义命名空间，避免 schema 互相污染。
6. **BLE 配对不受影响**：bond 数据留在 NVS，切到小智时不会被触碰；切回表盘无需重新配对。
7. **小智侧必须做的两处配置修正**：
   - `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE` 覆盖成 `n`（小智 `sdkconfig.defaults` 里默认是 `y`，
     在 `sdkconfig.defaults.stopwatch` 末尾覆盖，因为它排在 `SDKCONFIG_DEFAULTS` 最后、优先级最高）。
     否则从 ota_0 启动会被标记 PENDING_VERIFY，未 confirm 就重启会回滚到 factory。
   - 本项目**没有 `assets` / `model` 分区** → 小智走"无离线资源"路径：**按键对讲可用**，
     但没有离线唤醒词与表情动画。运行期 `esp_srmodel_filter()` 有显式 NULL 保护
     （`managed_components/espressif__esp-sr/src/model_path.c:558`），缺分区**不会崩**：
     `wake_word_` 为 `nullptr`，`AudioService::EnableWakeWordDetection()` 首行即 return。
   - 交互入口：Button A(GPIO2) 单击说话 / 长按配网；Button B(GPIO1) 长按切回原生表盘。

### 3.5.7 关键结论：板卡适配不是重复劳动

双固件必须先写一份完整的小智板卡适配（CO5300 显示、ES8311 编解码、M5PM1 电源/充电、CST820 触摸、RX8130 RTC、震动马达）。
**这份工作是方案 A 的必要条件中的同一份**：引脚映射、面板初始化序列、codec 寄存器配置、电源时序都已验证，方案 A 可直接复用（约 50~60%；差异仅在宿主类——A 要挂到现有 LVGL/ES8311 实例上，B 用的是小智自带的 `Display`/`Codec` 抽象）。

**所以"先做双固件"不是沉没成本，它就是融合方案的第一阶段。**

### 3.5.8 修订后的路线

```
P0  方案 C 预热（可选）：Mac 侧 MCP Server + 接入点先跑通，把服务端不确定性清零
P1  方案 B：双固件骨架 —— 分区重排 + 小智板卡适配 + 两侧"切换系统"入口
              （此时已得到一台可用的手表版小智）
P2  方案 B 续：日程/待办/笔记展示（小智侧走 WiFi 内网，表盘侧走 BLE 桥）
P3  决策点：若"两套系统不互通"确实难受 → 用已验证的板卡适配转做方案 A 融合
```

### 3.5.9 实机验证结果（2026-09-16，已跑通 ✅）

刷写内容（全部哈希校验通过）：

| Offset | 文件 | 体积 |
|---|---|---|
| 0x0 | `bootloader.bin`（沿用原 StopWatch 引导器） | 20,832 B |
| 0x8000 | `partition-table.bin` | 3,072 B |
| 0xd000 | `ota_data_initial.bin`（全 0xFF，清空 otadata） | 8,192 B |
| 0x20000 | `StopWatch-UserDemo.bin` | 5,823,696 B |
| 0x620000 | `xiaozhi.bin` | 2,962,672 B |

回读校验：factory 槽 = `StopWatch-UserDemo` v0.10.7；ota_0 槽 = `xiaozhi` **v2.2.6**。

小智实机启动日志（关键行）：

```
I (83)  boot:  4 ota_0            OTA app          00 10 00620000 00300000
I (652) boot: Loaded app from partition at offset 0x620000
I (732) app_init: Project name:     xiaozhi
I (736) app_init: App version:      2.2.6
I (869) Board: UUID=<设备标识>  SKU=stopwatch          <- 自定义板卡识别正常
I (5359) WifiStation: Found AP: <本机 Wi-Fi SSID>, RSSI: -22
I (10039) WifiBoard: Connected to WiFi: <本机 Wi-Fi SSID>    <- 复用 NVS 里已存的 WiFi
W (13249) Application: Alert [link] 激活设备: xiaozhi.me  <- 待账号激活
```

结论：

1. **切换机制端到端可用**：写 otadata → 重启 → 引导器加载 ota_0 → 小智正常启动；重启后稳定停在小智。
2. **板卡适配有效**：`SKU=stopwatch` 被识别，M5IOE1 IO 扩展器、CO5300 屏、ES8311 编解码都初始化通过。
3. **WiFi 凭据共享成立**：小智直接连上 NVS 里已有的 AP，**不需要重新配网**（印证 `"wifi"` 命名空间共用）。
4. **storage 零损失**：分区表换了，但 badge 图与配网凭据都保留（storage 的 offset/size 未变）。
5. **无效 otadata 的安全兜底也实测到了**：引导器会打印
   `E boot: ota data partition invalid, falling back to factory` 并回到 factory，**不会黑屏**。

> ⚠️ 排障备忘：手工构造 otadata 做验证时，CRC 必须用
> `esp_rom_crc32_le(UINT32_MAX, &ota_seq, 4)`，即等价于 `zlib.crc32(le32(seq), 0xFFFFFFFF)`，
> **不是** `zlib.crc32(le32(seq))`。用错会被判无效并回落 factory（安全，但会误以为"切换失败"）。

### 3.5.10 下一步（等用户确认）

- 在小智侧完成 `xiaozhi.me` 账号激活 → 验证语音对话。
- 切回表盘：**长按 Button B(GPIO1)**。
- 二期：日程/待办/笔记（P2）；如需离线唤醒词，把尾部 2.81MB 划成 `assets` / `model`。

---

## 4. 方案 A 详细设计（第二阶段候选）

### 4.1 工程布局

```
firmware-stopwatch-idf/
├── components/
│   └── xiaozhi/                      ← 新增：小智核心，尽量少改上游代码
│       ├── audio/                    ← 原样搬迁（audio_service / codecs / wake_words）
│       ├── protocols/                ← 原样搬迁（websocket / mqtt+udp）
│       ├── mcp_server.{h,cc}         ← 原样搬迁
│       ├── application.{h,cc}        ← 小幅改动（去掉全局 main 初始化）
│       ├── device_state_machine.*    ← 原样搬迁
│       └── board/                    ← 自写板卡适配（见 4.2）
├── main/
│   ├── apps/app_xiaozhi/             ← 新增 Mooncake App（UI + 生命周期）
│   └── ...（现有 App 不动）
└── partitions.csv                    ← 二/三期才可能改（见 4.6）
```

**原则：上游代码尽量零改动**，所有适配集中在 `components/xiaozhi/board/` 与 `apps/app_xiaozhi/`，便于后续同步上游。

### 4.2 板卡适配（`StopWatchBoard`）

小智自定义板卡需要 `config.h` + `<board>.cc` + `config.json`，并覆写三个虚函数：

```cpp
class StopWatchBoard : public WifiBoard {
    AudioCodec* GetAudioCodec() override;   // → 复用现有 ES8311 通路
    Display*    GetDisplay()    override;   // → LVGL 适配层（关键，见 4.3）
    Backlight*  GetBacklight()  override;   // → 桥到 GetHAL().setBackLightBrightness()
};
DECLARE_BOARD(StopWatchBoard);
```

**参考模板**：`boards/waveshare/esp32-s3-touch-amoled-1.8`（CO5300 AMOLED + ES8311，硬件最接近）＋ `boards/m5stack-core-s3`（M5Stack 电源/IOE 习惯）。差异点只有引脚、分辨率、触摸与 PMIC。

好消息：小智自带 `common/press_to_talk_mcp_tool.cc`，**按键触发说话是一等公民**，第一版不需要唤醒词。

### 4.3 必须解决的三个冲突

#### ① LVGL 所有权（最大风险）

- 现状：现有固件的 LVGL 由 `hal_display.cpp` / `hal.cpp` 初始化并驱动（M5GFX + CO5300）。
- 小智：`SpiLcdDisplay : LcdDisplay : Display` 会**自己初始化 LVGL / esp_lvgl_port 并接管屏幕**。
- 冲突：两套 LVGL 初始化 = 崩。

**解法（推荐）**：不复用 `SpiLcdDisplay`，写一个 `StopWatchDisplay : public Display` 适配层：

- 不初始化 LVGL，只在**现有** `lv_screen_active()` 上创建一个全屏不透明容器作为小智 UI 根节点；
- 实现基类的 `SetChatMessage` / `SetEmotion` / `SetStatus` / `ShowNotification` / `SetTheme` 等方法，用现有 `uitk::lvgl_cpp` 组件绘制；
- 进入 `AppXiaozhi` 时隐藏主界面容器，退出时销毁小智根容器并恢复；
- 所有 LVGL 访问沿用现有 `LvglLockGuard`。

预估 300~500 行，是本次融合最主要的自写代码量。

#### ② 音频仲裁（ES8311 + I2S0）

| 维度 | 现有 | 小智 |
| --- | --- | --- |
| 采样率 | 44100Hz（record / play / FFT） | 输入 16kHz（Opus 帧）、输出通常 24kHz |
| 用途 | BLE 麦克风流到 Mac、FFT 频谱、音效 | WebSocket/MQTT 实时语音、TTS 播放 |

**解法**：`components/xiaozhi/board/` 里实现 `StopWatchAudioCodec : public AudioCodec`，内部**复用现有 `hal_audio.cpp` 的 ES8311 实例**，只切换采样率与回调；并加一把互斥锁：

- 规则：**任一时刻只有一个音频消费者**（要么 BLE 麦克风，要么小智会话，要么 FFT/音效）；
- `AudioCodec::Start()/Stop()` 时重新配置 `esp_codec_dev` 采样率，切换后必须 reset I2S 通道；
- 退出小智会话后恢复 44100Hz，供 BLE 麦克风继续使用；
- 小智内部使用 `AudioService` 自带的 Opus 编解码（16kHz / 60ms 帧），不做重采样到 44.1k 的往返。

#### ③ WiFi + BLE 共存与功耗

- 现状实测：日常混合使用约 5 小时；BLE 麦克风在讲话时才高速传输。
- 小智会话：WiFi 长连 + 麦克风常开 + Opus 编码 + LVGL 动画，属于重量级负载，2.4GHz 同时跑 BLE 与 WiFi 会互相抢射频。

**解法**：
- **前台互斥**：`AppXiaozhi` 前台时暂停 BLE 麦克风与 HID 高频事件（Codex 桥的 BLE 连接可保留但降频）；
- **会话超时**：静音 30~60s 自动结束会话回到 idle，停 WiFi 省电（复用 `power_save_timer`）；
- **不去做常驻唤醒词**（v1）：ESP-SR AFE 常驻会把待机功耗拉高一个量级；
- **进入小智前台时 `markActivity()`**，避免被现有 activity monitor 误判休眠息屏；同时把小智会话纳入 `isActivitySleeping()` 的判断，避免息屏时音频被中断。

### 4.4 App 与交互设计（466×466 圆屏）

按仓库 `AGENTS.md` 的既定约束设计（主确认操作靠垂直中心，别放裁切角）：

| 状态 | 屏幕 | 交互 |
| --- | --- | --- |
| Idle | 圆环头像 + 当前状态 + 今日日程提示 | 按 `btnB` 开始说话；滑动返回 Launcher |
| Listening | 呼吸动效 + 实时波形 | 松开/再按结束 |
| Thinking | 转圈（复用现有 loading 动效语言） | 不可打断 |
| Speaking | 圆环随音量脉动 + 转写文本滚动 | 按 `btnB` 打断 |
| Error | 复用现有 `kMicroWarning` 橙色告警风格 | — |

UI 语言与配色沿用现有表盘体系（青 `0x12D6B2` / 蓝 `0x4292F5` / 告警橙），保证和 Codex 表盘、系统状态栏是一套视觉。

### 4.5 资源预算（估算）

| 项 | 现状 | 融合后估算 | 余量 |
| --- | --- | --- | --- |
| App 体积 | 5.55MB | **7.5 ~ 8.5MB** | 9MB 分区，**偏紧** |
| 内部 RAM | — | 小智在 2MB PSRAM 板卡上可跑，我们 8MB PSRAM 充裕；**I2S DMA 缓冲需留在内部 RAM** | 需实测 |
| 内部 Flash | — | model 分区（仅唤醒词时需要）1MB，放尾部未分配区 | 约 2.8MB 可用 |

**瘦身预案（若超 9MB）**：裁剪 `codex_pet_*` 帧资源、只保留必要字号、`-Os`、裁掉不用的 LCD/Touch 驱动依赖、可选关闭 `esp_image_effects`。
**兜底**：若仍超，则扩 `factory` 分区——但那会移动 `storage` 偏移，**用户上传的 badge 图会丢**，属于需要单独确认的操作。

### 4.6 分区表影响

- **v1（无唤醒词）：不需要改分区表。**
- **v3（加 ESP-SR 唤醒词）：只需在尾部新增一个 `model` 分区**
  ```
  model, data, spiffs, 0xD30000, 0x100000,   # 1MB，放 0xD30000（coredump 之后）
  ```
  这**不会移动** `nvs` / `phantom` / `factory` / `storage`，因此配网凭据与 badge 图都不受影响。
- 注意 `AGENTS.md` 明确要求：**不得把「修改分区表」当作 UI 任务的副作用**。上述任何分区改动都必须作为独立决策单独确认后执行。

---

## 5. 日程 / 待办 / 笔记方案

### 5.1 总体架构：语音入口走 MCP，展示通路走 BLE 桥

```
                        ┌─────────────────────────── 语音链路（联网时） ───────────────────────────┐
 用户说话 ──► 手表 mic ──► 小智服务器(云/自建) ──► ASR ──► LLM ──► MCP tools/call ──┐
                        │                                                          ▼
                        │                                        Mac: Agenda MCP Server (FastMCP)
                        │                                          todo.add / agenda.query / note.add
                        │                                                          ▼
                        │                                          SQLite + Markdown 镜像（单一数据源）
                        │                                                          │
                        └──────────────────────────────────────────────────────────┤
                                                                                   ▼
                 ┌──────────────── 展示链路（可选，不依赖云） ────────────────────────┐
                 │  Mac 守护进程监听 DB 变更 ──► 现有 Swift BLE 桥 ──► panel JSON      │
                 │      { "agenda": { "today": [...], "next": {...} } }               │
                 │                          ▼                                        │
                 │ 手表固件解析 agenda block ──► 「今日」界面 / 表盘一行 ──► 到点震动提醒  │
                 └───────────────────────────────────────────────────────────────────┘
```

**为什么展示不走 MCP？**
- MCP 的调用方向是「LLM → 工具」，是**按需触发**，不能作为确定性推送通道；
- 让 LLM 每次主动调 `self.agenda.set_items` 去刷新手表，不确定性太高；
- 现有 BLE 桥 panel 机制**已经过实机验证**（Codex 额度就是这么推的），复用成本最低、手表侧零联网功耗。

### 5.2 Mac 侧：Agenda MCP Server

**数据源（单点真相）**：`~/Library/Application Support/M5StopWatch/agenda.db`（SQLite）

```sql
CREATE TABLE items (
  id         INTEGER PRIMARY KEY,
  type       TEXT NOT NULL,        -- 'todo' | 'event' | 'note'
  title      TEXT NOT NULL,
  body       TEXT,
  due_at     INTEGER,              -- epoch 秒，todo/event 用
  remind_at  INTEGER,              -- 到点提醒
  location   TEXT,
  status     TEXT DEFAULT 'open',  -- open | done | archived
  tags       TEXT,
  source     TEXT,                 -- 'voice' | 'manual' | 'sync'
  ext_id     TEXT,                 -- 外部系统 ID（可空）
  created_at INTEGER, updated_at INTEGER
);
```

**Markdown 镜像**：同步导出到 Obsidian 兼容目录（`~/Obsidian/Inbox/YYYY-MM-DD.md`），保留纯文本可读性与可迁移性。

**MCP 工具清单（建议）**：

| 工具 | 参数 | 说明 |
| --- | --- | --- |
| `todo.add` | `title`, `due?`, `tags?` | 语音"记一下…"入口 |
| `todo.list` | `scope`(today/week/all), `status?` | 朗读今日待办 |
| `todo.complete` | `id` 或 `title` | "这个做完了" |
| `event.add` | `title`, `start`, `end?`, `location?` | 日程 |
| `agenda.query` | `date`(today/tomorrow/YYYY-MM-DD) | 查询某天 |
| `agenda.upcoming` | `hours`（默认 24） | "接下来有什么" |
| `note.add` | `title?`, `body`, `tags?` | 速记 |
| `note.search` | `query` | 检索 |
| `remind.set` | `text`, `at` | 到点提醒（写 `remind_at`） |

**工程约束（来自小智 MCP 实践）**：
- 工具名与参数名语义清晰，docstring 写清"何时该调用"，否则大模型不会调用；
- **返回值控制在 1024 bytes 以内**，避免朗读冗长；
- 返回值用精简 JSON 或短句。

**接入方式（两种，按隐私偏好二选一）**：

| 方式 | 做法 | 优点 | 代价 |
| --- | --- | --- | --- |
| **官方云接入点** | xiaozhi.me 控制台 → 智能体配置角色 → 右下角「MCP 接入点」→ 复制 `wss://api.xiaozhi.me/mcp/?token=…`；Mac 上常驻 `mcp_pipe.py`（官方示例）或 `xiaozhi-client`（Node，可挂现成 MCP Server） | 半天可用，几乎零运维 | 数据经云端；Mac 需常在线；token 需保密 |
| **自托管服务器** | 部署 `xinnan-tech/xiaozhi-esp32-server`（Python）到 Mac mini，MCP 走内网，手表指向自建服务器地址 | 语音与数据全程不出内网，隐私最佳 | 需要搭服务、维护 ASR/LLM/TTS 配置 |

> 建议：**P0 用官方云接入点快速验证，P3 视隐私需要迁到自托管**。两者对固件的要求完全一样（只是服务器地址不同）。

**常驻方式**：写成 launchd LaunchAgent（与本项目现有 Swift 桥的运维方式一致），带保活与日志轮转。

### 5.3 手表侧

**一期（展示为主，改动小）**
- 固件新增解析 panel JSON 中的 `agenda` block（与现有 `codex` block 并列）；
- 新增 `AppAgenda`「今日」界面：今天的事件与待办列表、下一条高亮；
- 到点提醒：优先复用现有 `app_alarm_clock` + RX8130 RTC + 震动马达（本地，不依赖网络）；
- 表盘可选加一行"下一件事"（与现有 `WK xx%` 行同风格，保持信息密度一致）。

**二期（设备侧 MCP 工具，可选）**
- `McpServer::AddTool` 注册设备能力，让 LLM 能直接操作手表：
  - `self.watch.show_agenda`（`date`）——"把手表切到今日"
  - `self.watch.vibrate`（`ms`）——配合提醒
  - `self.todo.add_local`（`title`）——**离线兜底**：断网时先写手表 FATFS，BLE 桥回传后合并进 SQLite
- `AddUserOnlyTool` 放特权操作（重启、亮度、清缓存等），不暴露给模型自主调用。

### 5.4 隐私与安全

- 语音内容必然经过 ASR/LLM/TTS（云端或自建），这一点需要在选型时说清楚；
- 日程/笔记属敏感数据：**默认全部本地存放**（SQLite + Markdown），不引入第三方云文档；如需同步，再单独评估；
- MCP 接入点 token 属凭据：**不得提交进 git**（仓库 `AGENTS.md` / `SECURITY_AND_PRIVACY.md` 已有此约定）；
- 现有 `codex_config.h` 的公开占位符约定继续遵守。

---

## 6. 分期路线图与验收标准

| 阶段 | 内容 | 验收标准 | 影响面 |
| --- | --- | --- | --- |
| **P0**（0.5~1 天） | Mac 侧 Agenda MCP Server + 数据层 + 接入点（官方云）；用手机小智 App 或现成小板验证 | 说"记一下明天十点开会"→ SQLite 出现记录 → 问"明天有什么"能答对 | **不碰手表固件**，零风险 |
| **P1**（3~5 天） | **双固件骨架（方案 B）**：分区重排并全量刷写 → 用 IDF 6.x 独立工程编译 xiaozhi v2.2.6 + 自写 StopWatch 板卡适配 → 两个系统各加"切换系统"入口（写 otadata + 重启） | 手表能在两系统间一键切换；小智侧按 B 能连续对话；切回表盘后 Codex 表盘、BLE 麦克风、配网、省电全部正常（**回归风险接近零**） | 新增独立工程 + 分区表 + 现有固件加一个菜单项 |
| **P2**（2~3 天） | 日程/待办/笔记展示：小智侧走 WiFi 内网直连 Mac HTTP 服务；表盘侧复用 BLE 桥 panel `agenda` block → 「今日」界面 + 到点震动提醒 | 手机关网也能在表盘侧看到今日日程；到点提醒准确 | 复用现有 panel 机制 |
| **P3**（决策点） | 若"两套系统不互通"难以接受 → 用已验证的板卡适配转做**方案 A 融合**；若要唤醒词 → 从 storage 划 1MB 加 `model` 分区 | 融合后单系统共存；或唤醒率/误唤醒达标 | 需单独确认分区表变更 |

**每阶段独立可交付、可回滚**——每期都能单独验收，不需要"全做完才能用"。

---

## 7. 风险清单

| # | 风险 | 等级 | 缓解 |
| --- | --- | --- | --- |
| 1 | IDF 版本错配（用最新小智 → 必须升 IDF 6） | 高 | **锁 v2.2.6**，固定 tag |
| 2 | LVGL 双所有者冲突 | 高 | 自写 `Display` 适配层，不接管 LVGL 初始化 |
| 3 | I2S/ES8311 采样率与占用争抢 | 高 | 音频互斥锁 + 切换时重配 + 退出恢复 44.1k |
| 4 | App 体积超 9MB 分区 | 中 | 瘦身资源/字体；兜底扩分区（需确认，会丢 badge 图） |
| 5 | WiFi+BLE 共存、续航下降 | 中 | 前台互斥、会话超时、不做常驻唤醒词 |
| 6 | 息屏/休眠打断音频 | 中 | 小智会话纳入 activity monitor 判断 + `markActivity()` |
| 7 | 上游代码漂移，后续难同步 | 中 | 上游零改动，适配集中在 board/app 两处 |
| 8 | 依赖 manifest 过大（138 块板卡驱动） | 中 | 裁剪 `idf_component.yml`，只留本机依赖 |
| 9 | 语音+日程数据隐私 | 中 | 本地优先存储；敏感场景走自托管服务器 |
| 10 | 现有 `AGENTS.md` 约束（分区表/HID/VID-PID 不得被副作用修改） | — | 严格按阶段提交，分区表变更单独立项确认 |
| 11 | 分区重排导致 `storage` 被重格式化（badge 图丢失） | 低 | 刷写前备份 `/spiflash/badge/*`，刷完写回；最多 6 张，可重传 |
| 12 | 双固件下的体验割裂（两套 UI、切换需重启、能力不互通） | 中 | 一期先接受；P3 决策点可转方案 A 融合 |

---

## 8. 待确认事项

1. **路线选择（已修订）**：是否接受**方案 B 双固件**作为第一阶段？若接受，需同时确认两个前提：
   (a) 接受"小智系统里没有 Codex 能力（额度/语音输入/HID 均不可用）"；
   (b) 接受分区重排后需重传一次 badge 图（最多 6 张）。
2. **切换入口放哪**：系统设置菜单里加一项，还是用组合键（如 `btnA + btnPwr`）长按触发？
2. **云端还是自托管**：语音走官方 xiaozhi.me，还是自建服务器（隐私优先）？
3. **数据落地**：本地 SQLite + Markdown（推荐），还是接入你现有的待办/日历工具（滴答清单 / 飞书 / 腾讯文档 / Apple 提醒事项）？
4. **唤醒方式**：一期只做按键（按 B 说话，省电、不改分区），还是必须上"小智小智"离线唤醒词（需加 1MB `model` 分区 + 待机功耗上升）？
5. **手表展示**：日程要不要上表盘（与 Codex 双环共存），还是只做一个独立 App 界面？

---

## 附：关键事实速查（供复核）

| 事实 | 出处 |
| --- | --- |
| 小智 v2.2.6 要求 `idf >= 5.5.2`、`lvgl ~9.5.0` | `v2.2.6/main/idf_component.yml` |
| 小智 v2.5.0 要求 IDF ≥ 6.0.1，5.x 不再支持 | `main/README.md` |
| 本项目 IDF 5.5.4 / LVGL 9.5.0 | `esp-idf` 源码、`sdkconfig`、`components/lvgl` |
| 本机音频为 ES8311 单编解码器、44100Hz | `main/hal/hal_audio.cpp` |
| 显示为 CO5300 AMOLED 466×466 | `main/hal/hal_display.cpp` |
| 配网组件与小智同源（`78/esp-wifi-connect`） | `main/main.cpp`、`main/idf_component.yml` |
| 小智板卡需覆写 `GetAudioCodec/GetDisplay/GetBacklight` | `docs/custom-board.md` |
| 小智自带按键说话工具 | `main/boards/common/press_to_talk_mcp_tool.cc` |
| 设备侧 MCP 注册：`McpServer::AddTool` / `AddUserOnlyTool` | `docs/mcp-usage.md` |
| MCP 接入点：`wss://api.xiaozhi.me/mcp/?token=…`，控制台生成 | 官方控制台 / `mcp-calculator` 实践文档 |
| 工具返回值建议 ≤1024 bytes | 官方 MCP 实践建议 |
| 当前固件体积 5.55MB / factory 9MB | `build/StopWatch-UserDemo.bin` |
