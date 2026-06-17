# Runtime 周期调度最终设计

## 1. 核心结论

最终采用：

```text
统一调度机制，分层聚合管理。
```

具体含义：

- `Runtime` 是唯一的周期调度入口。
- 所有周期性行为统一注册为 `RuntimePeriodicTask`。
- service 线程、io 线程、safety 线程、monitor 线程使用同一套任务模型。
- 不再使用 `setServiceTickHandler` 这类特殊单例 tick handler。
- 不再通过 `serviceHost.tickAll()` 聚合驱动所有 service。
- 每个需要周期运行的 service 都单独注册自己的周期任务。
- 每个 IO 采样器、监控器、轮询器也单独注册自己的周期任务。
- 串口发送、GPIO 输出、PWM 设置这类一次性 IO 命令不注册为周期任务，
  而是通过 `postIoTask` 投递到 io 线程执行。

最终模型不是“一个线程一个大 tick”，而是“一个线程承载多个具名周期任务”。

## 2. 总体结构

```text
Runtime
  负责线程、一次性任务投递、周期任务调度、运行快照和任务级错误记录。

ServiceLifecycleRegistry
  管理 service 的注册、启动、停止和生命周期状态。
  不负责周期 tick 聚合。

Service 周期任务
  每个需要周期运行的业务服务单独注册到 service 线程。

IO 周期任务
  每个采样器、监控器或设备轮询器按自己的频率注册到 io 线程。

IO 一次性命令
  串口发送、GPIO 输出、PWM 设置等写操作通过 postIoTask 投递到 io 线程。

Safety 周期任务
  安全检查、超时检测、风险控制任务注册到 safety 线程。

Monitor 周期任务
  诊断快照、趋势记录、运行状态采集任务注册到 monitor 线程。
```

职责边界：

- `Runtime` 不理解业务服务，只理解线程角色和周期任务。
- service 对象负责自己的业务状态推进。
- IO 对象负责读取设备或系统数据，并更新最新快照。
- IO 写命令必须进入 io 线程执行，避免多个线程直接访问同一个设备对象。
- safety 对象负责检查风险条件并产生安全事件。
- monitor 对象负责低频诊断、统计和持久化触发。

## 3. 周期任务模型

所有周期任务统一表达为：

```cpp
RuntimePeriodicTask{
    "task.name",
    RuntimeThreadRole::service,
    std::chrono::milliseconds(25),
    [this]() {
        taskObject.runOnce();
    },
}
```

其中：

- `name`：任务名称，必须稳定、唯一、可诊断。
- `role`：任务运行的线程角色。
- `interval`：任务自己的运行周期。
- `run`：单次执行函数，应尽量短小，不能长期阻塞。

同一个线程上可以注册多个不同周期的任务。Runtime 负责判断任务是否到期，
并在对应线程中执行。

## 4. 一次性命令模型

不是所有后台行为都应该注册成周期任务。

`RuntimePeriodicTask` 用于“按固定频率反复执行”的任务，例如传感器采样、
安全检查、状态机推进和诊断快照。

`postIoTask` 用于“由外部事件触发的一次性 IO 写命令”，例如：

```text
serial.send(frame)
gpio.write(pin, value)
fan.setDuty(duty)
pwm.setDuty(channel, duty)
```

这些操作必须投递到 io 线程执行：

```cpp
runtime.postIoTask([this, frame = std::move(frame)]() mutable {
    serialPort.send(frame);
});
```

原因：

- 串口、GPIO、PWM、I2C、SPI 等设备对象通常不是线程安全的。
- 设备写操作需要和周期采样运行在同一个 io 线程，避免并发访问硬件。
- service、ui、safety 等线程不应直接调用底层设备写接口。
- 一次性命令没有固定周期，不应该伪装成周期任务。

如果调用方需要等待结果，可以封装异步返回：

```cpp
std::future<SerialSendResult> sendAsync(std::vector<std::uint8_t> frame);
```

内部仍然通过 `postIoTask` 投递到 io 线程，并在 io 线程中完成 promise。

约束：

- 不允许在 io 线程中再等待一个 io 线程任务完成。
- 需要返回结果时优先使用 future、callback 或事件。
- 需要排队、重试、超时和响应匹配时，应引入事务队列。

## 5. Service 线程最终形态

service 线程不再运行：

```text
serviceHost.tickAll()
```

每个需要周期推进的 service 都直接注册自己的周期任务。

示例：

```text
service.lifecycle.tick      25ms    lifecycleService.tick()
service.temp.tick           25ms    tempService.tick()
service.alarm.tick          25ms    alarmService.tick()
service.config.audit        500ms   configService.auditPendingChanges()
service.recovery.tick       100ms   recoveryService.tick()
```

这样做的好处：

- 每个 service 的周期可以不同。
- 每个 service 有独立运行次数、最近运行时间和最近错误。
- 单个 service tick 失败不会遮蔽其他 service。
- Qt 主屏或诊断页可以直接看到具体哪个 service 异常。
- 不需要在 `ServiceHost::tickAll()` 内部维护一套额外的循环和错误状态。

service 的启动和停止仍然需要统一生命周期管理。这个职责可以由
`ServiceLifecycleRegistry` 或保留后的 `ServiceHost` 承担，但它只负责：

- 注册 service。
- 按顺序 `start`。
- 按反序 `stop`。
- 维护 service 生命周期状态。
- 提供生命周期快照。

它不再负责周期 tick 调度。

## 6. IO 线程最终形态

IO 线程和 service 线程使用同一套模型。每个 IO 功能按自己的频率注册周期任务。

示例：

```text
io.temperature.sample       50ms    temperatureSampler.sample()
io.pressure.sample          20ms    pressureSampler.sample()
io.liquidLevel.sample       100ms   liquidLevelSampler.sample()
io.gpio.poll                50ms    gpioInputPoller.poll()
io.system.sample            200ms   systemInfoMonitor.sample()
io.fan.feedback.sample      500ms   fanFeedbackSampler.sample()
```

IO 周期任务只负责采集和更新快照，不直接决定业务状态。

IO 一次性命令通过 `postIoTask` 进入 io 线程执行。

示例：

```text
service.temp.tick
  -> 计算风扇占空比
  -> postIoTask(fanController.setDuty)

service.command.send
  -> 生成串口帧
  -> postIoTask(serialPort.send)

ui.gpio.command
  -> 发起 GPIO 输出请求
  -> postIoTask(gpioController.write)
```

推荐数据流：

```text
io.temperature.sample
  -> 更新 latestTemperatureSnapshot
  -> service.temp.tick 读取快照并计算风扇控制策略
  -> service.alarm.tick 或 safety.limit.check 判断是否报警
```

IO 线程不承载业务规则。业务规则应放在 service、core 或 safety 中。

串口如果涉及“发送后等待响应”，推荐使用事务队列：

```text
SerialTransactionQueue
  enqueue(request)
  postIoTask(serialPort.send)
  io.serial.poll 周期读取响应
  匹配 response
  完成 future / callback / event
  超时后返回错误
```

其中：

- `serial.send` 是一次性命令，走 `postIoTask`。
- `io.serial.poll` 是周期任务，走 `registerPeriodicTask`。
- 响应匹配、超时和重试由串口事务对象负责，不散落在业务服务中。

## 7. Safety 线程最终形态

safety 线程用于承载与风险控制强相关的周期检查。

示例：

```text
safety.sensor.timeout       50ms    检查传感器数据是否超时
safety.pressure.limit       20ms    检查压力上下限
safety.temperature.limit    50ms    检查温度上下限
safety.fan.feedback         100ms   检查风扇反馈是否异常
safety.watchdog.kick        100ms   喂狗或刷新软件看门狗状态
```

safety 任务应满足：

- 逻辑短小。
- 周期明确。
- 错误和报警可追踪。
- 不依赖 UI。
- 不执行慢速持久化操作。

## 8. Monitor 线程最终形态

monitor 线程用于低频诊断、趋势、统计和持久化触发。

示例：

```text
monitor.diagnostic.snapshot 1000ms  生成诊断快照
monitor.history.persist     1000ms  写入趋势或故障记录
monitor.runtime.snapshot    1000ms  汇总 Runtime 任务状态
monitor.selftest.archive    5000ms  归档自检报告
```

monitor 任务可以读取 Runtime 快照、service 状态、设备快照和报警状态，
但不应该反向控制关键业务流程。

## 9. 为什么不要 serviceHost.tickAll

`serviceHost.tickAll()` 的问题在于它把多个服务的周期执行折叠成一个大任务：

```text
service.host.tick
  -> lifecycleService.tick()
  -> tempService.tick()
  -> alarmService.tick()
  -> recoveryService.tick()
```

这样 Runtime 只能看到一个任务：

```text
service.host.tick
```

但看不到每个 service 的独立运行状态。对于医疗设备平台，这会降低诊断质量：

- 不知道具体哪个 service 最近没有运行。
- 不知道具体哪个 service 最近失败。
- 不方便为不同 service 设置不同周期。
- 不方便单独注销或替换某个 service 的周期任务。
- `ServiceHost` 会同时承担生命周期管理和调度聚合两个职责。

最终设计中，Runtime 应直接看到这些任务：

```text
service.lifecycle.tick
service.temp.tick
service.alarm.tick
service.recovery.tick
```

这样任务边界更清楚，诊断信息也更直接。

## 10. 为什么不要 ioTickAll

IO 层也不使用统一的 `ioTickAll()`。

原因与 service 一致：

- 不同设备采样周期不同。
- 不同设备可用性不同。
- 不同设备失败模式不同。
- 每个采样器都需要独立运行快照。
- 一个采样器失败不应影响其他采样器。

最终设计中，IO 线程上运行多个独立周期任务，而不是一个巨大轮询函数。

## 11. 注册聚合层

虽然每个任务都单独注册，但不代表注册代码要散落在各处。

可以保留装配层或注册聚合层：

```text
ServiceTaskRegistrar
  集中注册和注销 service 周期任务。

IoTaskRegistrar
  集中注册和注销 IO 周期任务。

IoCommandDispatcher
  集中封装串口发送、GPIO 输出、PWM 设置等一次性 IO 命令投递。

SafetyTaskRegistrar
  集中注册和注销 safety 周期任务。

MonitorTaskRegistrar
  集中注册和注销 monitor 周期任务。
```

这些类只负责“把对象绑定到 Runtime 周期任务”，不负责业务逻辑。

示例职责：

```text
ServiceTaskRegistrar
  register(service.temp.tick)
  register(service.alarm.tick)
  unregister(service.temp.tick)
  unregister(service.alarm.tick)
```

这样既能保持任务独立，又能避免 `AppControl` 变成巨大的注册函数。

## 12. 命名规范

周期任务名称使用层级式命名：

```text
service.lifecycle.tick
service.temp.tick
service.alarm.tick
service.recovery.tick

io.temperature.sample
io.pressure.sample
io.liquidLevel.sample
io.gpio.poll
io.system.sample
io.fan.feedback.sample

safety.sensor.timeout
safety.pressure.limit
safety.temperature.limit
safety.watchdog.kick

monitor.diagnostic.snapshot
monitor.history.persist
monitor.runtime.snapshot
```

一次性 IO 命令的封装函数使用动作命名：

```text
sendSerialFrameAsync
writeGpioAsync
setFanDutyAsync
setPwmDutyAsync
```

命名原则：

- 第一段表示线程或职责域。
- 第二段表示对象、设备或模块。
- 第三段表示动作，周期任务常用 `tick`、`sample`、`poll`、`check`、`persist`。
- 名称必须稳定，进入日志、数据库或诊断 UI 后尽量保持兼容。
- 名称不能使用临时实现细节。

## 13. 任务设计约束

周期任务必须遵守：

- 单次执行尽量短小。
- 不做长期阻塞操作。
- 不在周期任务中等待同线程任务完成。
- 不直接访问不属于本线程的非线程安全对象。
- 跨线程交互优先使用 `postServiceTask`、`postIoTask` 或快照数据。
- 错误必须由 Runtime 捕获并写入任务快照。
- 任务注销后不得再访问已销毁对象。

service 任务应消费快照并推进业务状态。

IO 任务应采集数据并更新快照。

safety 任务应检查风险并产生报警或安全事件。

monitor 任务应汇总状态并触发低频记录。

一次性 IO 命令必须遵守：

- 只能通过 `postIoTask` 进入 io 线程访问设备对象。
- 不能在非 io 线程直接访问串口、GPIO、PWM、I2C、SPI 等设备对象。
- 不能在 io 线程中等待 io 线程任务完成。
- 需要结果时使用 future、callback 或事件。
- 需要响应匹配时使用事务队列。
- 需要周期读取或超时检查时注册独立周期任务。

## 14. 最终原则

最终架构中没有特殊 tick handler，也没有按线程聚合的大 tick。

```text
RuntimePeriodicTask 是唯一周期调度模型。
postIoTask 是一次性 IO 命令入口。
```

service、io、safety、monitor 都通过具名周期任务运行。生命周期管理可以聚合，
但周期执行不聚合。串口发送、GPIO 输出、PWM 设置等一次性写命令不伪装成
周期任务，而是投递到 io 线程执行。

这样每个后台行为都有明确线程、明确周期或触发来源、明确名称、明确错误状态
和明确诊断入口。

对于需要开机自检、运行监测、故障诊断、视觉报警、数据库留痕和异常恢复的
医疗设备底层平台，这种结构更适合长期扩展和问题回溯。
