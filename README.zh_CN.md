# Strong Password Generator_AI Passport

[English](README.md) | 简体中文

**Strong Password Generator_AI Passport** 是一款面向 FoloToy AI Passport 和 MoonBit Hackathon 2026 的离线三键强密码生成器。

固件启动后直接进入生成器，不联网、不传输生成结果、不保存密码历史，也不重新定义系统电源键。

## 功能

- **随机**：6～30 个可显示 ASCII 字符，默认长度 10；字母始终开启，可选数字和符号。开启的可选字符类别保证至少出现一个。
- **易记**：3～6 个离线单词，默认 4 个；支持首字母大写、完整单词或四字符缩写，以及 `-`、`.`、`_` 分隔符。
- **PIN**：4～12 位十进制数字，默认 6 位。
- **输入**：只使用 `UP`、`DOWN`、`OK`。`OK` 用于进入或确认编辑、切换布尔值及生成；编辑数值时长按 `UP` / `DOWN` 可连续增减；长按 `OK` 取消当前编辑，在普通主界面不执行破坏性动作。
- **反馈**：成功双音的频率、时长、包络和 PCM 样本由 MoonBit 生成，C 音频任务只负责非阻塞播放。
- **显示**：240×320 赛博朋克深色界面；中文采用 17px、4bpp、强提示字体子集；MoonBit 视图模型决定参数槽位、焦点、布局、强度颜色及电量显示策略。
- **核心**：生成策略、无偏随机索引、输出后置校验、熵与强度、应用状态、视图模型、电池策略和声音合成都由 MoonBit 实现。

## MoonBit 主体实现

当前仓库有 1,630 行生产 `.mbt` 与 659 行 MoonBit 测试，共 2,289 行。排除测试、空行和注释后，生产 MoonBit 有效代码为 1,223 行。`tools/check_repo.py` 会独立检查生产实现不少于 1,000 有效行，测试代码不能用于凑这个门槛。

MoonBit 生产模块直接进入 ESP-IDF 构建，并被固件调用：

- 密码、PIN、Passphrase 算法及启用字符类别保证；
- RandomSource 抽象、rejection sampling 和生成后安全校验；
- 参数约束、熵估算与强度分级；
- NAVIGATION / EDITING 状态机与配置差异判断；
- UI 参数槽位、坐标、焦点、编辑态和值的视图模型；
- CW2017 原始读数到可显示电量的纯策略；
- 成功双音序列、淡入淡出包络和 PCM 样本生成。

C 只保留 ESP-IDF/BSP 初始化、LVGL 控件绘制、I2C 原始读数、FreeRTOS 调度、音频写入、安全随机源及 Flash 词库访问。

## 上游项目与来源说明

本项目基于开源的 [FoloToy AI Passport](https://github.com/FoloToy/ai-passport) 固件与硬件支持工程进行开发。

原项目中的 ESP-IDF/BSP、硬件驱动、外设支持及相关组件继续遵循其原有许可证和版权声明。

本仓库在此基础上新增了面向 MoonBit Hackathon 2026 的 MoonBit 密码生成核心、产品逻辑、交互设计、UI、测试、文档及相关固件修改。

## 工具链

- ESP-IDF 5.5.3，目标为 `esp32c3`
- 支持 native/C 后端的 MoonBit，`moon` 与 `moonc` 位于 `PATH`
- Python 3

本次验证环境使用 `moon 0.1.20260904` 和 `moonc v0.10.12+1634b282e`。ESP-IDF 构建会调用 [`tools/generate_moonbit.py`](tools/generate_moonbit.py)，将 MoonBit 包编译为可移植 C，再链接到 `moonbit_password` ESP-IDF 组件。生成的 C 是构建产物，不进入 Git。

GitHub Actions 安装 MoonBit 的 `latest` 稳定通道，因为官方 CDN 不保证带日期的 CLI 历史包长期可下载。每次 CI 日志打印的实际工具版本即为该次验证记录；上面的版本仍是本地已经验证的开发快照。

## 测试

运行完整静态检查和主机测试：

```bash
./tools/validate.sh --static
```

也可单独运行 MoonBit 核心测试：

```bash
MOONBIT_NEW_NATIVE=0 moon -C moonbit check --target native --deny-warn
MOONBIT_NEW_NATIVE=0 moon -C moonbit test --target native --release
```

确定性随机源只用于主机测试。固件随机数始终通过 C FFI 进入 ESP32 适配器。

## 构建

激活 ESP-IDF 5.5.3，确认 MoonBit 位于 `PATH`，然后运行：

```bash
./tools/validate.sh --firmware
```

运行全部门禁：

```bash
./tools/validate.sh
```

固件门禁会使用全新的临时构建目录，执行 `idf.py build`，再用 `idf.py merge-bin` 生成合并镜像，检查各镜像偏移及分区边界，最后只复制验证通过的文件到：

```text
build/FoloToy-AI-Passport-full.bin
```

`build/FoloToy-AI-Passport.bin` 只是应用单镜像，不能代替合并完整固件。

Windows 可在 ESP-IDF PowerShell 激活环境后进入 Git Bash。如果 Microsoft Store 的 `python3` 别名不可用，可显式传入 ESP-IDF Python：

```bash
PYTHON='D:/path/to/idf-python/Scripts/python.exe' ./tools/validate.sh --static
```

如果受限 Windows 环境无法写入已配置的 ccache 目录，可在固件门禁中设置 `IDF_NO_CCACHE=1`；CI 默认仍启用 ccache。

## 烧录

合并镜像从 `0x0` 写入：

```bash
python -m esptool --chip esp32c3 --baud 460800 \
  --before default-reset --after hard-reset \
  write-flash 0x0 build/FoloToy-AI-Passport-full.bin
```

也可使用 AI Passport 官方网页烧录器选择同一个 `-full.bin` 文件。编译成功或烧录完成不能证明 UI、按键、字体、电源行为和 RNG 适配器已经通过真机验收。

## 离线词库与字体

- 易记模式内置 Electronic Frontier Foundation 的 1,296 词 [EFF Short Wordlist for Passphrases #1](https://www.eff.org/files/2016/09/08/eff_short_wordlist_1.txt)，按 CC BY 3.0 US 署名使用。仓库内源文件 SHA-256 为 `8f5ca830b8bffb6fe39c9736c024a00a6a6411adb3f83a9be8bfeeb6e067ae69`。
- 构建时代码生成器将单词打包为一个以 NUL 分隔的常量字节块和 16 位偏移表。数据保留在 Flash 中，启动时不会整体加载进 RAM。
- 中文 LVGL 字形子集由 Noto Sans SC 生成，OFL 1.1 声明位于 [`assets/fonts/NotoSansSC-OFL.txt`](assets/fonts/NotoSansSC-OFL.txt)。固件仅编入 ASCII 和 V1 界面需要的中文字形；当前子集使用 17px、4bpp 和强 autohint 改善小屏笔画粗细。
- 字形子集使用 LVGL 压缩字体格式，因此 `sdkconfig.defaults` 启用 `CONFIG_LV_USE_FONT_COMPRESSED=y`。如果面板和生成结果正常，但标题、模式与按钮文字为空白，请按默认配置重新构建，并确认生成的 `sdkconfig` 包含该选项。
- 随固件编译的 MoonBit runtime 文件保留 Apache-2.0 声明，见 [`components/moonbit_password/RUNTIME_LICENSE.txt`](components/moonbit_password/RUNTIME_LICENSE.txt)。项目自有代码沿用仓库 MIT License。

## 设计与安全

- [架构与决策记录](docs/application/ARCHITECTURE.zh_CN.md)
- [安全模型与限制](docs/application/SECURITY.zh_CN.md)
- [AI 使用说明](docs/application/AI_USAGE.zh_CN.md)
- [AI Passport 官方文档索引](docs/README.zh_CN.md)

## 验证状态

35 项 MoonBit 主机测试、生产 MoonBit 有效行数门禁、仓库检查、辅助 Python/C 主机测试，以及包含本轮修改的 ESP-IDF 5.5.3 构建均已通过。用户确认上一版中文字体已经正常显示，但笔画偏细。当前加粗字体、电量连续跟踪修复、MoonBit 视图/电量/声音迁移和成功音效仍需随新固件一起真机验证。首次电池 profile 修复只在开机初始显示正常，运行后仍出现 77% 和不充电冷启动 0%，因此当前实现进一步取消了每次开机复位 CW2017。
