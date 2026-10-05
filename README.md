# Smart-Car — MSPM0G3507 智能小车

基于 **TI MSPM0G3507** (Cortex-M0+) 的智能小车固件，运行于 LP-MSPM0G3507 LaunchPad 开发板。

## 硬件平台

- **MCU**: MSPM0G3507 (Cortex-M0+, 32MHz)
- **开发板**: LP-MSPM0G3507 LaunchPad
- **电机**: 直流减速电机 × 2（带霍尔编码器）
- **驱动**: TB6612 / 同类 H 桥电机驱动
- **通信**: XDS110 虚拟串口 (115200-8N1)

## 工程结构

```
├── empty.c                  # 主程序入口，SysTick 20ms 控制循环
├── empty.syscfg             # SysConfig 外设配置
├── app/
│   ├── car.c / car.h        # 上层控制逻辑
│   ├── car_command.c/.h     # UART 指令解析器
│   ├── car_debug.c/.h       # 运行状态诊断
│   ├── chassis.c/.h         # 底盘速度闭环
│   └── telemetry.c/.h       # 遥测数据上报
├── bsp/
│   ├── encoder.c/.h         # 正交编码器解码 (4x)
│   ├── motor.c/.h           # PWM 电机控制
│   ├── pid.c/.h             # PI 控制器
│   └── speed_measure.c/.h   # 速度测量
├── targetConfigs/           # CCS 调试目标配置
└── Debug/                   # SysConfig 生成 + 编译输出
```

## 指令集

通过串口发送指令控制小车：

| 指令 | 功能 | 示例 |
|------|------|------|
| `F` | 前进 | `F` |
| `B` | 后退 | `B` |
| `L` | 左转 (原地) | `L` |
| `R` | 右转 (原地) | `R` |
| `S` | 停止 | `S` |
| `1` / `2` / `3` | 速度档位 (低/中/高) | `2` |
| `V <左目标> <右目标>` | 独立轮速控制 | `V 16 -16` |
| `D <计数值>` | 距离闭环 (直线) | `D 500` |
| `T <计数值>` | 角度闭环 (差速转向) | `T 200` |

### 速度档位

| 档位 | 计数值/20ms | 说明 |
|------|-------------|------|
| 1 - LOW | 8 | 默认档 |
| 2 - MEDIUM | 12 | |
| 3 - HIGH | 16 | |

### 遥测输出格式

```
{B左目标:左反馈:左输出:右目标:右反馈:右输出:错误码:状态}$
```

**状态码**: 0=STOP, 1=TURN_RIGHT, 2=TURN_LEFT, 3=BACKWARD, 4=FORWARD

**错误码** (位域):
- Bit 0: 左轮无速度
- Bit 1: 右轮无速度
- Bit 2: 左轮方向错误
- Bit 3: 右轮方向错误
- Bit 4: 左右速度不平衡

## 闭环控制

- **速度 PI**: KP=12, KI=2, 积分限幅 ±120
- **目标软启动**: 每 20ms 步进 +2，避免急加速
- **通信超时**: 5 秒 (250 周期) 无指令 → 自动停车
- **距离闭环**: 编码器累计平均值达目标值停止
- **角度闭环**: 左右轮编码器差分达目标值停止

## 构建 & 烧录

### 方式一：CCS IDE

1. 导入工程: **Project → Import CCS Projects**
2. 构建: **Project → Build Project**
3. 调试: **Run → Debug**

### 方式二：命令行 (需 w64devkit)

```bash
make -C Debug all
```

### 烧录

使用 CCS Debug 或 DSLite：

```bash
DSLite.exe flash -c targetConfigs/MSPM0G3507.ccxml Debug/empty_LP_MSPM0G3507_nortos_ticlang.out
```

## 串口调试

- 波特率: 115200
- 数据位: 8
- 停止位: 1
- 校验: 无
- COM 口: 连接 LaunchPad 后自动枚举 (XDS110 Class Application/User UART)

## License

详见 TI 例程许可。