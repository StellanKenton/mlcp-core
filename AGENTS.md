# Agent Bootstrap

Before making any changes in this repository, read [rule.md](./rule/rule.md) once.
Treat `rule.md` as the session bootstrap rule file for this project

# Arch

**一句话定义**：一台医疗设备底层平台——Linux 驱动接入真实外设，C++17 服务统一管理设备状态，Qt 主屏 + OLED 副屏双屏显示，具备开机自检、运行监测、故障诊断、视觉报警、数据库留痕和异常恢复能力。

**能力边界目标**：

- Linux 驱动深度：I2C/SPI/GPIO 中断/PWM/字符设备/sysfs/debugfs/input 子系统。
- C++ 工程化：设备抽象层、状态机、配置、日志、错误码、数据库持久化、测试、模块边界。
- 医疗设备表达：开机自检、报警优先级、故障码、风险控制、异常恢复。
- 可演示作品集：双屏显示、旋钮交互、传感器闭环、故障注入。

模拟一台"便携式医疗设备底层控制平台"，完整运行流程：

1. **上电** → 系统启动，进入 `BOOTING` 态。
2. **开机自检** → 自动检查所有外设和子系统，生成自检报告。
3. **自检通过** → 进入 `STANDBY` 待机态，等待用户操作。
4. **用户操作** → 通过旋钮调节目标压力、风量或报警阈值。
5. **启动运行** → 进入 `RUNNING` 态，持续监测压力、液位、温度、风扇。
6. **异常发生** → 检测到低液位/堵塞/泄漏/超压/传感器超时，触发视觉报警，写入数据库，进入 `ALARM` 或 `DEGRADED`。
7. **故障恢复** → 异常消除后恢复至 `STANDBY` 或 `RUNNING`，全程留痕。
8. **诊断回溯** → Qt 主屏可查询历史趋势、自检报告、故障记录和配置变更.
