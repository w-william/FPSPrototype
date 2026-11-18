# FPSPrototype

基于 Unreal Engine 5.8 的 C++ / 蓝图第一人称射击原型。当前已完成武器弹药、服务器权威射击、GAS 伤害与生命值、HUD 和基础双人网络同步，并通过 Windows Development 打包及核心玩法验证。

项目仍处于原型阶段，尚未形成完整的游戏流程。仓库包含 UE 工程源码、配置与资源，不包含引擎、编译缓存或打包后的可执行程序。

## 已完成的工作

### 自定义战斗与武器系统

- **角色与武器接入**：`BP_Character` 使用 `FPSCombatCharacter`，接入已有移动、第一人称相机、持枪动画及武器附加流程。
- **数据驱动武器**：`WeaponDefinition` / `DA_Rifle` 配置伤害、射速、射程、弹匣容量、备弹、换弹时间和自动/半自动模式；每把武器独立保存弹药状态。
- **服务器权威射击**：客户端请求开火，服务器检查归属、附加关系、存活、换弹、射速和弹药，再执行准星射线与枪口遮挡检测。客户端不直接提交命中目标或伤害数值。
- **GAS 伤害与生命值**：通过 `FPSDamageEffect`、`FPSHealthSet` 处理伤害和生命值范围；生命值归零后停止移动与开火，并取消换弹。
- **换弹逻辑**：服务器计时，完成时才转移弹药；支持部分换弹、备弹不足及中途取消，换弹期间禁止射击。
- **网络同步**：客户端开火/换弹 RPC、相关客户端生命值复制、武器拥有者弹药复制和死亡状态同步。
- **输入与 HUD**：运行时添加 Enhanced Input 开火/换弹映射；HUD 显示生命值、弹药、换弹和死亡状态，服务器确认命中后准星短暂变红。
- **蓝图扩展事件**：提供 `OnShotFired`、`OnHitConfirmed`、`OnHealthChanged`、`OnDied`，供后续接入反馈表现。

默认步枪参数：

| 参数 | 当前默认值 |
| --- | --- |
| 生命值 | 100 |
| 单发伤害 | 25 |
| 弹匣 / 备弹 | 30 / 90 |
| 射速 | 600 RPM |
| 射程 | 10,000 cm |
| 换弹时长 | 2 秒 |
| 射击模式 | 自动；数据资产也支持半自动配置 |

### 瞄准偏移与第一人称视角

- **上下瞄准偏移**：新增 `AO_Rifle_Pitch`，由 `ABP_Character` 根据瞄准俯仰角驱动，范围为 -90°～90°。左右转向启用角色的“使用控制器旋转 Yaw”，角色本体不跟随控制器 Pitch / Roll 倾斜。
- **相机与人物穿模修复**：第一人称相机固定在胶囊体的眼位，不再跟随头骨动画；仅隐藏第一人称头颈，第三人称模型保留完整头颈。
- **持枪视野修复**：`ABP_FP` 使用相机空间的右手目标和双手 FABRIK，让武器在抬头、低头和转向时保持在视野右下方，同时维持左手握把位置。第一人称 FOV 为 90。
- **可调持枪位置**：`ABP_FP` 的 `FirstPersonView` 分类提供 `ViewGripOffset`，用于调整第一人称武器的位置与姿态。

### UE 模板与现有资源

工程保留 FirstPerson、Horror、Shooter 模板地图及其输入、角色、武器、UI、AI / StateTree 等配套代码和资源。这些模板内容与自定义 `Combat` / `Weapons` 系统分开保留，不代表已经完成了统一的恐怖或竞技游戏模式。

主要自定义关卡为 `/Game/0_/Maps/MainMap`；其 GameMode 为 `/Game/0_/Cores/BP_GameMode`。

## 环境与启动

- Unreal Engine **5.8**；当前验证版本为 **5.8.4**。
- Windows 64 位、支持 DX12 的显卡及驱动。
- Visual Studio 2022，安装“使用 C++ 的游戏开发”、MSVC 和 Windows SDK。
- Git。仓库直接包含 `.uasset` / `.umap` 二进制资源，正常克隆即可获得完整工程。
- 使用 UE 自带的 GameplayAbilities、EnhancedInput、StateTree / GameplayStateTree 等模块；项目插件配置见 `FPSPrototype.uproject`。
- 编辑器自动化已启用 UE 5.8 自带的实验性 `ModelContextProtocol` 和 `AllToolsets` 插件；引擎插件本体不随项目上传。

```powershell
git clone https://github.com/w-william/FPSPrototype.git
cd FPSPrototype
```

右键 `FPSPrototype.uproject` 生成 Visual Studio 项目文件，以 **Development Editor / Win64** 编译 `FPSPrototypeEditor`，然后打开工程。首次打开会生成缓存并编译着色器。

也可以将 `UE_ROOT` 环境变量设为本机 UE 5.8 安装目录，在仓库根目录使用 PowerShell 构建：

```powershell
$Project = (Resolve-Path '.\FPSPrototype.uproject').Path
& "$env:UE_ROOT\Engine\Build\BatchFiles\Build.bat" `
  FPSPrototypeEditor Win64 Development "-Project=$Project" `
  -WaitMutex -NoHotReloadFromIDE -MaxParallelActions=1
```

当前 Target 已采用 `BuildSettingsVersion.V7`，包含顺序保留 `Unreal5_7` 以保持已有代码兼容。

## 运行与操作

打开 `Content/0_/Maps/MainMap` 后运行 PIE。

| 操作 | 输入 |
| --- | --- |
| 移动 | W / A / S / D |
| 视角 | 鼠标 |
| 开火 | 鼠标左键；按住连发、松开停止 |
| 换弹 | R |

双人验证时，在编辑器 Play 设置中选择 **Listen Server**、**2 个玩家**，可启用 **Run Under One Process**。这是编辑器中的本地联机验证方式，当前未实现匹配、房间列表、账号或互联网服务接入。

## 已完成的验证

### 瞄准与第一人称视角修复验证

**2026-11-18、UE 5.8.4**：角色和动画蓝图编译通过；双客户端验证覆盖平视、抬头 80°、低头 70°及左右转向，连续转向采样 70 帧。相机眼位保持稳定，第一人称头颈不进入镜头，武器保持在视野右下方；动态右手目标误差小于 0.03 cm，左手握把误差小于 0.07 cm。

实际客户端开火与换弹通过：`30/90 → 26/90 → RELOADING → 30/86`。本轮修复完成了蓝图编译及 PIE 运行验证，尚未对这些新资产重新执行 Windows 打包验证。上述视角验证使用仓库外的临时编辑器脚本，不包含在仓库内的 7 项 C++ 自动测试中。

### 此前 UE 5.8 迁移与核心玩法验证

以下为 **2026-11-18、UE 5.8.4** 在本轮瞄准与视角修复前完成的本机验证结果：

| 验证项目 | 结果 |
| --- | --- |
| Editor C++ 构建 | 通过 |
| 仓库内 7 项武器 / 战斗自动测试 | 全部通过 |
| 5 个核心蓝图编译及武器、HUD、动画配置检查 | 通过 |
| 双人 Listen Server PIE 真实网络测试 | 通过：开火 RPC、GAS 扣血、血量/弹药同步、换弹、死亡后开火拦截 |
| Windows Development 构建、Cook、Stage、Pak / IoStore、Archive | 通过；Cook 0 错误、0 警告 |
| 打包游戏 DX12 操作验证 | 开火、换弹、移动及正常退出通过 |
| 打包根目录启动程序 | 启动及退出通过 |

实际打包游戏的 HUD 检查得到：`30/90 → 20/90 → RELOADING → 30/80`，生命值显示 `100/100`；移动后场景位置变化，退出码为 `0`。

仓库内测试位于 `Source/FPSPrototype/Private/Tests`。在编辑器的自动化测试界面选择 `FPSPrototype`，或在 Output Log 控制台执行：

```text
Automation RunTests FPSPrototype
```

这 7 项测试覆盖伤害与射速、遮挡/未命中/射程、权限/归属/死亡、自动/半自动/换弹、弹药守恒、部分换弹与实例独立性，以及非权威端不能修改弹药。

双人网络自动测试使用了**仓库外的临时编辑器测试宿主**，运行时代码与当前工程一致。它的通过结果是本次验证记录，不属于当前仓库内可直接运行的第 8 项测试。

上述检查属于兼容性与核心玩法验证，不代表已完成全功能、音频、性能、压力测试或 Shipping 发布验收。

## Windows 打包

本次打包包含以下 4 张地图：

- `/Game/0_/Maps/MainMap`
- `/Game/FirstPerson/Lvl_FirstPerson`
- `/Game/Variant_Horror/Lvl_Horror`
- `/Game/Variant_Shooter/Lvl_Shooter`

设置好 `UE_ROOT` 后，在仓库根目录运行：

```powershell
$Project = (Resolve-Path '.\FPSPrototype.uproject').Path
$Archive = Join-Path (Get-Location).Path 'Saved\Packages'
& "$env:UE_ROOT\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun `
  "-project=$Project" -noP4 -unattended -platform=Win64 `
  -clientconfig=Development -build -cook -stage -pak -iostore -archive `
  '-MapsToCook=/Game/0_/Maps/MainMap+/Game/FirstPerson/Lvl_FirstPerson+/Game/Variant_Horror/Lvl_Horror+/Game/Variant_Shooter/Lvl_Shooter' `
  '-ubtargs=-MaxParallelActions=1' '-AdditionalCookerOptions=-corelimit=4' `
  "-archivedirectory=$Archive"
```

打包产物放在本地 `Saved/Packages`，不会提交到仓库。

## 目录

| 路径 | 内容 |
| --- | --- |
| `FPSPrototype.uproject` | UE 工程及插件配置 |
| `Config/` | 工程、输入和默认地图等配置 |
| `Content/0_/` | 当前主关卡、自定义角色、动画、武器和数据资产 |
| `Content/FirstPerson/`、`Content/Variant_*/` | 保留的 UE 模板内容 |
| `Source/FPSPrototype/Public/Combat/`、`Private/Combat/` | 自定义战斗、GAS 和 HUD |
| `Source/FPSPrototype/Public/Weapons/`、`Private/Weapons/` | 自定义武器与弹药 |
| `Source/FPSPrototype/Private/Tests/` | 仓库内 7 项自动测试 |
| `Source/FPSPrototype/Variant_*/` | 保留的模板变体代码 |
| `Docs/CombatSetup.txt` | 战斗接线与运行链路说明 |

## 当前限制与待完成内容

- 当前使用即时射线和基础命中体，没有爆头倍率或移动射击的历史回溯命中补偿。
- 自定义战斗系统尚未接入完整枪口特效、音效、开火/换弹蒙太奇和复活流程；已有临时弹道线、命中标记与蓝图事件。
- 尚未完成完整比赛流程、菜单、持久化或正式在线服务。
- **CSV 性能采样模式的退出异常尚未解决**：两次启用 CSV 的运行返回 `777003`。相同游戏包在普通运行以及不启用 CSV 的完整操作测试中均正常退出，当前通过结论基于后两种路径。
- 验证时出现非阻塞的 StateTree API 弃用提示及 GameplayCue 搜索路径提示；后续引擎升级和性能整理时需要处理。
- 不建议在 UE 5.8 的编辑器自动化测试中使用 `-NoShaderCompile`；本次该启动方式在测试开始前触发引擎内部空指针，允许正常着色器初始化后测试通过。
