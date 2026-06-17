# Qt 与 STL Bridge 优化建议

## 1. 文档目的

本文档针对当前 `mlcp-hmi/bridge` 中 Qt 与 C++17/STL 业务层之间的适配代码，
从扩展性、可读性、可测试性、线程边界、错误表达和医疗设备 HMI 可追踪性角度
给出完整优化建议。

当前 bridge 代码总体方向是正确的：Qt 类型主要停留在 `bridge` 与 `ui` 层，
`service`、`system` 和设备服务仍以 STL 类型为主，未出现严重的 Qt 类型下沉问题。
因此建议采用渐进式优化，而不是一次性大重构。

## 2. 当前现状

当前 bridge 主要包含两个 ViewModel：

- `LifecycleViewModel`
  - 通过 `Q_PROPERTY(QString stateText)` 暴露系统生命周期状态。
  - 内部使用 `QTimer` 每 200 ms 调用 `AppControl::lifecycleStateName()` 轮询状态。
  - 负责 `std::string` 到 `QString` 的转换与大写显示。

- `FanSettingsViewModel`
  - 通过多个 `Q_PROPERTY` 暴露风扇/温控配置与运行快照。
  - 通过 `refresh()` 从 `AppControl::tempServiceSnapshot()` 拉取数据。
  - 通过 `applySettings()` 调用 `AppControl` 修改温控配置。
  - 负责 `std::optional<double>` 到显示文本的转换。
  - 所有属性共用一个 `settingsChanged()` 信号。

当前调用链大致为：

```text
QML -> bridge/ViewModel -> system/AppControl -> service/device/persistence
service/device/persistence -> system/AppControl -> bridge/ViewModel -> QML
```

## 3. 总体评价

当前实现适合演示阶段，优点明显：

- bridge 层较薄，没有直接访问设备、数据库或 Linux API。
- Qt 类型没有污染 `service/temp`、`service/lifecycle` 等业务服务。
- ViewModel 暴露给 QML 的属性直观，页面绑定成本低。
- `AppControl` 已经把 runtime/service 线程访问包在统一入口里，bridge 暂时不需要理解太多线程细节。

但如果项目继续扩展到自检报告、报警、历史趋势、配置变更、审计记录和故障恢复，
当前 bridge 会逐步暴露以下问题：

- ViewModel 直接依赖完整 `AppControl`，测试和替换成本会升高。
- 每个 ViewModel 都手写一套 STL 到 Qt 的字段搬运和格式化逻辑。
- 信号粒度偏粗，属性变更不可精确表达。
- 状态刷新依赖轮询，后续多服务、多页面时会增加无效调用。
- 显示文本和领域值混在一起，不利于单位、语言、报警色和无效值策略统一管理。
- 错误主要以字符串进入 UI，缺少稳定错误码和可断言错误模型。

## 4. 优化目标

优化目标不是把 bridge 做厚，而是让 bridge 成为稳定、可测、可扩展的边界：

```text
ui 只处理展示与交互
bridge 只处理 Qt 适配、UI 状态、线程投递、命令转发
service/core 处理业务规则、状态机、安全策略和错误模型
system/appcontrol 处理装配、运行时线程和应用级入口
```

建议达成以下目标：

1. Qt 类型只出现在 `app`、`ui`、`bridge`。
2. bridge 不直接依赖大型具体编排对象，而依赖面向 UI 的窄接口。
3. STL 领域对象到 Qt UI 状态的转换集中管理。
4. UI 可观察状态以 snapshot/state 对象表达，避免散落字段搬运。
5. 业务错误保留错误码、错误来源和用户可读文案。
6. 状态变化优先通过事件或回调推送，低优先级数据才轮询。
7. ViewModel 可以用 fake backend 单元测试。

## 5. 建议一：抽出面向 HMI 的窄接口

### 5.1 当前问题

`LifecycleViewModel` 和 `FanSettingsViewModel` 直接依赖 `AppControl`。
这让 bridge 知道了系统总控对象，也让单元测试必须构造较重的应用环境。

`AppControl` 的职责包括启动、停止、设备初始化、服务装配、自检、runtime 调度和
对 UI 暴露接口。bridge 只需要其中一小部分能力，直接依赖完整类会放大耦合。

### 5.2 建议方案

新增面向 bridge 的纯 C++ 接口，例如：

```cpp
class IHmiBackend {
public:
    virtual ~IHmiBackend() = default;

    virtual std::string lifecycleStateName() const = 0;
    virtual mlcp::hmi::service::temp::TempServiceSnapshot tempServiceSnapshot() = 0;
    virtual mlcp::hmi::service::temp::TempServiceConfig tempServiceConfig() = 0;
    virtual void applyTempServiceConfig(
        const mlcp::hmi::service::temp::TempServiceConfig& config) = 0;
};
```

也可以按业务拆成更小接口：

- `ILifecycleUiPort`
- `ITempServiceUiPort`
- `ISelfTestUiPort`
- `IAlarmUiPort`
- `IHistoryUiPort`

### 5.3 推荐落地方式

短期可以让 `AppControl` 实现这些接口，ViewModel 构造函数只依赖接口引用：

```text
bridge/ViewModel -> IHmiBackend
system/AppControl -> IHmiBackend
```

这样不改变现有业务路径，却能明显降低 bridge 的测试成本。

### 5.4 收益

- ViewModel 单元测试可以直接使用 fake backend。
- bridge 不再依赖完整应用装配对象。
- 后续替换 demo backend、模拟器 backend、真实硬件 backend 更容易。
- UI 能力边界更清楚，避免页面绕过用例层访问系统内部。

## 6. 建议二：引入 UI State 与 Mapper

### 6.1 当前问题

`FanSettingsViewModel::refresh()` 同时做了：

- 调用后台接口。
- 捕获异常。
- 拆解 `TempServiceSnapshot`。
- 设置多个成员变量。
- 格式化温度文本。
- 设置 UI 状态文本。
- 发出 Qt 信号。

该函数现在还不长，但后续字段增加后会变成典型的字段搬运函数。

### 6.2 建议方案

为每类页面或功能建立纯 C++ UI state：

```cpp
struct FanSettingsUiState {
    bool available {false};
    bool temperatureAutoControl {true};
    double targetCelsius {37.0};
    double toleranceCelsius {0.5};
    int manualFanDuty {0};
    int minFanDuty {0};
    int maxFanDuty {255};
    std::optional<double> latestTemperatureCelsius;
    int latestFanDuty {0};
    std::string operationStatus;
    std::string errorMessage;
};
```

再提供 mapper：

```cpp
FanSettingsUiState toFanSettingsUiState(
    const mlcp::hmi::service::temp::TempServiceSnapshot& snapshot);
```

如果 mapper 需要 Qt 文本，则放在 bridge 内部：

```cpp
QString formatTemperatureText(const std::optional<double>& value);
QString formatFanStatusText(const FanSettingsUiState& state);
```

### 6.3 放置建议

可以按职责拆分：

```text
mlcp-hmi/bridge/fanSettingsViewModel.h
mlcp-hmi/bridge/fanSettingsViewModel.cpp
mlcp-hmi/bridge/fanSettingsUiState.h
mlcp-hmi/bridge/fanSettingsMapper.h
mlcp-hmi/bridge/fanSettingsMapper.cpp
```

如果 state 只服务一个 ViewModel，也可以先作为 `.cpp` 内部匿名命名空间结构，
等第二个页面复用时再独立成文件。

### 6.4 收益

- ViewModel 只负责 Qt 属性缓存和信号，不承载转换细节。
- mapper 可以单独测试，不需要启动 Qt/QML。
- 后续增加字段、单位、错误状态时修改集中。
- 代码审查时更容易区分“业务数据变化”和“UI 表示变化”。

## 7. 建议三：显示文本与领域值分离

### 7.1 当前问题

`latestTemperatureText` 当前直接保存类似 `"37.0 C"` 的显示文本。
这对简单页面很方便，但后续会遇到问题：

- 温度单位可能需要统一管理。
- 多语言翻译不应散落在 C++ 字符串拼接中。
- QML 可能同时需要数值、单位、颜色和无效状态。
- 趋势图需要原始数值，而不是格式化文本。
- 医疗设备界面中无效值、超时值、传感器故障值需要明确区分。

### 7.2 建议方案

ViewModel 同时暴露结构化值和必要的便捷文本：

```cpp
Q_PROPERTY(bool hasLatestTemperature READ hasLatestTemperature NOTIFY latestTemperatureChanged)
Q_PROPERTY(double latestTemperatureCelsius READ latestTemperatureCelsius NOTIFY latestTemperatureChanged)
Q_PROPERTY(QString latestTemperatureText READ latestTemperatureText NOTIFY latestTemperatureChanged)
```

QML 用于趋势图、告警判断、颜色判断时优先使用结构化值。
文本仅作为展示便利属性。

### 7.3 收益

- 数据用途更清晰。
- 避免 UI 从字符串里反向解析数值。
- 支持趋势、报警、单位切换和本地化。
- 便于测试边界值，如无温度、NaN、超范围、传感器超时。

## 8. 建议四：拆分属性变更信号

### 8.1 当前问题

`FanSettingsViewModel` 中所有属性共用 `settingsChanged()`。
这会带来几个问题：

- 任意一个字段变化都会触发所有绑定重算。
- QML 难以知道具体哪个属性变化。
- 后续添加动画、提示、局部刷新时控制粒度不足。
- 测试无法精确断言某个字段是否真的变化。

### 8.2 建议方案

为高频或独立语义的属性拆分信号：

```cpp
signals:
    void availabilityChanged();
    void configChanged();
    void latestReadingChanged();
    void statusTextChanged();
```

也可以进一步细分为：

```cpp
void availableChanged();
void temperatureAutoControlChanged();
void targetCelsiusChanged();
void fanDutyChanged();
void latestTemperatureChanged();
void operationStatusChanged();
```

推荐先采用中等粒度，避免信号过多：

- `availabilityChanged`
- `configChanged`
- `runtimeSnapshotChanged`
- `operationStatusChanged`

### 8.3 实现建议

引入状态比较，只在值确实变化时发信号：

```cpp
if (nextState.latestFanDuty != state_.latestFanDuty ||
    nextState.latestTemperatureCelsius != state_.latestTemperatureCelsius) {
    state_.latestFanDuty = nextState.latestFanDuty;
    state_.latestTemperatureCelsius = nextState.latestTemperatureCelsius;
    emit runtimeSnapshotChanged();
}
```

### 8.4 收益

- 降低 QML 无效刷新。
- 变更语义更清楚。
- 后续页面复杂化时更容易维护。
- 测试可以精确覆盖属性变更行为。

## 9. 建议五：从轮询逐步转向事件推送

### 9.1 当前问题

`LifecycleViewModel` 使用 200 ms 定时器轮询生命周期状态。
对单个状态文本影响不大，但后续如果报警、传感器、趋势、服务健康状态都轮询，
会导致 UI 层出现多个定时器和大量重复跨线程调用。

### 9.2 建议方案

优先为低频且重要的状态变化建立事件或回调机制：

- 生命周期状态变化。
- 报警产生、恢复、确认。
- 自检开始、单项结果变化、自检完成。
- 配置变更完成。
- 服务可用性变化。

数据采样类信息可以保留低频轮询或统一调度：

- 温度最新值。
- 风扇 duty。
- 系统 CPU、内存、磁盘。
- 历史趋势增量。

### 9.3 Qt 线程投递边界

业务线程不能直接更新 QObject 成员。
从 service/runtime 线程进入 ViewModel 时，应通过 Qt 队列投递到 UI 线程：

```cpp
QMetaObject::invokeMethod(
    this,
    [this, snapshot]() {
        applySnapshot(snapshot);
    },
    Qt::QueuedConnection);
```

### 9.4 推荐过渡策略

第一阶段不移除所有轮询，只先做两件事：

1. 将生命周期状态改为由 `AppControl` 暴露订阅接口或状态变更回调。
2. 建立一个 bridge 侧统一 refresh timer，避免每个 ViewModel 自己起定时器。

### 9.5 收益

- 减少无效刷新。
- 状态变化响应更及时。
- 线程边界更明确。
- 更适合后续报警、自检、诊断事件接入。

## 10. 建议六：统一错误模型，不只向 UI 暴露字符串

### 10.1 当前问题

`FanSettingsViewModel` 捕获异常后把 `error.what()` 直接写入 `statusText`。
这种方式简单，但不适合医疗设备 HMI 的长期维护：

- UI 无法区分配置非法、服务不可用、持久化失败、设备异常。
- 测试只能断言字符串，稳定性差。
- 本地化困难。
- 审计和故障码难以关联。
- 错误严重级别不明确。

### 10.2 建议方案

引入 bridge 可识别的错误结构：

```cpp
enum class UiErrorCode {
    none,
    tempServiceUnavailable,
    invalidTempTarget,
    invalidFanDuty,
    configSaveFailed,
    backendFailure,
};

struct UiError {
    UiErrorCode code {UiErrorCode::none};
    std::string technicalMessage;
    std::string userMessage;
};
```

ViewModel 暴露：

```cpp
Q_PROPERTY(int errorCode READ errorCode NOTIFY operationStatusChanged)
Q_PROPERTY(QString errorText READ errorText NOTIFY operationStatusChanged)
Q_PROPERTY(bool hasError READ hasError NOTIFY operationStatusChanged)
```

### 10.3 与业务层关系

更理想的方向是 service/system 接口逐步从异常转向 `Result<T, Error>` 风格。
bridge 可以把业务错误码映射为 UI 错误码和用户文案。

异常仍可作为应用边界兜底，但不建议作为普通业务失败路径的主要表达方式。

### 10.4 收益

- QML 可以根据错误码选择颜色、图标、提示策略。
- 测试可以断言错误码。
- 便于本地化。
- 便于和报警、审计、故障记录关联。

## 11. 建议七：命令入参结构化

### 11.1 当前问题

`applySettings(bool, double, int, int)` 参数数量已经接近上限。
后续如果加入 `minFanDuty`、`toleranceCelsius`、`controlInterval`、权限上下文、
操作者信息或修改原因，函数签名会继续膨胀。

### 11.2 建议方案

引入命令结构体：

```cpp
struct FanSettingsCommand {
    bool temperatureAutoControl {true};
    double targetCelsius {37.0};
    int manualFanDuty {0};
    int maxFanDuty {255};
};
```

由于 QML 直接传 C++ struct 成本较高，ViewModel 对 QML 可以保留简单 invokable，
内部立即转换为 command：

```cpp
Q_INVOKABLE bool applySettings(bool temperatureAutoControl,
                               double targetCelsius,
                               int manualFanDuty,
                               int maxFanDuty);
```

内部实现：

```cpp
FanSettingsCommand command {
    temperatureAutoControl,
    targetCelsius,
    manualFanDuty,
    maxFanDuty,
};
return applyFanSettingsCommand(command);
```

### 11.3 收益

- 内部逻辑更易扩展。
- 校验、审计和日志可以围绕 command 组织。
- 后续接权限、确认弹窗、配置变更记录更自然。

## 12. 建议八：输入校验分层

### 12.1 当前问题

QML 当前做了基础字符串解析，bridge 做了 `manualFanDuty` clamp，
service 层再做配置校验。这种分层还不够明确。

### 12.2 推荐分层

```text
QML:
  做输入格式提示，例如空值、非数字、明显非法字符。

bridge:
  做 UI 命令规整，例如字符串/数值转换、空输入 fallback、生成 command。
  不复制核心安全规则。

service/core:
  做权威业务校验，例如目标温度范围、风扇 duty 范围、安全阈值、权限、审计。
```

### 12.3 建议

- 不建议 bridge 静默 clamp 关键安全参数。
- 对手动风扇 duty 这种可恢复输入，可以提示“已按允许范围修正”。
- 对目标温度、安全阈值等参数，应返回明确错误，不应静默修正。
- 校验失败应保留错误码。

## 13. 建议九：保存状态与加载状态分离

### 13.1 当前问题

`applySettings()` 设置 `statusText_ = "Saved"` 后马上调用 `refresh()`，
而 `refresh()` 又会将 `statusText_` 改成 `"Loaded"`。
这会导致“保存成功”提示容易被覆盖。

### 13.2 建议方案

拆分状态：

- `loadStatus`
- `saveStatus`
- `operationMessage`
- `lastUpdatedAt`

或者至少区分：

```cpp
Q_PROPERTY(QString dataStatusText READ dataStatusText NOTIFY dataStatusChanged)
Q_PROPERTY(QString operationStatusText READ operationStatusText NOTIFY operationStatusChanged)
```

### 13.3 收益

- UI 能同时显示“当前数据已刷新”和“上次保存成功”。
- 操作反馈不会被周期刷新覆盖。
- 后续审计、配置变更记录更容易接入。

## 14. 建议十：生命周期状态暴露结构化信息

### 14.1 当前问题

`LifecycleViewModel` 只暴露 `stateText`。
主屏目前够用，但后续医疗设备需要更多状态信息：

- 当前系统状态枚举。
- 状态显示名称。
- 状态颜色或 UI 严重级别。
- 是否允许启动运行。
- 是否允许自检。
- 是否处于报警或降级。
- 组件状态，如风扇、温度传感器、温控服务、自检、运行时。

### 14.2 建议方案

底层已有 `LifecycleSnapshot`，bridge 可以逐步暴露：

```cpp
Q_PROPERTY(int stateCode READ stateCode NOTIFY lifecycleChanged)
Q_PROPERTY(QString stateText READ stateText NOTIFY lifecycleChanged)
Q_PROPERTY(QString severityText READ severityText NOTIFY lifecycleChanged)
Q_PROPERTY(bool alarmActive READ alarmActive NOTIFY lifecycleChanged)
Q_PROPERTY(bool degraded READ degraded NOTIFY lifecycleChanged)
```

组件状态可以用后续的 `QAbstractListModel` 暴露，而不是多个硬编码属性。

### 14.3 收益

- 主屏、维护页、诊断页可以复用同一生命周期数据。
- UI 不需要靠字符串判断状态。
- 后续接报警颜色和操作使能更安全。

## 15. 建议十一：列表型数据使用 QAbstractListModel

### 15.1 适用场景

以下数据不建议长期用多个 `Q_PROPERTY` 或 QML 本地 `ListModel` 模拟：

- 自检报告条目。
- 报警历史。
- 故障记录。
- 趋势数据。
- 配置变更记录。
- 组件健康状态列表。

### 15.2 建议方案

bridge 中为列表数据提供 `QAbstractListModel`：

```text
SelfTestReportModel
AlarmHistoryModel
FaultRecordModel
TrendPointModel
LifecycleComponentModel
```

每个 model 负责：

- 定义 role。
- 从 STL vector 或 repository 查询结果转换为 Qt model。
- 处理增量刷新或整体 reset。
- 保持 UI 线程更新。

### 15.3 收益

- QML 页面绑定更自然。
- 数据刷新更可控。
- 大列表性能更好。
- 便于分页、筛选和排序。

## 16. 建议十二：建立 Bridge 层文件组织规则

### 16.1 当前风险

如果所有 ViewModel 都直接放在 `bridge` 根目录，后续文件会快速增多。

### 16.2 推荐目录

可以在功能变多后调整为：

```text
mlcp-hmi/bridge
├── common
│   ├── uiError.h
│   ├── qtStringFormat.h
│   └── propertyChange.h
├── lifecycle
│   ├── lifecycleViewModel.h
│   ├── lifecycleViewModel.cpp
│   ├── lifecycleMapper.h
│   └── lifecycleMapper.cpp
├── temp
│   ├── fanSettingsViewModel.h
│   ├── fanSettingsViewModel.cpp
│   ├── fanSettingsUiState.h
│   ├── fanSettingsMapper.h
│   └── fanSettingsMapper.cpp
├── selftest
├── alarm
└── history
```

注意：如果真实调整目录结构，需要同步更新 `rule/arch.md`，并更新 CMake。
在功能还少时，不建议为了目录漂亮而提前拆太细。

## 17. 建议十三：减少 QML 与 ViewModel 的状态双写

### 17.1 当前问题

QML 中 `loadFanSettings()` 会从 ViewModel 读取配置，再写入多个输入控件。
这会形成两份状态：

- ViewModel 中的真实配置快照。
- QML 输入框中的编辑中草稿。

这种模式可用，但需要明确区分“已保存状态”和“编辑草稿状态”。

### 17.2 建议方案

保留 QML 草稿，但把语义命名清楚：

- `currentConfig`：后端已保存配置。
- `draftConfig`：UI 正在编辑但未提交的配置。
- `dirty`：草稿是否与当前配置不同。
- `saving`：是否正在保存。
- `lastSaveResult`：上次保存结果。

如果后续页面复杂，可以让 ViewModel 管理 draft：

```cpp
Q_INVOKABLE void resetDraftFromCurrent();
Q_INVOKABLE bool applyDraft();
Q_PROPERTY(bool dirty READ dirty NOTIFY draftChanged)
```

### 17.3 收益

- 避免刷新覆盖用户正在编辑的值。
- 可支持“取消”“恢复默认”“应用但不退出”等操作。
- 更符合医疗设备配置修改需要确认和审计的场景。

## 18. 建议十四：明确同步调用与异步调用边界

### 18.1 当前问题

`AppControl::tempServiceSnapshot()` 使用 `runServiceTaskAndWait()` 同步等待 service 线程。
从 UI 线程调用同步等待，短期简单，但如果后台任务阻塞，会影响界面响应。

### 18.2 建议方案

按操作类型区分：

- 快速读取快照：可以同步，但需要超时保护或保证不会阻塞。
- 配置保存：建议异步，UI 显示 saving 状态。
- 自检请求：必须异步，UI 显示进度。
- 历史查询：建议异步或分页。
- 报警确认：可以同步提交命令，但结果回调更新。

### 18.3 推荐接口形态

短期：

```cpp
Q_INVOKABLE void refresh();
Q_INVOKABLE void applySettingsAsync(...);
```

中期：

```text
bridge command -> backend/service queue -> completion callback -> queued UI update
```

### 18.4 收益

- 避免 UI 卡顿。
- 更适合真实设备 I/O 和数据库写入。
- 操作中、成功、失败状态更清楚。

## 19. 建议十五：把 bridge 纳入测试

### 19.1 当前缺口

当前测试主要覆盖 service、runtime、persistence 等层。
bridge 层如果继续扩展，应补充测试，尤其是 mapper、错误映射和状态变更信号。

### 19.2 推荐测试类型

1. mapper 纯 C++ 单元测试
   - snapshot 正常值。
   - 温度为空。
   - 服务 degraded。
   - lastError 非空。
   - duty 边界值。

2. ViewModel fake backend 测试
   - refresh 成功后属性更新。
   - backend 抛异常或返回错误后 available/error 状态正确。
   - applySettings 成功后操作状态正确。
   - applySettings 失败后不覆盖当前配置。
   - 属性未变化时不发多余信号。

3. Qt 信号测试
   - 使用 `QSignalSpy` 断言信号次数。
   - 验证 queued update 在 UI 线程执行。

4. QML 集成测试
   - 页面初始化加载。
   - 输入非法值提示。
   - 保存成功/失败显示。
   - 页面切换时不覆盖编辑草稿。

### 19.3 收益

- bridge 改动可以被快速验证。
- 复杂页面不只靠手工点击测试。
- 错误文案和错误码映射更稳定。

## 20. 建议十六：为医疗设备场景补充可追踪性

### 20.1 配置变更

温控目标、风扇 duty、报警阈值等都属于可能影响安全的配置。
bridge 发起修改时，命令应保留：

- 修改前值。
- 修改后值。
- 操作者或会话信息。
- 修改来源页面。
- 修改时间。
- 成功或失败原因。

这些信息不一定都由 bridge 生成，但 bridge 应传递必要上下文。

### 20.2 报警与故障

报警展示不应只依赖字符串。
建议所有报警相关 UI 状态都至少包含：

- 报警码。
- 报警级别。
- 当前状态：active、latched、acknowledged、recovered。
- 发生时间。
- 恢复时间。
- 用户可读说明。
- 推荐处理动作。

### 20.3 自检报告

自检报告建议通过 model 暴露条目：

- 检查项 ID。
- 名称。
- 状态。
- 耗时。
- 错误码。
- 详情。

## 21. 分阶段落地路线

### 第一阶段：低风险整理

目标：不改变 UI 行为，先改善可读性和测试入口。

- 新增 HMI backend 窄接口，让 ViewModel 依赖接口而不是完整 `AppControl`。
- 为风扇设置抽出 `FanSettingsUiState` 和 mapper。
- 将温度格式化函数集中到 bridge 内部函数。
- `refresh()` 中只在状态变化时发信号。
- 修复保存成功提示被 `Loaded` 覆盖的问题。
- 给 mapper 增加单元测试。

### 第二阶段：状态语义增强

目标：让 UI 不再依赖字符串判断状态。

- 增加结构化温度属性：`hasLatestTemperature`、`latestTemperatureCelsius`。
- 生命周期增加 `stateCode`、`alarmActive`、`degraded` 等结构化属性。
- 错误状态增加 `errorCode`、`hasError`、`errorText`。
- 配置修改引入内部 command 结构。
- 拆分 `settingsChanged` 为中等粒度信号。

### 第三阶段：事件与异步化

目标：减少 UI 线程同步等待和无效轮询。

- 生命周期状态改为事件推送。
- 自检和配置保存改为异步命令。
- 增加 bridge 侧统一 refresh scheduler。
- 使用 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` 统一跨线程 UI 更新。
- 引入 operation 状态：idle、loading、saving、success、failed。

### 第四阶段：扩展到完整 HMI 数据模型

目标：支撑报警、自检、历史趋势和配置变更页面。

- 增加 `SelfTestReportModel`。
- 增加 `AlarmHistoryModel`。
- 增加 `LifecycleComponentModel`。
- 增加趋势数据 model。
- 接入审计与故障记录查询。
- 补充 QML 集成测试和关键操作测试。

## 22. 推荐优先级

建议优先级如下：

| 优先级 | 建议 | 原因 |
| --- | --- | --- |
| P0 | 抽 HMI backend 窄接口 | 降低耦合，提升可测试性 |
| P0 | 抽 UI state/mapper | 避免字段搬运扩散 |
| P1 | 保存状态与加载状态分离 | 修复现有反馈语义问题 |
| P1 | 结构化值与显示文本分离 | 支撑趋势、单位、本地化 |
| P1 | 错误码化 | 支撑测试、报警、审计 |
| P2 | 拆分信号粒度 | 提升可读性与刷新效率 |
| P2 | 生命周期事件推送 | 减少轮询，提升响应 |
| P3 | QAbstractListModel 化 | 支撑自检、报警、历史列表 |
| P3 | 全面异步化 | 支撑真实硬件与数据库耗时操作 |

## 23. 不建议立即做的事

以下修改暂不建议第一阶段就做：

- 不建议把所有 ViewModel 一次性重写。
- 不建议为两个 ViewModel 立刻拆出很深目录。
- 不建议把所有同步接口马上改成异步，容易扩大影响面。
- 不建议在 bridge 复制 service/core 的完整业务校验规则。
- 不建议让 QML 直接操作 service、device、persistence。
- 不建议把 `QString`、`QObject`、`QTimer` 引入 `service` 或 `core`。

## 24. 建议的目标形态

长期较理想的 bridge 结构如下：

```text
QML
  |
  v
ViewModel / QAbstractListModel
  |
  v
UiState + Mapper + UiError
  |
  v
HMI Port Interface
  |
  v
AppControl / Application Service
  |
  v
service / core / device / persistence
```

对应原则：

- QML 只绑定属性、发出用户意图。
- ViewModel 只保存 UI 状态、发信号、转发命令。
- Mapper 负责 STL 领域对象到 UI 状态的转换。
- HMI Port 负责隔离 bridge 与系统总控实现。
- AppControl 负责线程调度、服务装配和应用级入口。
- service/core 负责业务规则和安全边界。

## 25. 总结

当前 Qt 与 STL bridge 没有明显方向性错误，最大优点是保持了 Qt 类型边界。
真正值得优化的是“未来扩展时的稳定性”：把直接依赖、字段搬运、显示格式、
错误字符串、轮询刷新和粗粒度信号这些问题逐步收束。

建议从三个小而关键的改动开始：

1. ViewModel 依赖 HMI 窄接口，而不是完整 `AppControl`。
2. 抽出 `UiState` 和 mapper，让 ViewModel 更像 Qt 适配器。
3. 分离结构化值、显示文本和操作状态，避免 UI 语义混杂。

完成这三步后，后续接入自检、报警、历史趋势、配置审计时，bridge 会更稳，
代码审查也更容易判断每一层到底在负责什么。
