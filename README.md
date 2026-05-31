# 命令行伪3D渲染器 - 终极增强版（Wolf3D CLI Engine）

一个完全运行在 **终端（Terminal）** 中的高级伪3D游戏引擎 / Raycasting Engine，灵感来自经典《Wolfenstein 3D》（德军总部3D）。

支持现代化扩展功能，同时保持纯 C 语言实现，无任何外部图形库依赖。

---

## ✨ Features / 特性

### 🎮 核心3D引擎
- DDA Raycasting 实时渲染
- 每一列像素独立射线计算
- 墙体真实距离透视
- 终端伪3D沉浸效果

### 🧠 敌人 AI 系统
- 状态机 AI：
  - IDLE（巡逻）
  - CHASE（追击玩家）
  - ATTACK（近战攻击）
- 自动寻路追踪玩家
- 简单碰撞避免穿墙

### 🔫 武器系统
- 手枪（Pistol）
- 霰弹枪（Shotgun）
- 武器切换（1 / 2）
- 射击冷却系统（Cooldown）
- 弹药系统（Ammo）

### 💥 战斗系统
- 子弹实体（Bullet Entity）
- 命中检测（Collision）
- 敌人 HP 系统
- 击杀与死亡逻辑

### 🎁 拾取系统（Pickups）
- ❤️ 血包（HP +20）
- 🔫 弹药（Ammo +10）
- 自动范围拾取

### 🚪 动态门系统（Doors）
- 开 / 关门动画
- 平滑插值（Animation blending）
- 地图可破坏通行

### 🗺️ 小地图系统（Minimap）
- 实时玩家位置
- 地图网格显示
- 玩家方向标记（@）

### 🌫️ 雾效系统（Fog）
- 距离衰减渲染：
  - `#` 近
  - `O`
  - `o`
  - `.`
  - 空白（远）
- 增强空间深度感

### 🎯 HUD 系统
- HP 生命值
- Ammo 弹药
- Score 分数
- Position 坐标

### 🧱 程序生成地图
- 随机地图生成（Procedural Map）
- 自动封闭边界
- 可扩展房间结构

### ⚡ 性能优化
- 60 FPS 终端渲染（约16ms帧）
- Buffer 渲染机制
- z-buffer 深度控制

---

## 🚀 快速开始（Quick Start）

### 系统要求
- Linux / macOS / WSL
- GCC / Clang
- 支持 ANSI escape terminal

### 编译
```bash
gcc -O3 wolf3d.c -o wolf3d -lm
```

### 运行
```bash
./wolf3d
```

> ⚠️ 推荐终端尺寸：**80 x 40 或更大**

---

## 🎮 控制方式（Controls）

| 按键 | 功能 |
|------|------|
| W / S | 前进 / 后退 |
| A / D | 左右旋转 |
| SPACE | 射击 |
| 1 / 2 | 切换武器 |
| X | 开门 |
| R | 装弹 |
| H | 加血（调试） |
| ESC | 退出游戏 |

---

## 🧠 技术原理（How It Works）

### 📡 Raycasting（核心）
- 每一列屏幕发射射线
- DDA 算法遍历网格
- 计算墙体碰撞距离
- 生成垂直投影高度

### 🎨 渲染流程
1. 清空 buffer
2. Raycast walls
3. 渲染 floor / ceiling
4. 渲染 enemies & pickups
5. HUD overlay
6. 输出到 terminal

### 🤖 AI 逻辑
- 距离判断：
  - 远 → IDLE
  - 中 → CHASE
  - 近 → ATTACK
- 向量归一化移动

---

## 🗺️ 地图系统（Map System）

- `0` = 空地
- `1` = 墙体
- 支持：
  - 动态门
  - 随机生成
  - 碰撞检测

---

## 🔥 Modern Features / 最新升级

✔ 完整 FPS 战斗循环系统  
✔ 敌人 AI 状态机系统  
✔ 双武器系统（手枪 / 霰弹枪）  
✔ 子弹物理系统  
✔ 拾取系统（HP / Ammo）  
✔ 动态门动画系统  
✔ 小地图 HUD 系统  
✔ 雾效距离渲染  
✔ 程序生成地图  
✔ z-buffer 深度渲染优化  
✔ 终端 3D 渲染优化  
✔ 完整实体系统（Enemy / Bullet / Pickup / Door）

---

## 📷 Screenshots / 截图

<img width="952" height="733" alt="图片" src="https://github.com/user-attachments/assets/a0f04cc9-cb38-4545-9fcd-a392767232ee" />
<img width="895" height="718" alt="图片" src="https://github.com/user-attachments/assets/a87796bc-3bde-4fce-acce-2a36117ba645" />
<img width="695" height="752" alt="图片" src="https://github.com/user-attachments/assets/520f4a1c-239e-4191-96a1-fd6d5d149487" />

---

## 🙏 致谢（Credits）

- [Lode's Computer Graphics Tutorial](https://lodev.org/cgtutor/raycasting.html) – 射线投射经典教程
- id Software — Wolfenstein 3D 灵感来源
- Retro FPS & Terminal Gaming 社区
- C Language / Unix Terminal hackers

---


## ⚡ Note

This project is fully terminal-based.
No GPU. No engine. Only math + C + imagination.
