# mlcp-hmi 架构说明
本文档说明 `mlcp-hmi` 的目录职责、模块边界和代码放置规则。
本项目是基于 **Qt + STL + Linux** 的医疗器械 HMI 软件。
运行在arm linux下这类资源较少的环境中，代码结构需要兼顾性能、安全和可维护性。
目标是：核心逻辑可测试、模块职责清晰、安全行为可追踪、代码边界稳定。
---
## 1. 总体原则
- `app`：程序入口、初始化、对象装配。
- `ui`：界面显示、用户输入、HMI 交互。
- `bridge`：UI 与业务层之间的适配。
- `service`：应用用例编排、跨模块协调。
- `core`：核心业务、安全规则、状态机、流程。
- `device`：具体硬件访问与设备协议。
- `persistence`：数据库、文件和持久化数据。
- `security`：认证、权限、加密、审计。
- `system`：Linux 系统能力、运行环境、生命周期。
- `common`：无业务语义的公共基础设施。
- `test`：测试代码、测试数据、故障注入场景。
- `third_party`：第三方依赖、license、patch、版本说明。
推荐依赖方向：
```text
app -> ui -> bridge -> service -> core
service -> device / persistence / security / system
core -> common
```
核心约束：
- `core` 不依赖 Qt、Linux API、数据库、串口、UI。
- Qt 类型主要出现在 `app`、`ui`、`bridge`。
- Linux 相关代码主要出现在 `system`、`device`、`app`。
- UI 不直接访问设备、数据库或系统 API。
- 设备层不决定核心业务流程。
- 关键操作必须经过权限检查并可审计。
---
## 2. 当前目录结构
```text
mlcp-hmi
├── app
├── bridge
├── common
│   ├── error
│   ├── result
│   ├── types
│   ├── utils
│   └── version
├── core
│   ├── algorithm
│   ├── safety
│   ├── state_machine
│   └── workflow
├── device
│   ├── fan
│   ├── pressure
│   ├── temp
│   ├── rotator
│   └── serial
├── persistence
│   ├── database
│   └── storage
├── security
│   ├── audit
│   ├── auth
│   ├── crypto
│   └── permission
├── service
│   ├── alarm
│   ├── config
│   ├── example
│   ├── lifecycle
│   ├── selftest
│   ├── temp
│   ├── event
│   ├── log
│   └── memory
├── system
│   ├── appcontrol
│   ├── diagnostic
│   ├── power
│   └── runtime
├── test
├── third_party
└── ui
    ├── pages
    └── resources
```
---
## 3. 目录职责
### 3.1 app
`app` 是应用入口和装配层。
应放：`main.cpp`、Qt Application 初始化、启动参数解析、全局日志初始化、配置初始化、数据库初始化、安全模块初始化、设备模块初始化、service/bridge/ui 对象创建、主窗口或 QML Engine 启动、程序退出流程、异常兜底、崩溃处理初始化。
不应放：核心算法、业务状态机、设备协议、数据库表操作细节、UI 页面细节。
### 3.2 bridge
`bridge` 是 UI 与业务层之间的适配层，用于隔离 Qt UI 和业务模块。
应放：UI 到 service 的调用适配、service 返回结果到 UI 模型的转换、Qt signal/slot 与业务命令转换、DTO、ViewModel mapper、线程调度封装、异步任务封装、业务事件到 UI 通知的转换。
不应放：核心业务规则、设备通信协议、数据库实现、安全策略本身。
### 3.3 common
`common` 是全项目可依赖的基础设施目录。
- `error`：错误码、错误分类、错误描述。
- `result`：`Result<T>`、成功/失败返回封装。
- `types`：无强业务语义的基础类型。
- `utils`：字符串、时间、范围检查等通用工具。
- `version`：软件版本、构建版本、协议版本等信息。
约束：不放业务流程、设备逻辑、UI 逻辑；不建议依赖 Qt；不要把 `common` 变成杂物目录。
### 3.4 core
`core` 是核心业务逻辑目录，应尽量保持纯 C++/STL。
- `algorithm`：核心算法、计算模型、参数计算、数据处理算法。
- `safety`：安全规则、安全联锁、危险状态判断、保护策略。
- `state_machine`：设备状态机、治疗流程状态机、异常恢复状态机。
- `workflow`：核心业务流程、治疗流程、操作流程、流程切换规则。
约束：不依赖 `QString`、`QObject`、`QTimer` 等 Qt 类型；不直接访问串口、数据库、文件系统、Linux API；不直接写日志或审计；不直接弹窗或控制 UI。
### 3.5 device
`device` 是硬件设备访问层。
- `fan`：风扇控制、转速设置、状态读取、故障检测。
- `pressure`：压力传感器采集，读取 `rumi-pressure` 驱动暴露的 sysfs
  属性，提供 raw、电压和 kPa 压力值；校准、异常检测和报警策略由上层模块编排。
- `temp`：温度传感器采集，读取温度驱动暴露的 sysfs 属性，提供摄氏温度值；恒温策略和风扇联动由 `service/temp` 编排。
- `rotator`：旋转机构控制、位置读取、限位处理、运动错误处理。
- `serial`：串口打开关闭、收发、超时、协议帧、CRC、重试机制。
约束：可以使用 Linux 设备接口和串口 API；对上层暴露稳定接口；不泄漏底层协议细节；不决定治疗流程；不直接操作 UI。
### 3.6 persistence
`persistence` 是数据持久化层。
- `database`：数据库连接、表结构、查询、事务、迁移、repository 实现。
- `storage`：文件存储、配置文件、导出文件、备份文件、数据完整性校验。
约束：不放核心业务规则、UI 逻辑、设备通信逻辑；数据库异常应转换为统一错误码；重要数据写入应考虑一致性、完整性和恢复能力。
### 3.7 security
`security` 是认证、授权、加密和审计目录。
- `audit`：关键操作审计、用户行为记录、安全事件记录。
- `auth`：登录、会话、密码策略、身份认证。
- `crypto`：加密、解密、签名、Hash、密钥管理封装。
- `permission`：角色权限、功能权限、操作授权策略。
约束：审计日志与普通运行日志分离；影响安全的配置修改必须可审计；登录、权限变更、关键操作、报警确认应可追踪；密码、密钥、token 不应明文存储或输出到日志。
### 3.8 service
`service` 是应用服务层，用于组织业务用例。
- `service.h` / `serviceHost`：服务生命周期公共接口与服务宿主，统一执行
  `start`、`stop` 和生命周期快照维护；新增服务需要实现 `IService`，
  通过 `runtimeTasks()` 声明自己的周期任务，由 `system/appcontrol`
  中的统一注册器汇总注册。
- `alarm`：报警触发、报警确认、报警静音、报警恢复、报警分发。
- `config`：配置读取、修改、校验、默认值、权限检查、配置变更通知。
- `example`：新增业务服务的复制样板，包含 `IService`、`snapshot()`、
  `runtimeTasks()` 和采样类 `snapshot()` 约定；正式业务接入时复制后改名，
  并提供独立配置存储或 repository。
- `selftest`：开机自检与 UI 命令触发自检的一次性流程，统一编排检查项、生成自检报告并上报生命周期状态。
- `temp`：温控服务，运行在 service 线程中，周期消费 IO 线程采集的温度快照并计算风扇占空比；依赖设备层抽象，不直接访问 UI。
- `event`：事件总线、事件分发、订阅通知、模块间事件传递。
- `log`：运行日志、调试日志、模块日志、日志等级控制。
- `memory`：内存状态监控、缓存管理、运行内存告警。
约束：可协调 `core`、`device`、`persistence`、`security`、`system`；不应把核心安全判断写散在 service 中；不应直接包含 UI 页面逻辑；不应直接暴露数据库表结构给 UI。
### 3.9 system
`system` 是 Linux 系统能力和运行环境目录。
- `appcontrol`：应用启动、停止、重启、升级控制。`AppControl` 只保留启动、
  停止和对 `bridge` 暴露的应用接口；`DeviceRegistry` 负责发现并持有风扇、
  温度、压力、液位等设备对象；`AppWiring` 负责服务装配、服务宿主和
  自检项注册；`ServiceRegistry` 汇总服务、服务自检项和服务声明的
  runtime 任务；`TaskRegistry` 负责集中注册和注销运行期周期任务。
- `diagnostic`：系统诊断、硬件诊断、环境检查、故障收集。
- `lifecycle`：进程生命周期、初始化阶段、运行阶段、退出阶段管理。
- `power`：关机、重启、电源状态、电源保护策略。
- `runtime`：CPU、内存、磁盘、线程、时间、运行状态监控。
约束：可以使用 Linux API；不放核心业务流程；不放 UI 页面细节；系统异常应转换为统一错误或事件。
### 3.10 test
`test` 存放测试代码和测试资源。
应放：单元测试、集成测试、状态机测试、安全规则测试、设备 mock 测试、数据库测试、UI/HMI 交互测试、测试数据、测试夹具、故障注入场景。
重点：`core/safety`、`core/state_machine`、报警、权限、配置修改、审计逻辑必须重点覆盖。
### 3.11 third_party
`third_party` 存放第三方依赖。
应放：第三方库源码或二进制包、license、patch、版本说明、依赖清单、安全漏洞或兼容性说明。
约束：第三方库版本必须明确；修改第三方库应保留 patch 说明；医疗器械项目应能追踪第三方组件来源和版本。
### 3.12 ui
`ui` 是 Qt HMI 表现层。
- `pages`：页面、窗口、对话框、页面导航相关代码。
- `resources`：图片、字体、样式、翻译、QRC、静态资源。
应放：UI 控件、页面布局、用户输入展示、状态展示、HMI 提示、确认弹窗、报警展示、权限受限提示。
约束：不直接访问数据库、串口或设备；不直接实现核心安全规则；UI 输入校验只作为第一层提示；关键操作应通过 service 发起，并记录审计。
---
## 4. 典型调用链
用户启动操作：
```text
ui/pages -> bridge -> service -> core/workflow -> core/safety -> device -> service/event -> ui/pages
```
配置修改：
```text
ui/pages -> bridge -> service/config -> security/permission -> core/safety -> persistence -> security/audit
```
报警产生：
```text
device or core -> service/alarm -> security/audit -> persistence/database -> bridge -> ui/pages
```
---
## 5. 医疗器械项目约束
- 安全规则集中在 `core/safety`。
- 状态机集中在 `core/state_machine`，状态迁移必须可测试。
- 报警逻辑由 `service/alarm` 编排，报警条件由 `core/safety` 判断。
- 关键操作必须经过权限检查，并写入 `security/audit`。
- 影响安全的配置必须有范围、默认值、权限、审计和回滚策略。
- 设备故障、通信超时、传感器异常必须转换为明确错误或报警事件。
- UI 不应绕过 service 直接控制设备。
- 数据持久化应考虑断电、异常退出、写入失败和恢复场景。
- 第三方依赖需要记录版本、来源、license 和修改记录。
---
## 6. Code Review 检查项
- 是否把 Qt 类型引入了 `core`？
- 是否把 Linux API 引入了 `core`？
- UI 是否直接访问了 device 或 database？
- 核心安全判断是否放在 `core/safety`？
- 状态迁移是否放在 `core/state_machine`？
- 关键操作是否有权限检查和审计？
- 设备错误是否转换为统一错误或事件？
- 配置修改是否有范围校验和审计记录？
- 第三方依赖是否记录版本和 license？
- 新增安全、报警、状态机逻辑是否有测试？
---
## 7. 总结
最重要的原则：
```text
core 保持纯净，UI 不碰硬件，设备不管业务，安全逻辑集中，关键操作可审计。
```
