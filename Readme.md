# Music-Frequency-Spectrum —— STM32F103 音频频谱分析仪 项目分析

> 分析日期：2026-08-31
> 平台：STM32F103C8T6（LQFP48）
> 工具链：STM32CubeMX 6.9.2 + STM32Cube FW_F1 V1.8.5 + MDK-ARM V5.32

---

## 1. 项目概述

本项目是一个基于 STM32F103C8T6 的**音频频谱分析仪**，核心功能是：

1. 通过 **ADC + DMA + 定时器触发** 对模拟音频信号进行等间隔采样；
2. 使用 **CMSIS-DSP 库** 的 FFT 函数将时域信号变换到频域，并求取幅值谱；
3. 通过 **SPI 接口的 SSD1306 OLED（128×64）** 实时绘制频谱柱状图 / 波形图；
4. 通过 **USART1** 输出调试数据（ADC 原始值、频率等）。

整体信号链路：

```
模拟音频 → PA1(ADC1_IN1) → TIM3 触发 ADC 采样 → DMA 搬运
        → arm_cfft_f32 快速傅里叶变换 → arm_cmplx_mag_f32 求模
        → 幅值归一化 → 峰值频率检测 → u8g2 绘制 OLED 频谱
```

---

## 2. 硬件平台与引脚分配

| 引脚 | 功能 | 说明 |
|------|------|------|
| PA1  | ADC1_IN1 | 模拟音频输入 |
| PA3  | OLED_CS  | OLED 片选（GPIO 输出） |
| PA4  | OLED_RES | OLED 复位（GPIO 输出） |
| PA5  | SPI1_SCK | OLED 时钟 |
| PA6  | OLED_DC  | OLED 数据/命令选择（GPIO 输出） |
| PA7  | SPI1_MOSI| OLED 数据 |
| PA9  | USART1_TX | 调试串口发送 |
| PA10 | USART1_RX | 调试串口接收 |
| PA13/PA14 | SWDIO/SWCLK | 调试下载口 |
| PD0/PD1 | HSE 8MHz | 外部晶振 |

**时钟配置：**
- 系统时钟 SYSCLK = **72 MHz**（HSE 8MHz × PLL9）
- APB1 = 36 MHz，APB2 = 72 MHz
- ADC 时钟 = PCLK2 / 6 = **12 MHz**（满足 F1 系列 ≤14MHz 限制）
- SPI1 = 72MHz / 32 = **2.25 Mbit/s**

---

## 3. 软件架构 / 文件结构

```
项目根目录
├── Core/                       # CubeMX 生成的外设驱动
│   ├── Inc/  main.h, adc.h, dma.h, gpio.h, spi.h, tim.h, usart.h
│   └── Src/  main.c, adc.c, dma.c, gpio.c, spi.c, tim.c, usart.c,
│             stm32f1xx_it.c, stm32f1xx_hal_msp.c
├── Drivers/
│   ├── CMSIS/                  # 内核支持
│   ├── STM32F1xx_HAL_Driver/   # HAL 库
│   └── DSP/                    # CMSIS-DSP 库（arm_cortexM3l_math.lib）
├── HardWare/                   # 用户自定义硬件层
│   ├── fft.c / fft.h           # FFT 核心算法封装
│   └── sys.c / sys.h           # 系统辅助（位带操作等，来自正点原子）
├── U8G2/                       # u8g2 图形库（OLED 显示）
│   ├── oled_driver.c/.h        # SPI 底层移植 + 测试函数
│   └── u8g2 / u8x8 系列源文件
├── Spectral Music.ioc          # CubeMX 工程配置
└── MDK-ARM/                    # Keil 工程与编译产物
```

---

## 4. 信号处理流程详解

### 4.1 ADC 采样（定时器触发 + DMA）

- **TIM3** 配置为向上计数，`PSC=72-1`、`ARR=100-1`，主输出触发选 `TIM_TRGO_UPDATE`（更新事件）。
- **ADC1** 配置为外部触发 `ADC_EXTERNALTRIGCONV_T3_TRGO`，即每次 TIM3 更新事件触发一次 ADC 转换。
- **DMA1_Channel1** 配置为**循环模式**，外设→内存，负责 ADC 数据搬运。

关键采样频率计算：

```
TIM3 计数时钟 = 72MHz / PSC(72) = 1 MHz
更新周期    = ARR(100) / 1MHz = 100 µs
采样频率 Fs = 1 / 100µs = 10 kHz
```

### 4.2 FFT 计算（CMSIS-DSP）

在 `fft.c` 中：

```c
#define SAMPLING 256                       // FFT 点数 N
#define arm_cfft_sR_f32_lenxxx &arm_cfft_sR_f32_len256
```

FFT 流程（`fft_calculation()`）：
1. 将 ADC 模拟电压值填入 `testInput`（实部=电压，虚部=0）；
2. `arm_cfft_f32()` 执行 256 点 FFT；
3. `arm_cmplx_mag_f32()` 计算幅值谱；
4. 幅值归一化：`output[0] /= N`（直流），`output[i] /= N/2`（其余频点）。

### 4.3 关键频谱参数

```
采样频率 Fs   = 10 kHz
FFT 点数 N    = 256
频率分辨率 Δf = Fs / N = 10000 / 256 ≈ 39.06 Hz
奈奎斯特频率  = Fs / 2 = 5 kHz（有效可测频率上限）
```

### 4.4 峰值频率检测

`GetPowerMag()` 遍历 `[1, SAMPLING/2)` 区间寻找幅值最大的频点，由 `MAX_F_POINT` 反推该频点的频率：

```c
MAX_F = (FFT_FS / SAMPLING) * MAX_F_POINT;
```

---

## 5. 显示模式

`main.c` 中定义了 5 个显示函数，主循环当前启用 `display3() + display4() + display5()`：

| 函数 | 内容 | 说明 |
|------|------|------|
| `display1()` | 32 根柱状图（上半屏） | 全高 64px，每柱宽 3px，带峰值下落 |
| `display2()` | 128 根柱状图 | 全屏细柱状图，带峰值下落 |
| `display3()` | 32 根柱状图（下半屏 y=32~63） | 当前启用，柱体 + 顶部下落亮点 |
| `display4()` | 顶部文字栏 | 显示 `F:峰值频率 E:频率分辨率 SF:采样频率` |
| `display5()` | 波形图（示波器模式） | 用折线绘制 ADC 时域波形 |

- 峰值下落效果通过全局数组 `fall_pot[128]` 记录每个柱子的历史峰值，每帧递增实现“下落”动画。
- OLED 使用 u8g2 库的 `ssd1306_128x64_noname_f`（全缓冲模式），SPI 4 线硬件驱动（`oled_driver.c` 中的 `u8x8_byte_4wire_hw_spi`）。

---

## 6. 发现的问题与改进建议

> 以下问题按严重程度从高到低排列。

### 🔴 6.1 `GetPowerMag()` 中局部变量 `Mag` 未初始化（严重逻辑错误）

```c
void GetPowerMag(void)
{
    float Mag;                       // ❌ 未初始化为 0，初值为随机垃圾值
    ...
    for(uint16_t i=1;i<SAMPLING/2;i++)
    {
        if(Mag<fft_Struct.testOutput[i]){Mag=...; MAX_F_POINT=i;}
    }
}
```

`Mag` 是栈上局部变量，初值不确定（往往是残留的垃圾值，可能极大）。若垃圾值大于所有频点幅值，则 `MAX_F_POINT` 保持为上一次的值甚至初始值，导致峰值频率检测结果**不稳定、错误**。

**修复：**
```c
float Mag = 0.0f;
```

### 🔴 6.2 `fft_get_adc_value()` 每帧 printf 打印 256 个浮点数，严重拖慢主循环

```c
for(uint16_t i=0;i<SAMPLING;i++)
{
    fft_Struct.ADC_Analog_Value[i] = ...;
    printf("%02f\r\n", fft_Struct.ADC_Analog_Value[i]);   // ❌ 每帧打印 256 行
}
```

以 115200 波特率估算，每个浮点约 10 字符 ≈ 100 bit，256 个点 ≈ 25600 bit，单帧仅串口传输就需 **约 220ms**，导致 OLED 刷新率被压到 5 FPS 以下，实时性严重受损。

**修复：** 删除或注释掉这行 `printf`，或将调试输出移到独立的低频/条件触发分支。

### 🟠 6.3 ADC/DMA 采集方案设计冗余（DMA 形同虚设）

```c
HAL_ADC_Start_DMA(&hadc1, (uint32_t *)fft_Struct.ADC_Digital_Value, 1); // 长度=1
```

DMA 以**循环模式 + 长度 1** 启动，实际只会持续覆盖 `ADC_Digital_Value[0]` 这一个元素；而真正 256 个采样点是在 `fft_get_adc_value()` 里通过轮询 `AD_Flag` 标志 + `HAL_ADC_GetValue()` 手动读取的。DMA 在这里没有发挥搬运 256 点缓冲的作用，且 `HAL_ADC_ConvCpltCallback` 回调（`AD_Flag=1`）与主循环轮询形成了一种混合但低效的同步方式。

**改进建议（二选一）：**
- **方案 A（推荐）**：DMA 长度设为 `SAMPLING`，一次搬运完整 256 点缓冲，在 DMA 完成回调中置标志，主循环检测标志后直接取整段缓冲做 FFT，CPU 无需逐点轮询。
- **方案 B**：若保持现有逐点读取方式，则无需开启 DMA，直接定时器触发 + 中断读取即可。

### 🟠 6.4 FFT 未加窗函数，存在频谱泄漏

`fft_calculation()` 直接对时域信号做 FFT，没有加窗（如 Hann / Hamming 窗）。当输入频率不是频率分辨率 Δf 的整数倍时，能量会泄漏到相邻频点，导致频谱柱状图“发胖”、峰值幅度偏低。

**改进：** 在 FFT 前对 `ADC_Analog_Value` 乘一个 256 点窗函数（可预先生成常量表），FFT 后按窗的相干增益做幅度补偿。

### 🟡 6.5 显示函数与 u8g2 缓冲模式混用

- 初始化用的是 **全缓冲** 模式（`_f` 后缀），但主循环又用 `u8g2_FirstPage()/u8g2_NextPage()` 这种**分页**模式的写法；
- `display4()` 内部调用了 `u8g2_SendBuffer()`，会在 `display5()` 尚未绘制完成时提前发送一帧不完整画面，之后又被 do-while 循环重复发送，属于冗余/逻辑混乱。

**改进：** 全缓冲模式下直接 `u8g2_ClearBuffer() → 绘制 → u8g2_SendBuffer()` 即可，无需分页循环；`display4()` 内部的 `SendBuffer` 应删除。

### 🟡 6.6 `display5()` 波形幅度缩放与直流偏移问题

```c
cur_adc = fft_Struct.ADC_Digital_Value[i*2]/128;   // 0~4095 → 0~31
```

- ADC 满量程 4095 除以 128 只有 0~31，只占用了屏幕上半部，且无直流偏置去除，波形会被静态偏置顶到固定位置；
- 同时用 `i*2` 隔点取数据，相当于做了 2 倍降采样。

**改进：** 先减去直流分量，再按满量程映射到 0~63 区间。

### 🟡 6.7 `display4()` 峰值频率被强制转整数显示

```c
sprintf(buff,"F:%5d E:%d SF:%d",(uint16_t)MAX_F, ...);
```

`MAX_F` 是 float，直接强转 `(uint16_t)` 会丢失小数位。若需保留精度应改用 `%d.%d` 或 `%.1f` 格式化。

### ⚪ 6.8 冗余/未使用代码

- `GetVoltageMMA()` 函数定义了但**从未被调用**；其中 `MIN_Value=MIN_Value; MAX_Value=MAX_Value;` 两句是无意义的自赋值。
- `oled_driver.c` 中保留了 u8g2 官方 `draw()` 及大量 `testDraw*` 测试函数（未在主循环调用），以及两个大体积位图数组 `bilibili`、`three_support`，会占用较多 Flash，正式项目可清理。
- `HardWare/sys.c`、`sys.h`（正点原子系统文件）中大部分功能未被使用。

### ⚪ 6.9 硬件层面：音频信号无偏置调理电路

STM32 的 ADC 只能测量 **0 ~ 3.3V** 单极性电压，而音频是 ± 交流信号。若直接耦合，负半周会被截掉。实际使用时需要在 PA1 前加**直流偏置电路**（如电阻分压抬升到 1.65V 中心点）与适当的隔直/滤波，否则频谱会出现严重的直流分量与失真。

---

## 7. 总结

| 维度 | 评价 |
|------|------|
| **功能完整性** | ✅ 已实现从采样→FFT→幅值谱→OLED 显示→峰值频率检测的完整链路 |
| **代码结构** | ✅ 分层清晰（HAL / 硬件层 / 显示层），CubeMX 生成 + 用户代码分离 |
| **算法** | ✅ 使用官方 CMSIS-DSP，FFT 幅值归一化处理正确 |
| **实时性** | ⚠️ 受 `printf` 逐帧打印与逐点轮询影响，刷新率偏低 |
| **正确性** | ⚠️ `Mag` 未初始化是必须修复的缺陷 |
| **可维护性** | ⚠️ 存在大量未使用的测试代码与冗余逻辑 |

**优先建议处理顺序：**
1. 修复 `GetPowerMag()` 的 `Mag` 未初始化问题（正确性）；
2. 移除 `fft_get_adc_value()` 中的 `printf`（实时性）；
3. 明确 ADC/DMA 采集方案（方案 A 一次性搬运 256 点）；
4. 统一 u8g2 缓冲模式用法，清理 `display4()` 中的 `SendBuffer`；
5. 增加窗函数、改善波形/峰值显示的缩放。
