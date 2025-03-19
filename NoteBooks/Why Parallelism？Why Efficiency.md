# Why Parallelism？Why Efficiency?

**Why Parallelism？**

因为单颗核(core)的处理器性能增长遇到了瓶颈，若想要继续提高处理器的性能，策略是在处理器中放更多的核(core)。

同时为了让应用程序更高效地执行，我们需要利用好处理器中的多核(core)，即我们需要写并发程序。

**Why Efficiency?**

在多核(core)的处理器上，每一个核(core)并非都被高效利用到了，反而是大部分时间处于空闲中，这时我们说核(core)利用效率低下。

有没有什么具体的量化方法？

将处理器想象成一个正方形，其有一定的面积；将核(core)也想象成一个正方形，其也有一定面积且比处理器更小。

一颗处理器上核(core)越多越好，我们将核(core)放置在处理器上需要占用一定面积，那么在处理器面积固定的情况下我们将核(core)面积做得越小肯定越好。

* 一个非常重要的指标是**单位面积的性能**（performance per area），可用于衡量处理器的效率

* 同时还有能效，**单位能源的性能**（performance per Watt）

<hr>

本次课程主要讲解了多核(core)处理器的历史脉络以及随之发展的并发程序，并同时通过实验告知我们为何有时多核(core)执行任务与单核(core)执行任务的加速比并非理想。

![image-20250211221710786](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250211221710786.png)

此图横坐标为1970~2010年，纵坐标为处理器的功率密度（处理器表面每平方厘米产生的瓦特量）

Intel当时想要榨干单核(core)处理器的性能：

* 处理器更大的带宽：从4bit->64bit
* ILP（(Instruction-Level Parallelism）指令集并行技术
* 处理器更快的时钟频率：从10MHZ->3GHZ

当带宽和ILP都被快榨干时，时钟频率成为了Intel的买点，当时Intel甚至宣称时钟频率 == 速度。

但是正如上图所示，随着Intel不断推出高时钟频率的处理器，处理器的功率密度也越来越大了（处理器产生的热量越来越大），在2000年的处理器上甚至能够煎鸡蛋；图中橙色点为预测点，这些点的热量已经夸张到核反应堆中的热量了。也就是说再以指数形式提高处理器的时钟频率是不太可能了，热量无法散除。

Intel在技术上遇到了瓶颈，转而向让处理器拥有更多的核(core)进发。

![image](http://www.gotw.ca/images/CPU.png)

<center><font size=2>the free lunch is over — herb sutter</center>

> ILP 指令级并行，于《《计算机体系结构：量化研究方法》》一书的第三章重点提及：“大约 1985年之后的所有处理器都使用流水线来重叠指令的执行过程，以提高性能。由于指令可以并行执行，所以指令之间可能实现的这种重叠称为指令级并行(IP)。”

<hr>

若一个任务A交给单核(core)需要花费56s完成，那么任务A交给4个核(core)完成可能需要34s。

为什么？为什么交给4个核(core)完成不是56/4 = 14s?

* 假设任务A包含了16个计算小任务，核(core)a，b，c，d可能分别被分配了1,1,1,13个小任务，如此核(core)d就成为了瓶颈，这被称为负载不均衡
* 想要达到负载均衡，需要执行一定的策略，需要一定额外时间
* 核(core)之前需要通信，有时甚至需要同步（等待他人完成），也需要一定额外时间。（想象一下1个人完成一个简单的小任务并汇报结果可能很快，但是150个人完成150个小任务并汇报结果并非按照预期所想，因为这150个人中每个人大部分时间是不工作的，而是等待）

## Advanced processor Principles: Pipeline, Superscalar and Superpipelined

本知识点参考CSAPP 第四章 处理器体系结构 ，图来自[九曲阑干](https://space.bilibili.com/354767108)

###  Pipeline

先从非流水线化的计算硬件入手：

![image-20250216101441130](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250216101441130.png)

* 组合逻辑电路不存储任何信息，它们只是简单地响应输入信号，产生等于输入的某个函数的输出。

* 时序逻辑电路是有状态且在这个状态上进行计算的系统，我们必须引入按位存储信息的设备。

显然我们非流水线化的计算硬件是时序逻辑电路，引入了时钟寄存器（如图中的Reg）。

> 存储设备都是通过通过一个时钟控制的，时钟是一个周期性信号，决定什么时候将新值加载到设备中。存储设备分为：
>
> * 时钟寄存器
> * 随机访问存储器：如内存，寄存器文件
>
> 时钟寄存器和寄存器文件中的寄存器是不同的概念，分别对应于硬件和机器级编程来说的。
>
> 在硬件中，（时钟）寄存器直接将它的输入和输出线连接到电路的其他部分
>
> 在机器级编程中，寄存器（文件）代表CPU中国为数不多的可直接寻址的字

![image-20250216101302096](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250216101302096.png)

上述图为非流水线计算硬件的电路图表示，大红框中表示时钟寄存器，在其值基础上决定下一条PC的值。

上述非流水线化的计算硬件需要非常慢的时钟周期，因为对于时钟寄存器，需要等到时钟上升沿时才能保存状态。

![image-20250216104704736](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250216104704736.png)

上图红框中表示时钟信号上升沿。

![image-20250216104744200](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250216104744200.png)

上图红框中表示一个时钟周期。

非流水线化的计算硬件要执行完一系列组合逻辑后才能将结果写入时钟寄存器（即完成计算的延时较长），这意味着时钟周期要设计的较长。

<hr>

接下来我们将组合逻辑部分更细地划分为不同阶段（即将延时拆分），并在各个阶段中引入时钟寄存器（流水线寄存器），得到流水线化的计算硬件：

![image-20250216102812053](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250216102812053.png)

流水线计算硬件的电路图表示：

![image-20250216102851385](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250216102851385.png)

从指令流的角度理解流水线工作原理：

![image-20250216103239972](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250216103239972.png)

通常我们说上述五级流水线每一个时钟周期能够执行完成一条指令，因为在流水线“忙”时（如红框中所示），一个时钟周期能够同时完成不同指令的不同阶段，且正好完成了五个阶段。

通常我们会忽略流水线“预热”和“完成执行”时非“忙”的时钟周期。

$吞吐量(IPS instruction per second) = 1条指令 * F（时钟频率） $

$F(时钟频率) = 1s / 时钟周期$

因为我们将组合逻辑部分更细地划分为不同阶段且在各个阶段中引入时钟寄存器，时钟周期可以设计得更短一些。

那么我们岂不是可以不断将组合逻辑部分更细地划分得到更短的时钟周期，从而得到更大的吞吐量？

> * 首先对于硬件设计者来说将系统计算设计划分成一组具有相同延迟的阶段是一个严峻的挑战（若划分成一组延迟差距角度的各阶段那么会有瓶颈，在具有最大延迟阶段上，想想木桶效应）
> * 同时对于处理器中的某些硬件单元，如ALU和内存，不能划分成更多延时较小的单元
> * 流水线过深，收益效果会逐渐下降：注意是因为时钟寄存器也有一定的延时，在中间不断插入时间寄存器也会增加延时
>
> from CSAPP P286

流水线还有**数据冒险**和**控制冒险**问题（CSAPP P295）

* 数据冒险：下一条指令会用到以往指令计算出结果/访存结果，下一条指令在译码阶段若不等待以往指令得出结果会取出错误数据。
* 控制冒险：以往指令的计算结果/访存结果会决定下一条指令的位置，但是我们会通过分支预测先继续执行指令，这可能导致错误。

解决方法:

* 插入气泡（bubble) 暂停（stalling）避免

* 数据转发（旁路）避免

控制冒险需要进一步的考虑，当流水线已经执行了错误分支的指令，我们需要取消这些指令的执行。

案例：

![image-20250216111832166](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250216111832166.png)

对于如上汇编代码，我们从0x000开始执行，我们的分支预测策略为假设运行分支，那么执行时的流水线示意图为：

![image-20250216112004152](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250216112004152.png)

jne 指令需要到Execute阶段才能得到Cnd，然后判断是否需要跳转到target地址执行。

`irmovq $2, %rdx`和`irmovq $3, %rbx`指令都是分支预测时提前执行的指令，当发现分支预测错误后，需要取消这两条指令的执行并将这两条指令从流水线中剔除，具体的做法是：

![image-20250216131551442](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250216131551442.png)

当jne指令执行完Execute阶段后发现分支预测错误，需要在流水线的E时钟寄存器和D时钟寄存器处插入气泡，并接着取出正确分支的指令开始执行。

因为取消这两条指令，我们流水线浪费了两个时钟周期。

![image-20250216152102274](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250216152102274.png)

流水线寄存器含有暂停（stall）信号线和气泡（bubble）信号线。

* 当暂停信号线为1时，时钟寄存器将在时钟信号上升沿时保持其当前状态，可实现指令阻塞在流水线的某个阶段中。

  插入暂停流水线示意图：

  ![image-20250216152741084](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250216152741084.png)

  因为addq指令的数据冒险，我们需要将addq指令阻塞在Decode阶段, 即在D时钟寄存器上连续插入三个时钟周期的暂停。我们浪费了三个时钟周期。

* 当气泡信号线为1时，时钟寄存器将在时钟信号上升沿时，时钟寄存器的状态会设置成某个固定的复位配置，可实现无效化指令后续阶段的执行。

### What is SIMD?

[from slideshare -- Advanced processor Principles](https://www.slideshare.net/slideshow/advanced-processor-principles/96397402?from_search=2)

#### **Flynn's Classification**（弗林分类法）

**Flynn's Classification**（弗林分类法）是计算机体系结构领域中最经典的分类方法之一，由 Michael J. Flynn 在 1966 年提出。它根据指令流（Instruction Stream）和数据流（Data Stream）的多重性对计算机体系结构进行分类，将计算机分为四种基本类型:

#### **(1) SISD（Single Instruction, Single Data）**

- **定义**：  
  单指令流单数据流，即一次执行一条指令，操作一个数据。
- **特点**：  
  - 典型的串行计算机架构。  
  - 每个时钟周期执行一条指令，操作一个数据元素。  
- **示例**：  
  - 传统的单核 CPU（如早期的 Intel 8086）。  
  - 简单的嵌入式处理器。

#### **(2) SIMD（Single Instruction, Multiple Data）**
- **定义**：  
  单指令流多数据流，即一次执行一条指令，但操作多个数据。
- **特点**：  
  - 适用于数据并行任务（如向量运算）。  
  - 通过一条指令同时操作多个数据元素（如数组或矩阵）。  
- **示例**：  
  - GPU（图形处理器）的着色器核心。  
  - Intel 的 SSE/AVX 指令集。  
  - 早期的向量处理器（如 Cray-1）。

#### **(3) MISD（Multiple Instruction, Single Data）**

- **定义**：  
  多指令流单数据流，即多个指令同时操作一个数据。
- **特点**：  
  - 理论上存在，但实际应用极少。  
  - 多个指令流对同一数据进行不同操作，通常用于容错或冗余计算。  
- **示例**：  
  - 某些容错系统（如航天器中的冗余计算单元）。  
  - 实际中 MISD 架构非常罕见，更多是理论上的分类。

#### **(4) MIMD（Multiple Instruction, Multiple Data）**
- **定义**：  
  多指令流多数据流，即多个指令流同时操作多个数据流。
- **特点**：  
  - 支持任务并行和数据并行。  
  - 每个处理器核心可以独立执行不同的指令，操作不同的数据。  
- **示例**：  
  - 多核 CPU（如 Intel Core i7、AMD Ryzen）。  
  - 分布式计算系统（如 Hadoop、Spark）。  
  - 集群和超级计算机。

---

#### 现代架构与 Flynn 分类法

现代计算机通常结合多种 Flynn 分类法的特性：

- **CPU**：  
  - 多核 CPU 是 MIMD 架构，但每个核心可能支持 SIMD 指令（如 AVX）。  
- **GPU**：  
  - 本质上是 SIMD/SIMT 架构，但现代 GPU 也支持一定程度的 MIMD 特性。  
- **异构计算**：  
  - 结合 CPU（MIMD）和 GPU（SIMD）的优势，适用于高性能计算和机器学习。

**Flynn 分类法的扩展**

随着计算机体系结构的发展，Flynn 分类法也被扩展和细化，以涵盖更多现代架构：
- **SPMD（Single Program, Multiple Data）**：  
  - SIMD 的扩展，多个处理器执行相同的程序，但操作不同的数据。  
  - 常见于 GPU 编程模型（如 CUDA、OpenCL）。

- **SIMT（Single Instruction, Multiple Threads）**：  
  - GPU 的编程模型，单指令流多线程，每个线程操作不同数据。  
  - 结合了 SIMD 和 MIMD 的特点。

#### SIMD 和 SSE/AVX

SIMD (Single Instruction, Multiple Data)，即单指令多数据，顾名思义，是通过一条指令对多条数据进行同时操作。

据维基百科说，最早得到广泛应用的SIMD指令集是Intel的MMX指令集，共包含57条指令。**MMX提供了8个64位的寄存器(MM0 - MM7)，每个寄存器可以存放两个32位整数或4个16位整数或8个8位整数**，寄存器中“打包”的多个数据可以通过一条指令同时处理，不再需要分成几次分别处理。

之后，SSE出现了，提供了8个128位寄存器(XMM0 - XMM7)，并且有了处理浮点数的能力。可以同时处理两个双精度浮点数或四个单精度浮点数，或者同时处理四个32位整数或者八个16位整数又或者十六个8位整数。

再后来，又升级了AVX。AVX将SSE的每个128位寄存器扩展到256位，并增加了8个256寄存器。16个256位寄存器称作(YMM0 - YMM15)。再后来Intel又推出了AVX512，把YMM扩展到512位，又新增16个寄存器，共32个512位寄存器(ZMM0 - ZMM31)。

[截取自 -- x40并行编程指南](https://goodcucumber.github.io/x40paraguide/)

### SuperScalar 超标量

[from Superscalar Processor](https://www.slideshare.net/slideshow/superscalar-processor/81332531?from_search=0)

![image](https://image.slidesharecdn.com/advanced-processors-180508142357/75/Advanced-processor-Principles-22-2048.jpg)

Superscalar 处理器的核心思想是 **在一个时钟周期内发射（Issue）并执行多条指令**，而不是传统的单指令发射（如标量处理器）。

Superscalar的核心技术包含：

* **多发射（Multiple Issue）**：每个时钟周期从指令流中取出多条指令，并分派到**不同的执行单元**。
* **寄存器重命名（Register Renaming）**：通过动态分配物理寄存器，消除指令间的 **WAW（写后写）** 和 **WAR（写后读）** 冒险。

* **动态调度（Dynamic Scheduling）**：在运行时检测指令间的依赖关系，并动态调整指令执行顺序，最大化资源利用率。

* **乱序执行（Out-of-Order Execution, OoOE）**：允许指令在不违反数据依赖的前提下乱序执行，以减少流水线停顿。

  ![image](https://image.slidesharecdn.com/advanced-processors-180508142357/75/Advanced-processor-Principles-32-2048.jpg)

![image](https://image.slidesharecdn.com/superscalarseminer-171029073807/75/Superscalar-Processor-6-2048.jpg)

1. **Decode阶段**输出多条解码后的指令，存入Dispatch Buffer。

2. **Dispatch**从Dispatch Buffer中选择**可执行指令**（操作数就绪、目标单元空闲），按策略**分派到保留站（Reservation Stations）或执行单元。**

3. **Reservation Stations** 是每个执行单元（如ALU、FPU）前的小型缓冲队列，用于暂存已分派但尚未执行的指令，并管理操作数的动态就绪状态。

4. 在 **Superscalar流水线** 中，**重排序缓冲区（Re-order Buffer, ROB）**是管理指令乱序执行与顺序提交的核心组件。

   ROB 为每条指令维护一个条目，**状态标记**：

   - **未执行**（Issued）：指令已分派到保留站，等待执行。
   - **已执行未提交**（Executed）：指令执行完成，但结果尚未提交。
   - **已提交**（Committed）：结果已写入架构寄存器或内存。

    **ROB 包含的三类指令**：

   * **Instruction RS（保留站中的指令）**
   * **Instruction executing in FUs（功能单元中执行的指令）**
   * **Instruction finished execution but waiting to be completed in program order（执行完成但等待按序提交的指令）**

   ROB 头部指针始终指向程序顺序最早的未提交指令。仅当头部指令状态为 **已执行未提交** 时，才允许进入提交阶段。

5. Complete 阶段表示指令的 **执行阶段已结束**，即指令在功能单元（如ALU、FPU）中完成了计算或内存访问，结果已经生成。完成后的结果通常写入 **重排序缓冲区（ROB）** 或 **物理寄存器文件**，**但尚未对架构状态（如用户可见的寄存器或内存）生效。**

6. Retire 阶段表示指令的 **结果被正式提交**，即更新架构状态（如用户可见的寄存器或内存），并保证该指令的效果对后续程序可见。

7. Store Buffer 确保存储指令 **按程序顺序提交到内存**（即使它们乱序执行）。

> 超标量在一个指令流中发掘指令级并行(ILP)：在同一指令流中并行执行不同的指令。

# A Modern Multi-Core Processor

## Multi-Core

我们上述介绍的SuperScalar 超标量处理器极大地挖掘和利用了ILP（**Instruction-Level Parallelism，指令级并行性**），但是可以看到为实现SuperScalar我们用硬件实现了许多复杂的机制，最终导致越来越复杂的控制逻辑电路和增大的缓存。

同时ILP提升速度的效果是有限的，困境在于我们只实现了底层指令的并行性，但是程序依旧是按照顺序写的，未考虑到并行计算。

![image-20250216171923149](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250216171923149.png)

一个想法是我们不只是构建一个巨大的“单片”处理器，而是将其分成多个处理器

![image-20250216172106131](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250216172106131.png)

虽然每一个处理器性能比原来的少25%，但是我们现在有两个：$2 * 0.75 = 1.5$，有加速的潜力！

同时经测试，在处理器上能源通常消耗在电路的信息通信中，缩小处理器可减短通信范围，能够省下能源

于是我们可以利用多核（Multi-core）实现并行执行：

* 线程层面的并行执行：在不同核上同时执行完全不同的指令流
* 软件决定何时创建线程
* 实际运用如C语言中的pthread API

> **指令流** 是一个连续的、按照程序顺序排列的指令序列。
>
> 在使用pthread API创建线程执行程序时，可能不同线程执行的程序不同，即使执行相同的程序，执行进度也大概率不相同。

## Data-parallel expression

**sin(x)** 的泰勒展开公式：

$sin(x) = x - \frac{x^3}{3!} + \frac{x^5}{5!} - \frac{x^7}{7!} + \dots$

<span id="sinx"></span>

![image-20250217111805416](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250217111805416.png)

<center>对数组x中的每一个元素实现计算sin(x)并将结果放入result数组</center>

![image-20250217112115176](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250217112115176.png)

<center>data parallel版本的伪代码</center>

因为x[i]之间是无数据依赖的，所以我们希望全部x[i]数据能够并行地执行相同的代码

**硬件上的实现：More ALUs and SIMD**

![image-20250217112545834](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250217112545834.png)

然后我们确实能够使用SIMD指令完善上述伪代码：

![image-20250217113108288](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250217113108288.png)

> SIMD指令运行在一个核中，多个ALUs被相同的指令流控制。

在 **SIMD（Single Instruction, Multiple Data）** 编程模型中，指令的依赖关系在执行之前通常是已知的：

1. **通常由程序员声明**：程序员可以通过编写代码时明确指出哪些操作是独立的，或者哪些数据可以并行处理。
2. **通过循环分析由高级编译器推断**。

<hr>

想一想当我们有多个核并且每个核有多个ALUs，每一个核同时执行相同的指令流，但是处理不同的数据

![image-20250217121049039](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250217121049039.png)

现代CPU和GPU都参考了这一想法，同时**GPU 在设计抛弃了 CPU 中许多复杂的分支预测和逻辑电路（如乱序执行的功能）**

![image-20250217121630579](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250217121630579.png)

对于CPU，上图中描述的是SIMD指令对于分支通过掩码的方式进行处理（隐式处理分歧）

如图中共有8个ALUs，其中3个执行if分支，其余5个执行else分支，那么8个ALUs单元需要先全部执行if分支，这时有5个ALUs空闲；再执行else分支，这时有3个ALUs空闲。

对于GPU也差不多，只是有不同的概念：

**线程束（Warp）模型**

- GPU 将线程分组为线程束（Warp），每个 Warp 包含多个线程（如 NVIDIA GPU 的 Warp 大小为 32 个线程）。
- 同一 Warp 中的线程必须执行相同的指令序列。

 **线程分歧（Divergence）**

- 当 Warp 中的线程遇到分支（如 `if-else`）时，可能会出现线程分歧：

  - 部分线程执行 `if` 分支，另一部分执行 `else` 分支。

- GPU 的处理方式：

  - **串行化执行**：先执行 `if` 分支（其他线程空闲），再执行 `else` 分支（之前执行 `if` 的线程空闲）。

  - **性能损失**：线程分歧会导致部分线程空闲，降低计算效率。

## Hiding stalls with multi-threading

随着时间推移，内存访问的问题并没有得到巨大的进步：

* 内存延时：对于来自处理器的内存请求（如load，store）内存系统处理需要的总时间
  - Example：100cycles，100nesc
* 内存带宽：内存系统提供数据给处理器的速率
  * Example：20GB/s

**Stalls**

* 在指令流中下一条指令因为依赖上一条指令而不能运行导致stalls
* 内存访问是stalls的主要来源

为减少（reduce）Stalls现代CPU已经做了不少工作：

* 通过访问caches减少Stalls

* 预取(prefetching)数据到caches中

我们可以通过在同一个core中多线程交替执行来避免（avoid/hide）stalls

![image-20250218092415868](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250218092415868.png)

如上图当Thread 1遇到Stall，保存其上下文，让Thread 2继续运行.......

> 上下文保存到哪？
>
> core的L1 cache 或者专门的Context storage（在core里面）中
>
> 如上图的右图部分，我们将core中Context storage分成了4个部分(因为这个core有4个hardware threads)， 每一个部分运用存储对应线程的上下文/数据

超线程（Hyper-threading）即运用了上述思想: 在同一核心上同时多路复用多个指令流（simultaneous instruction streams, SMT）

* core管理多线程的上下文

* 每一个时钟，core从多个线程中选择指令运行在ALUs
* 如Intel Hyper-threading , 2 threads per core

**Main Idea**

* benefit: 更高效地利用了core'ALU资源
* Costs：
  * 需要额外的空间存储线程上下文
  * 对内存带宽要求更高了：More threads --> lager working set --> less cache space per thread --> more higher ratio to access memory

## 

![image-20250218101457404](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250218101457404.png)

> 每个核有8个ALUs，即指令流可以同时处理8个不相关的数据片段；
>
> 同时每个核有4个线程，每一个线程运行不同的指令流，所以一个core可以处理32个不相关的数据片段；
>
> 有16个core，则总共可以并行处理512个不相关的数据片段；

再来看看GPU：

![image-20250218102121807](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250218102121807.png)

> 对于NVIDIA GTX 480 core，同样也使用了超线程思想，每个core可以处理48个warp的数据，每一个warp有32个线程，总共有15个core
>
> 那么其可以并行处理48 * 32 * 15 = 23040个数据片段

假设：

![image-20250218103216227](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250218103216227.png)

需要6.4TB/sec的带宽，但是实际上只有177GB/sec，即使GPU具有优越的性能，但是带宽更不上那么只能发挥出其$\frac{177GB/sec}{6.4TB/sec} = 3\%$的效率

所以**Bandwidth is a critical resource**，作为程序员需要尽量让CPU/GPU少访问内存，多计算

# Parallel Programing Abstractions

## SPMD programming abstraction

SPMD（Single Program, Multiple Data，单程序多数据）:

**(1) Single Program（单程序）**

- **定义**：所有处理单元（如MPI进程、GPU线程块）运行**同一份代码**。
- **代码逻辑统一**：代码中可能包含条件分支，通过运行时参数（如进程ID、线程ID）区分不同处理单元的行为。

**(2) Multiple Data（多数据）**

- **数据划分**：全局数据被划分为多个子集，每个处理单元操作不同的数据分区。
- **数据本地性**：处理单元优先访问本地数据，必要时通过通信获取远程数据。

### ISPC

**ISPC**（Intel® Implicit SPC，全称 *Intel® Single Program Compiler*）是由 Intel 开发的一种开源编译器，专门用于编写高性能的 **SIMD（单指令多数据）并行代码**

1. **隐式 SIMD 编程**：开发者无需手动编写 SIMD 内联汇编或 intrinsics，只需编写类似单线程的代码，ISPC 编译器会自动生成优化的 SIMD 指令。
2. **多核并行支持**：通过任务并行模型（如 `launch` 语法）将任务分配到多个 CPU 核心。

**ISPC 的编程模型：Gang 和 Program Instances**

* **Gang（组）**: Gang 是 ISPC 中的基本并行执行单元，表示一组 **并发的 SIMD 程序实例（Program Instances）**。
  * 类似于 GPU 编程中的 **线程束（Warp）**，但运行在 CPU 的 SIMD 硬件单元上。
  * 一个 Gang 中的多个 Program Instances 会共享同一组 SIMD 寄存器，通过单条 SIMD 指令并行处理多个数据元素。

* **Program Instance（程序实例）**: Program Instance 是 ISPC 中的一个逻辑执行单元，每个实例对应处理 SIMD 指令中的一个数据。
  * 比如若使用 AVX2（256 位寄存器），我需要并行处理double类型数组，那么Program Instance个数为$256 / 8 / 8 = 4$

[我们继续以sin(x)的泰勒公式求法为例](#sinx)

![image-20250219161717527](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250219161717527.png)

![image-20250219161736727](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250219161736727.png)

* `uniform` 表示变量在所有 Program Instances 中共享（标量）。

* `programCount` 表示在一个gang中Program Instances的个数
* `programIndex` 表示在一个gang中当前Program Instances的id

这里使用的是AVX/AVX2(256位寄存器)，且数据为float，所以一个gang中有8个Program Instances。

上述sinx.ispc代码会被同时执行在各个Program Instances中，流程示意图为：

![image-20250219162308623](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250219162308623.png)

其中红框中有8个指令执行流，id分别为0,1,2....,7

![image-20250219163539804](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250219163539804.png)

> 在SPMD编程模型中，程序员认为程序运行在programCount个逻辑指令流中，每个逻辑指令流具有不同的programIndex
>
> 这是SPMD编程模型提供的抽象

![image-20250219164222748](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250219164222748.png)

`foreach` 是 ISPC 的并行循环语法，效果和上述等价

## Three parallel programming models and Three machine architectures

### **共享地址空间（shared address space）Model**

* 线程通过读/写共享变量进行交流
* 共享变量如同在公告板（shared address space）上的张贴
* 共享变量通过同步原语（locks,sempahors,etc）来保证同步

![image-20250219165705586](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250219165705586.png)

具体的实现方法有SMP（Symmetric multi-processor 对称多处理器，即处理器到内存的距离均相等），通过直接分享物理内存

![image-20250219170114080](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250219170114080.png)

NUMA（Non-uniform memory access）非统一内存访问

![image-20250219170336087](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250219170336087.png)

缓存一致性保证了本地数据和全局数据的一致

### 消息转发（Message passing）Model

![image-20250219184759711](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250219184759711.png)

现在只有它们自己的私有地址空间，线程直接使用send/receive交换消息

* 不需要任何特殊硬件支持，需要网络
* 适合大型服务器中节点与节点之间沟通

实现方式为在机器上开辟一段共享地址空间：

* sending message  = 将消息从存放消息的地址空间复制到消息库缓存区（message library buffers）
* receiving message = 将消息从消息库缓存区复制到存放消息的地址空间
* 软件实现即可无需硬件实现

### 数据并行（Data parallel）Model

数据并行模型将某个函数或计算映射(map)到一组数据（collection）上。

如Data parallelism in ISPC:

* Think of loop body as function
* foreach construct is a map
* Collection is implicitly defned by array indexing logic

我们需要避免数据竞争和非确定性问题：

![image-20250219192003323](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250219192003323.png)

这里由于程序是同时执行的，可能会有同时写y[i-1]的可能，这就有非确定性问题。

<hr>

流模型：通过流来处理数据并应用纯函数，需要避免数据竞争和非确定性问题。

* 函数需要避免数据竞争和非确定性问题。
* 每一个函数调用的输入和输出都能够提前被知晓，能够通过**预取**数据来hide latency
* 生成者-消费者的地点能够被提前知晓：第一个kernel的输出能够立刻被第二个kernel处理，值保存到core buffer/cache中无需写入内存，可以节省带宽

![image-20250219192954118](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250219192954118.png)





**总结**

在实践中，要充分利用高端机器，通常需要同时使用共享地址空间、消息传递和数据并行等多种编程模型。

总之需要在心中默想：编程模型是什么？硬件是如何实现的？

# Parallel Programing Basics

## Creating a parallel program

### Amdahl’s Law（阿姆达尔定律）

并行程序的宏观思考过程可总结如下：

* 挖掘工作可并行的部分。
* 划分工作。
* 管理数据的方面，沟通，同步。

最初我们计算并行效率可通过如下公式：

$Speedup (Pprocessor) = \frac{Time(1 Processor)}{Time(P Processor)}$

我们能够依据Amdahl’s定律得知$Speedup$是有上限的，定义$S$为不可并行执行占总顺序执行的比例，那么：

$Amdahl’s Law = \frac{1}{s+\frac{1-s}{p}}$

**举例**

先需要对一个$N * N$的图片进行如下两个步骤：

1. 提高图片每一个像素的亮度至两倍
2. 对图片全部像素求平均

我们可以很容易此种方法：对于步骤1无数据依赖完全可并行，对于步骤2可以先P个线程分别并行求和某一块区域，最终将P块区域的和相加，求平均。那么就有下图所示：

![image-20250309213226775](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250309213226775.png)

其中$S = \frac{p}{2^n}$, $Speedup <= \frac{2n^2p}{p^2+2n^2-p}，当n>>p时，Speedup <= 1$

### Parallel Programming Process

![image-20250309214120208](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250309214120208.png)

* **Decomposition(分解)**：将原问题分解为许多子问题（tasks），这个过程需要思考**依赖关系**，最好能够分解出足够的子问题让全部的执行部件处于忙碌状态
* **Assignment(分配)**：将子问题分配给线程进行执行，这个过程需要思考**负载均衡，减少消息传递消耗（communication costs）**
  * 静态分配：硬编码分配方式，指定tasks给线程执行
  * 动态分配：运行时决定线程执行哪些tasks，比如将tasks装入queue，线程执行完task后再从queue中取出task，一般不会有太差的负载均衡

​	![image-20250309220716737](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250309220716737.png)

* **Orchestration(编排)**：需要考虑**组织消息传递，同步，保持较好的局部性**
* **Mapping(映射)**：将线程的工作映射到处理器执行单元上，一般不是程序员需要考虑的事情，一般由**操作系统，编译器，硬件（CPU,GPU）**决定

## A parallel programming example

算法执行Gauss - Seidel sweeps

$Gauss-Seidel \ sweeps更新方式: A[i,j] = 0.2 * (A[i, j] + A[i,j - 1] + A[i - 1, j] + A[i, j + 1] + A[i + 1, j]) $， 数据依赖如下：(回想下[PLCS问题](https://www.cnblogs.com/cilinmengye/p/18548242#psum)，其LCS普通算法数据依赖很像这个)

![image-20250309221953997](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250309221953997.png)

对于这个算法，我们挖掘其中的可并行性，发现对角线上的元素无数据依赖：

![image-20250309222132625](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250309222132625.png)

但是这依旧有许多问题：

* 负载不均衡：每个对角线上需处理的元素个数偏差较大
* 局部性差
* 需要同步

**换个算法吧，我们需要让生活更美好简单一点：红黑排序**

![image-20250309223119928](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250309223119928.png)

![image-20250309223423325](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250309223423325.png)

为什么说要取决于程序运行在哪个机器上呢？

需要注意边界情况，当程序运行在Message passing Model时，因为数据在不同的机器/处理器上，边界数据传递需要开销，那么边界越多开销也越大

# Assignment 1: Performance Analysis on a Quad-Core CPU

## Environment Setup

* CPU信息

  ```
  Architecture:             x86_64
    CPU op-mode(s):         32-bit, 64-bit
    Address sizes:          46 bits physical, 57 bits virtual
    Byte Order:             Little Endian
  CPU(s):                   160
    On-line CPU(s) list:    0-159
  Vendor ID:                GenuineIntel
    Model name:             Intel(R) Xeon(R) Platinum 8383C CPU @ 2.70GHz
      CPU family:           6
      Model:                106
      Thread(s) per core:   2
      Core(s) per socket:   40
      Socket(s):            2
      Stepping:             6
      CPU max MHz:          3600.0000
      CPU min MHz:          800.0000
      BogoMIPS:             5400.00
  ```

*  install the Intel SPMD Program Compiler (ISPC) available here: http://ispc.github.io/

  ```
  wget https://github.com/ispc/ispc/releases/download/v1.26.0/ispc-v1.26.0-linux.tar.gz
  
  tar -xvf ispc-v1.26.0-linux.tar.gz && rm ispc-v1.26.0-linux.tar.gz
  
  # Add the ISPC bin directory to your system path. 
  export ISPC_HOME=/home/cilinmengye/usr/ispc-v1.26.0-linux
  export PATH=$ISPC_HOME/bin:$PATH
  ```

* The assignment starter code is available on https://github.com/stanford-cs149/asst1

## Program 1: Parallel Fractal Generation Using Threads (20 points)

**Is speedup linear in the number of threads used? **

**In your writeup hypothesize why this is (or is not) the case?**

(you may also wish to produce a graph for VIEW 2 to help you come up with a good answer. Hint: take a careful look at the three-thread datapoint.)



![image-20250228150536947](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250228150536947.png)

![image-20250228150547803](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250228150547803.png)



**To confirm (or disprove) your hypothesis, measure the amount of time each thread requires to complete its work by inserting timing code at the beginning and end of `workerThreadStart()`**

VIEW 1:

```
[mandelbrot thread 0]:          [333.949] ms
[mandelbrot thread 1]:          [356.226] ms

[mandelbrot thread 0]:          [131.044] ms
[mandelbrot thread 2]:          [154.097] ms
[mandelbrot thread 1]:          [428.861] ms

[mandelbrot thread 0]:          [62.530] ms
[mandelbrot thread 3]:          [83.256] ms
[mandelbrot thread 1]:          [291.673] ms
[mandelbrot thread 2]:          [292.539] ms

[mandelbrot thread 0]:          [28.683] ms
[mandelbrot thread 4]:          [50.483] ms
[mandelbrot thread 1]:          [193.241] ms
[mandelbrot thread 3]:          [193.720] ms
[mandelbrot thread 2]:          [288.939] ms

[mandelbrot thread 0]:          [17.357] ms
[mandelbrot thread 5]:          [37.675] ms
[mandelbrot thread 1]:          [134.275] ms
[mandelbrot thread 4]:          [134.862] ms
[mandelbrot thread 2]:          [223.723] ms
[mandelbrot thread 3]:          [224.468] ms

[mandelbrot thread 0]:          [13.236] ms
[mandelbrot thread 6]:          [32.660] ms
[mandelbrot thread 1]:          [95.954] ms
[mandelbrot thread 5]:          [97.984] ms
[mandelbrot thread 2]:          [166.271] ms
[mandelbrot thread 4]:          [167.878] ms
[mandelbrot thread 3]:          [214.565] ms

[mandelbrot thread 0]:          [9.987] ms
[mandelbrot thread 7]:          [28.890] ms
[mandelbrot thread 1]:          [75.118] ms
[mandelbrot thread 6]:          [75.412] ms
[mandelbrot thread 2]:          [130.862] ms
[mandelbrot thread 5]:          [131.121] ms
[mandelbrot thread 3]:          [185.480] ms
[mandelbrot thread 4]:          [186.073] ms
```

VIEW 2:

```
[mandelbrot thread 1]:          [177.231] ms
[mandelbrot thread 0]:          [226.604] ms

[mandelbrot thread 2]:          [120.581] ms
[mandelbrot thread 1]:          [128.685] ms
[mandelbrot thread 0]:          [174.056] ms

[mandelbrot thread 3]:          [98.259] ms
[mandelbrot thread 1]:          [101.493] ms
[mandelbrot thread 2]:          [102.176] ms
[mandelbrot thread 0]:          [147.582] ms

[mandelbrot thread 5]:          [68.970] ms
[mandelbrot thread 4]:          [73.143] ms
[mandelbrot thread 2]:          [73.587] ms
[mandelbrot thread 3]:          [76.763] ms
[mandelbrot thread 1]:          [82.289] ms
[mandelbrot thread 0]:          [112.975] ms

[mandelbrot thread 6]:          [59.428] ms
[mandelbrot thread 2]:          [60.442] ms
[mandelbrot thread 4]:          [63.801] ms
[mandelbrot thread 5]:          [66.606] ms
[mandelbrot thread 3]:          [71.558] ms
[mandelbrot thread 1]:          [78.108] ms
[mandelbrot thread 0]:          [100.265] ms

[mandelbrot thread 7]:          [53.217] ms
[mandelbrot thread 5]:          [57.710] ms
[mandelbrot thread 2]:          [57.926] ms
[mandelbrot thread 3]:          [61.531] ms
[mandelbrot thread 4]:          [62.739] ms
[mandelbrot thread 6]:          [63.645] ms
[mandelbrot thread 1]:          [77.445] ms
[mandelbrot thread 0]:          [90.286] ms
```

可以通过对比VIEW 1和VIEW 2在不同线程数执行时各个线程的运行时间上看，VIEW 1具有严重的负载不均衡问题。特别是在VIEW 1下用3个线程执行时，thread 1运行时间居然高达428.861ms是其他线程运行时间的4倍！

这导致VIEW 1下用3个线程运行比用2个线程运行的加速比还要低！

VIEW 2显示出来了较为良好的负载均衡

为什么呢？其实我们可以从PPM图中看出：

![image-20250228163239093](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250228163239093.png)

上图为VIEW 1生成的PPM，按照我的策略，在使用3个线程执行时，从下到上的三个区域分别由thread0,1,2负责.

判断（x,y）坐标是否在mandelbrot集合中是由代码中`static inline int mandel(float c_re, float c_im, int count)`函数进行计算的。当算出（x,y）坐标越“接近”mandelbrot集合中，那么图中在（x,y）坐标上显示地越白。

关键是（x,y）坐标越“接近”mandelbrot集合，在`mandel`函数中迭代得越久（最大为256）。从上图中可以看到VIEW 1 thread1负责的区域相对与thread 0, thread 2有大片的空白，说明thread 1的计算量更大。

![image-20250228164246139](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250228164246139.png)

来看看VIEW 2的图，可以看到白点的分别就均匀许多了。

这里的代码具体参见`mandelbrotThreadV1.cpp`

**Modify the mapping of work to threads to achieve to improve speedup to at about 7-8x on both views of the Mandelbrot set. In your writeup, describe your approach to parallelization and report the final 8-thread speedup obtained.**

```
VIEW 1
[mandelbrot thread 0]:          [334.905] ms
[mandelbrot thread 1]:          [355.082] ms

[mandelbrot thread 0]:          [223.479] ms
[mandelbrot thread 1]:          [244.310] ms
[mandelbrot thread 2]:          [244.273] ms

[mandelbrot thread 0]:          [167.591] ms
[mandelbrot thread 1]:          [188.222] ms
[mandelbrot thread 3]:          [188.149] ms
[mandelbrot thread 2]:          [188.211] ms

[mandelbrot thread 0]:          [134.268] ms
[mandelbrot thread 1]:          [155.675] ms
[mandelbrot thread 4]:          [155.588] ms
[mandelbrot thread 3]:          [155.652] ms
[mandelbrot thread 2]:          [155.684] ms

[mandelbrot thread 0]:          [111.937] ms
[mandelbrot thread 2]:          [132.946] ms
[mandelbrot thread 4]:          [132.864] ms
[mandelbrot thread 1]:          [132.969] ms
[mandelbrot thread 3]:          [132.941] ms
[mandelbrot thread 5]:          [132.888] ms

[mandelbrot thread 0]:          [95.648] ms
[mandelbrot thread 1]:          [116.998] ms
[mandelbrot thread 3]:          [116.925] ms
[mandelbrot thread 2]:          [116.974] ms
[mandelbrot thread 4]:          [116.892] ms
[mandelbrot thread 6]:          [116.812] ms
[mandelbrot thread 5]:          [117.228] ms

[mandelbrot thread 0]:          [85.144] ms
[mandelbrot thread 1]:          [104.262] ms
[mandelbrot thread 4]:          [104.145] ms
[mandelbrot thread 2]:          [104.286] ms
[mandelbrot thread 3]:          [104.250] ms
[mandelbrot thread 7]:          [106.611] ms
[mandelbrot thread 5]:          [106.744] ms
[mandelbrot thread 6]:          [106.666] ms

VIEW 2
[mandelbrot thread 0]:          [191.501] ms
[mandelbrot thread 1]:          [212.256] ms

[mandelbrot thread 0]:          [127.668] ms
[mandelbrot thread 2]:          [149.055] ms
[mandelbrot thread 1]:          [149.279] ms

[mandelbrot thread 0]:          [95.970] ms
[mandelbrot thread 1]:          [115.653] ms
[mandelbrot thread 3]:          [115.783] ms
[mandelbrot thread 2]:          [115.902] ms

[mandelbrot thread 0]:          [76.880] ms
[mandelbrot thread 2]:          [97.456] ms
[mandelbrot thread 1]:          [97.590] ms
[mandelbrot thread 4]:          [97.547] ms
[mandelbrot thread 3]:          [97.708] ms

[mandelbrot thread 0]:          [64.118] ms
[mandelbrot thread 3]:          [83.671] ms
[mandelbrot thread 4]:          [83.687] ms
[mandelbrot thread 2]:          [83.868] ms
[mandelbrot thread 1]:          [84.021] ms
[mandelbrot thread 5]:          [83.885] ms

[mandelbrot thread 0]:          [55.046] ms
[mandelbrot thread 6]:          [75.713] ms
[mandelbrot thread 5]:          [75.799] ms
[mandelbrot thread 4]:          [75.939] ms
[mandelbrot thread 3]:          [76.110] ms
[mandelbrot thread 2]:          [76.357] ms
[mandelbrot thread 1]:          [76.464] ms

[mandelbrot thread 0]:          [48.182] ms
[mandelbrot thread 7]:          [68.148] ms
[mandelbrot thread 6]:          [68.219] ms
[mandelbrot thread 5]:          [68.308] ms
[mandelbrot thread 4]:          [68.495] ms
[mandelbrot thread 3]:          [68.535] ms
[mandelbrot thread 2]:          [68.653] ms
[mandelbrot thread 1]:          [68.736] ms
```

我真的尽量了，写了3种不同的方法对任务分配进行改进：

1. 按照行进行划分区域，然后使用轮转的策略让不同线程负责不同的行，如下：

   | ...  |
   | ---- |
   | 2    |
   | 1    |
   | 0    |
   | 2    |
   | 1    |
   | 0    |
   | 2    |
   | 1    |
   | 0    |

2. 按照行进行划分区域，然后使用轮转的策略让不同线程负责不同的行，但是不按照固定顺序，如下：

   | ...  |
   | ---- |
   | 1    |
   | 0    |
   | 2    |
   | 0    |
   | 2    |
   | 1    |
   | 2    |
   | 1    |
   | 0    |

3. 按照点进行划分区域，然后使用轮转的策略让不同线程负责不同的点，如下

   | ...  |      |      |      |      |      |
   | ---- | ---- | ---- | ---- | ---- | ---- |
   | 0    | 1    | 2    | 0    | 1    | 2    |
   | 0    | 1    | 2    | 0    | 1    | 2    |

但是最终结果在使用8线程V1最多有6.75的加速比，V2最多有6.00的加速比

```python
threadNum = np.array([k for k in range(2, 9)])
speedUpV1 = np.array([1.98, 1.62, 2.42, 2.46, 3.12, 3.26, 3.79])
speedUpV2 = np.array([1.89, 2.43, 2.87, 3.29, 3.74, 4.19, 4.68])
speedUpV1_V1 = np.array([2.00, 2.89, 3.76, 4.55, 5.32, 6.02, 6.67])
speedUpV1_V2 = np.array([1.97, 2.82, 3.58, 4.31, 4.96, 5.54, 6.04])
plt.plot(threadNum, speedUpV1, marker = 'o', label = 'methord1_V1')
plt.plot(threadNum, speedUpV2, marker = 'o', label = 'methord1_V2')
plt.plot(threadNum, speedUpV1_V1, marker = 'o', label = 'methord2_V1')
plt.plot(threadNum, speedUpV1_V2, marker = 'o', label = 'methord2_V2')
plt.xlabel("threadNum")
plt.ylabel("speedUp")
plt.legend()
plt.show()
```

![image-20250301164752273](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250301164752273.png)

methord1是初始未进行改进的方法，methord2是改进后的方法

**Now run your improved code with 16 threads. Is performance noticably greater than when running with eight threads? Why or why not?**

我的结果是如下：

```
6.73x speedup from 8 threads
11.16x speedup from 16 threads
```

因为我的机器有80个核，所以并不会受到核的限制，当增加线程到16时，看起来加速比也提高了1.67倍左右。

但是原作业中只有8核，用16个线程是会启用到超线程的，但是超线程本质上是两个逻辑线程共用同一组计算部件，肯定性能上比一个core上运行一个线程要差。

### BUG1: 浮点数计算的精度问题

**若需精确控制小数位数，应避免直接依赖浮点数，改用高精度库或字符串处理。**

在框架代码中，有检查serial版本和thread版本最终output结果是否相同的判断，但是在浮点数计算中可能遇到浮点数精度问题导致的数值最终不一样的问题，比如框架代码中：
```c++
    for (int j = startRow; j < endRow; j++) {
        for (int i = 0; i < width; ++i) {
            float x = x0 + i * dx;
            float y = y0 + j * dy;
            int index = (j * width + i);

            output[index] = mandel(x, y, maxIterations);
        }
    }
```

serial版本和thread版本中，dy,dx的值分别一样，但是在serial中，当 y0 = -1, j = 601时， 计算出来的y 和 在thread中, 当 y0 = -1 + 600 * dy, j = 1时, 计算出来的y，都结果不一样。

所以thread版本需要让y0,x0以及i，j与serial版本一样才能最保证结果相同。

## Program 2: Vectorizing Code Using SIMD Intrinsics (20 points)

> Run `./myexp -s 10000` and sweep the vector width from 2, 4, 8, to 16. Record the resulting vector utilization. You can do this by changing the `#define VECTOR_WIDTH` value in `CS149intrin.h`. Does the vector utilization increase, decrease or stay the same as `VECTOR_WIDTH` changes? Why?

![image-20250318162651590](https://raw.githubusercontent.com/cilinmengye/Resource-Warehourse/main/CloundIMG/image-20250318162651590.png)

结果看起来是decrease的，首先我们要搞清楚Vector Utilization的计算方式：

$Vector \ Utilization = \frac{Utilized \ Vector \ Lanes}{Total \ Vector \ Lanes}$

$Total \ Vector \ Lanes = Total \ Vector \ Instructions * Vector \ Width$

同时有很多因素导致在一次Vector指令操作时，Vector Lanes不能得到充分利用：

1. 分支判断if
2. 循环while

这些语句总是会导致lane会有停用等待的情况，当Vector Width成倍数增长时，Total Vector Instructions并非成倍数的下降，Utilized Vector Lanes也并非成倍数的上升

