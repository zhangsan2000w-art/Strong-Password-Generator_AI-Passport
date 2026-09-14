# 架构与决策记录

[English](ARCHITECTURE.md) | 简体中文

## 边界

```text
UP / DOWN / OK
      |
AI Passport BSP 按键回调，仅入队
      |
FreeRTOS 输入任务 + LVGL 适配层（C）
      |
MoonBit 状态模型与密码核心
      |
C FFI：安全随机、输出缓冲、Flash 词库
      |
ESP-IDF + 官方 AI Passport BSP
```

MoonBit 负责产品规则，C 负责平台边界。MoonBit 核心模块不导入 LVGL、GPIO、FreeRTOS 或 AI Passport 头文件。

## MoonBit 核心

| 模块 | 职责 |
| --- | --- |
| `policy.mbt` | 长度边界、字符集和分隔符 |
| `random_source.mbt` | 基于回调的随机抽象与拒绝采样 |
| `password.mbt` | Random 密码与 PIN 生成 |
| `passphrase.mbt` | 词库选择与格式化 |
| `entropy.mbt` | 面向显示的熵估算 |
| `state.mbt` | 压缩应用模型及 NAVIGATION/EDITING 状态转换 |
| `ffi.mbt` | 稳定的 C 导出 API 与平台回调 |

应用状态压缩在一个 `UInt64` 中，C 适配层将其视为不透明值。状态机处理每次输入前会清除一次性 action 字段，因此停留在生成行时再次按 `OK` 可以有意重新生成。

## 平台集成

`tools/generate_moonbit.py` 针对 native 目标调用 `moonc build-package` 和 `moonc link-core`。ESP-IDF 随后编译生成的 C 与仓库内 MoonBit runtime。测试文件和主机 stub 不会进入固件代码生成。

FFI 表面只包含整数、不透明 64 位状态、一个 `UInt` 随机回调、字符输出及词库查询。未来更换板卡时，可以只替换这些回调和 UI 适配层，无需重写生成规则。

EFF 词库在构建时生成为一个只读 NUL 分隔字节数组和一张 `uint16_t` 偏移表。MoonBit 按需读取单个字符，启动时不会在堆上创建整份词库副本。

## UI 与并发

240×320 LVGL 页面开机后直接创建。所有动态标签只使用可显示 ASCII 或内置 Noto Sans SC 子集。普通焦点使用填充背景和边框，EDITING 使用独立橙色。

BSP 按键回调只把 `{button,event}` 复制到固定深度 FreeRTOS 队列并立即返回。输入任务完成 MoonBit 状态转换或生成，再仅在刷新视觉对象时获取 BSP LVGL 锁。RNG 与生成逻辑既不在按键回调中执行，也不在持有 LVGL 锁时执行。

## 决策

| 决策 | 原因 | 主要备选 | 验证方式 | 当前限制 |
| --- | --- | --- | --- | --- |
| MoonBit native-to-C 代码生成 | 让真实产品核心进入固件，同时保留 ESP-IDF 工具链 | 用 C 重写核心 | 构建日志编译 `generated/password_core.c`，导出符号链接进应用 | 构建时需要兼容的 MoonBit 工具链 |
| 回调式随机源 | 让算法与 ESP-IDF 解耦，并支持确定性测试 | 在核心内直接调用 `esp_random()` | 拒绝采样与确定性序列测试 | C 回调失败通过输出失败状态表达 |
| 在按键 ADC 初始化前为 CTR-DRBG 播种 | GPIO0 ADC 电阻梯被占用前可取得硬件熵，后续请求不与 ADC1 竞争 | 每次生成时保持 RF 开启 | ESP-IDF 构建与真机生成验收 | 播种健康和长时间行为需要真机验证 |
| 拒绝采样 | 避免字符、数字和单词索引的取模偏差 | 直接取模 | 边界测试强制拒绝不完整桶 | V1 不包含统计认证 |
| Flash 打包 1,296 词词库 | Flash 占用有界，启动 RAM 开销接近零 | 启动时解析并加载文本 | 生成器校验词数和格式，固件布局检查 | 词汇量有意小于 EFF 长词库 |
| 生成 CJK 字体子集 | 避免中文缺字，也不引入完整大字库 | 纯英文 UI 或完整 CJK 字体 | 生成范围包含全部 V1 文案，仍需真机看屏 | 新增 UI 文案后必须重新生成子集 |
| V1 不持久化设置 | 避免误存密码并保持 V1 聚焦 | 仅把非敏感偏好写入 NVS | 源码审计与真机验收 | 重启后参数恢复默认值 |
