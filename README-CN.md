# Mini Cybernetics Foundation（迷你控制论基础）

**从零开始、零依赖的 C 语言实现**，涵盖控制论的基础理论——从 Wiener 最初的控制与通信愿景，到 Ashby 的必需多样性定律、von Neumann 的自复制自动机、Maturana 与 Varela 的自创生理论，以及 von Foerster 的二阶控制论。每个模块对应 MIT、Stanford、Caltech、Cambridge 和 SFI 的课程，将经典控制论原理转化为可运行的 C 代码。

## 子模块总览

| 子模块 | 主题 | 参考课程 |
|-----------|--------|-------------|
| [mini-ashby-homeostasis](mini-ashby-homeostasis/) | 必需变量、超稳定性、Conant-Ashby 定理、内稳态器模型、多样性理论 | MIT 6.241, Stanford CS229 |
| [mini-autopoiesis-theory](mini-autopoiesis-theory/) | 自催化集合（RAF）、反应网络、化学计量动力学、组织闭合、结构耦合 | MIT 6.S198 |
| [mini-complex-adaptive-system](mini-complex-adaptive-system/) | CAS 智能体、NK 适应度景观、涌现检测、进化计算、交互网络 | MIT 6.883, Stanford MS&E 338, SFI Complexity Explorer |
| [mini-control-philosophy](mini-control-philosophy/) | 目的论与目的性、观测器理论、调节器理论、层级控制、多样性与熵 | MIT 6.241J, MIT 6.832 |
| [mini-second-order-cybernetics](mini-second-order-cybernetics/) | 观测系统、建构主义认识论、递归计算、本征形式、对话理论 | MIT 6.241J, Stanford AA274, ETH 227-0216 |
| [mini-self-organizing-system](mini-self-organizing-system/) | 自催化超循环、元胞自动机、自组织临界性、协同学、图灵斑图 | MIT 6.841, Stanford CS358 |
| [mini-von-neumann-automata](mini-von-neumann-automata/) | 自复制自动机、元胞自动机、容错计算、生命游戏、规则分类 | MIT 6.045, Caltech CS 151, Stanford CS254, Cambridge Part II |
| [mini-wiener-cybernetics](mini-wiener-cybernetics/) | 反馈控制、Wiener 滤波器、信息论、Wiener 预测、Wiener 过程、火控系统 | MIT 6.241J / 6.432, Stanford EE363, Caltech CDS140, Cambridge Control Theory |

## 设计理念

- **零外部依赖** — 纯 C（C99/C11），仅使用 `libc` 和 `libm`
- **模块自包含** — 每个目录自带 `Makefile`、`include/`、`src/`、`examples/`、`demos/`、`tests/`、`docs/`
- **理论到代码的映射** — 每个模块包含 `docs/` 目录，内有九校课程对齐说明和知识层级覆盖
- **基础性控制论** — 覆盖从 Wiener（1948）经 Ashby（1952–1956）、von Neumann（1966）、Maturana & Varela（1973–1980）、von Foerster（1981）到现代复杂性科学的完整思想谱系

## 构建方式

每个模块相互独立。进入模块目录后运行：

```bash
cd mini-ashby-homeostasis
make all    # 构建全部
make test   # 运行测试
```

需要 **GCC** 和 **GNU Make**。

## 项目结构

```
mini-cybernetics-foundation/
├── mini-ashby-homeostasis/           # W. Ross Ashby — 内稳态、超稳定性、必需多样性
├── mini-autopoiesis-theory/          # Maturana & Varela — 自创生、组织闭合、结构耦合
├── mini-complex-adaptive-system/     # Holland、Kauffman — CAS 智能体、NK 景观、涌现
├── mini-control-philosophy/          # Wiener、Rosenblueth、Ashby — 目的论、观测器、调节器、层级
├── mini-second-order-cybernetics/    # von Foerster、Maturana — 观测系统、建构主义、递归
├── mini-self-organizing-system/      # Prigogine、Haken、Bak — 协同学、自组织临界性、图灵斑图
├── mini-von-neumann-automata/        # von Neumann、Conway、Wolfram — 自复制、元胞自动机、容错
└── mini-wiener-cybernetics/          # Norbert Wiener — 反馈、滤波、信息、预测
```

## 许可证

MIT
