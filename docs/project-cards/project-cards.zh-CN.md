# 项目卡总览

## 01 板载LED周期闪烁

**呈现效果**

板载LED亮0.5秒、灭0.5秒，并一直循环运行。

**技术实现与新知识**

通过DDRB将PB7设为输出，循环改写PORTB电平，并用_delay_ms设置500ms亮灭间隔，完成寄存器级GPIO输出、位操作和最小AVR C工程的构建与运行。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/01_bare_metal_blink)

## 02 板载灯与外接LED交替闪烁

**呈现效果**

板载LED先亮0.2秒、灭0.2秒，再由外接黄色LED亮0.2秒、灭0.2秒。两盏灯按照这个顺序一直循环运行。

**技术实现与新知识**

将PB7与PB4同时配置为输出，使用位掩码分别控制板载LED与外接LED，以200ms延时组织交替亮灭，练习同一端口上的多引脚独立控制。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/02_external_led)

## 03 指定次数与速度的LED闪烁

**呈现效果**

LED先快速闪烁3次，熄灭停顿1秒，再慢速闪烁2次，最后保持熄灭。可以修改闪烁次数和每次亮灭的时间，得到不同的灯光组合。

**技术实现与新知识**

将延时与闪烁封装为wait_ms和blink函数，通过次数、间隔参数复用输出逻辑，用const和定宽整数类型组织快闪、停顿、慢闪的顺序执行。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/03_blink_function)

## 04 三色灯顺序循环

**呈现效果**

红、绿、蓝三盏灯依次亮起，每盏灯亮起后有一小段全部熄灭的间隔，然后轮到下一盏。三种颜色按相同顺序一直循环；修改各步骤的时间和灯光设置，就能改变整个演示。

**技术实现与新知识**

用状态数组记录三路LED在各阶段的输出，用时长数组保存50ms亮灯与20ms全灭间隔，再通过for循环索引执行，形成状态与时序分离的表驱动程序。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/04_traffic_light)

## 05 按钮控制LED与稳定输入

**呈现效果**

按住按钮时LED亮起，松开后LED熄灭。程序持续区分按钮的按下、松开状态，按钮没有被按下时，输入也保持明确状态。

**技术实现与新知识**

PB4配置为内部上拉输入，按下为低电平；读取PINB后控制PB5输出，实现有效低按钮到LED的电平映射，涉及输入方向、上拉与逻辑取反。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/05_button_hold_led)

## 06 按一次切换一次灯光

**呈现效果**

第一次按下按钮时LED亮起，再按一次时熄灭。持续按住按钮不会让程序不断切换灯光；这个阶段尚未过滤按钮接触时的短暂抖动。

**技术实现与新知识**

保存上一轮按钮状态并与当前读数比较，仅在下降沿翻转LED，区分持续电平与离散事件；该版本尚未加入机械消抖。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/06_button_toggle_led)

## 07 按钮选择四种灯光行为

**呈现效果**

连续按键可以依次选择熄灭、常亮、快速闪烁和慢速闪烁四种行为。快速和慢速模式分别执行一组闪烁后熄灭，再按键进入下一种行为。

**技术实现与新知识**

以mode保存当前模式，在按键下降沿递增并循环归零，通过switch/case选择熄灭、常亮及两组不同延时的闪烁行为，实现状态驱动的输出控制。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/07_button_statemachine)

## 08 用闪灯次数显示按钮抖动

**呈现效果**

按下一次按钮后，程序记录短时间内按钮信号变化的次数。松开按钮后，LED按记录的次数闪烁，用灯光显示一次按压内部出现了多少次信号变化。

**技术实现与新知识**

检测到按下后，在100次、每次间隔50μs的采样窗口中累计电平变化，释放后用LED闪烁次数显示结果，练习机械抖动采样与事件计数。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/08_measure_button_bounce)

## 09 过滤按钮重复触发

**呈现效果**

按钮按下后，程序等待输入稳定再确认操作。第一次有效按下点亮LED，下一次有效按下熄灭，减少一次按压被识别成多次操作的情况。

**技术实现与新知识**

分别维护原始读取、候选状态和确认状态，连续稳定计数达到阈值后才提交状态变化，再用确认事件切换LED，建立逐次消抖与边沿计数逻辑。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/09_nonblocking_debounce_fsm)

## 10 短按与长按触发不同闪灯

**呈现效果**

短按按钮并松开后，LED快速闪烁3次；按住较长时间再松开，LED慢速闪烁5次。使用同一个按钮即可选择两种不同的灯光响应。

**技术实现与新知识**

在消抖后的按下状态中累计持续时间，释放时按阈值选择快闪或慢闪函数，将按键生命周期、时间判定和输出动作分离。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/10_short_long_press)

## 11 两个按钮分别控制两盏灯

**呈现效果**

两个按钮各自对应一盏LED，按键可分别切换对应灯的亮灭。两盏灯保存各自状态，可以交替操作，也可以让两盏灯处于不同的亮灭组合。

**技术实现与新知识**

为两个输入分别维护消抖计数、确认状态和按下次数，再独立控制PB6、PB7，扩展单路状态处理，避免多输入之间共享变量造成干扰。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/11_dual_button_dual_led)

## 12 电脑查看按键事件与次数

**呈现效果**

开发板启动时，电脑显示启动消息。操作两个按钮时，电脑分别显示对应的按钮事件和累计按下次数，同时保留两盏LED的控制功能。

**技术实现与新知识**

配置USART0发送通道与波特率，将启动消息、按键事件和计数编码为ASCII输出，结合GPIO控制练习串口调试与整数到十进制文本的转换。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/12_uart_tx_debug)

## 13 电脑命令控制两盏LED

**呈现效果**

在电脑上输入on_1、off_1、on_2或off_2，可以分别开关两盏LED。输入status查看当前亮灭，输入help查看命令；程序还处理退格，并对过长或无法识别的命令给出提示。

**技术实现与新知识**

以有界字符缓冲区接收UART命令，处理退格、换行和超长输入，再通过字符串比较执行LED控制与状态查询，形成基本命令解析器。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/13_uart_rx_command)

## 14 电脑命令开关蜂鸣器

**呈现效果**

在电脑上发送on命令使蜂鸣器发声，发送off命令停止发声。输入的内容会回传到电脑，过长的命令会显示错误提示。

**技术实现与新知识**

复用UART行接收与长度检查逻辑，识别on/off命令后设置蜂鸣器引脚电平，并回传输入，完成文本命令到有源蜂鸣器输出的映射。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/14_buzzer_square_wave)

## 15 灯光闪烁时继续检测按钮

**呈现效果**

LED约每隔1秒切换一次亮灭。灯光运行期间，仍可按下或松开按钮，电脑会显示状态变化消息，不需要等整段闪灯结束后才识别按钮。

**技术实现与新知识**

主循环交替推进计时、LED周期检查与按钮消抖，根据时间差更新输出并发送状态事件，替代整段阻塞闪烁，形成协作式任务结构。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/15_nonblocking_multitask)

## 16 硬件计时控制灯光

**呈现效果**

LED按约1秒亮、1秒灭的节奏持续闪烁。灯光的切换依据开发板持续记录的时间进行。

**技术实现与新知识**

Timer1采用CTC模式，通过预分频与OCR1A配置1ms比较匹配中断；ISR更新system_ms，主循环原子读取计时快照后调度LED。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/16_timer_1ms_tick)

## 17 持续显示系统运行时间

**呈现效果**

LED每隔1秒切换状态，电脑同时重复显示SYSTEM ALIVE和当前运行的毫秒数。运行时间不断增加，可用来观察装置是否仍在工作以及数值输出是否正确。

**技术实现与新知识**

以volatile uint32_t维护ISR更新的毫秒计数，通过ATOMIC_BLOCK取得一致快照，再转换为十进制文本输出，涉及中断共享、多字节原子性与数值类型宽度。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/17-isr-volatile-atomicity)

## 18 灯光、按键与定时报告同时运行

**呈现效果**

LED每隔0.5秒切换一次状态，按钮按下次数持续累计。电脑约每隔2秒报告当前时间、LED亮灭和按钮计数，操作按钮时这些功能仍继续运行。

**技术实现与新知识**

共用1ms系统节拍，为LED、串口报告和按钮消抖分别维护时间戳，实现不同周期的协作式调度，并用uint32_t计数与串口状态输出观察运行。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/18_fixed_tick_tack_scheduler)

## 19 电机脉冲与周期换向信号

**呈现效果**

开发板持续输出电机步进信号，并约每隔5秒切换方向。电脑约每隔4秒输出一条运行消息；不连接电机时，也可以测量输出信号。

**技术实现与新知识**

Timer1通过输出比较产生STEP脉冲，Timer3维护系统时基，主循环分别安排DIR换向与运行报告，使硬件脉冲产生与软件周期任务并行推进。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/19_step_dir_pulse_generator)

## 20 步进电机首次转动

**呈现效果**

通过驱动器和独立电源连接步进电机，让它按开发板信号转动，并观察改变方向信号后的转向变化。这个实验沿用项目19的工程。

**技术实现与新知识**

按原实验卡将STEP/DIR接入外部限流驱动器，并核对相电流、独立供电与共地，完成逻辑控制到步进电机驱动的连接；实验沿用19工程。

实物实验：沿用19工程，仓库没有独立的20目录。

## 21 运动中接收电脑命令

**呈现效果**

电脑可以启动或停止电机、修改运行速度、切换方向，并查询运行状态。电机输出继续进行时，程序仍接收新的命令；电脑也会定期显示速度、方向和是否正在运行。

**技术实现与新知识**

硬件维持STEP输出，主循环采用持久字符缓冲逐步接收命令，调用电机模块执行启停、调速与换向，并周期报告状态，避免命令等待阻塞运动控制。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/21_nonblocking_stepper_control)

## 22 电机加减速与自动演示

**呈现效果**

电机启动后逐渐加速，达到设定速度后保持运行，停止时再逐渐减速。电脑可以修改速度和加速度，并显示阶段变化和速度记录；自动演示会完成加速、运行一段时间、减速停止的流程。

**技术实现与新知识**

以运动状态机按时间更新步频斜坡，区分加速、匀速和减速阶段；演示任务根据状态切换与计时发出停止请求，日志记录速度和阶段变化。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/22_stepper_acceleration_ramp)

## 23 微步、负载与温升比较

**呈现效果**

在不同微步设置和负载下运行电机，比较运动平顺度、声音和发热情况。实验记录应同时列出速度、负载、温度以及是否出现失步，便于比较不同设置。

**技术实现与新知识**

原实验卡通过调整微步、电流与负载，对照记录速度、温升和失步情况，比较驱动参数对平顺度与负载能力的影响；仓库未提供独立实验工程。

实物实验：依据原项目卡，仓库没有独立的23目录。

## 24 电机回零、限位与急停

**呈现效果**

电机可以自动寻找起点，碰到开关后退离，再慢速接近确认位置。运行中触发边界开关或急停输入时，程序停止运动并显示状态；回零时遇到开关异常或找不到起点，也会报告故障。

**技术实现与新知识**

将限位、ESTOP与回零分为独立模块，在中断保护检查中停止脉冲，并以回零状态机处理寻找、退离、慢速定位及超时，建立异常路径与故障状态。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/24_stepper_limits_homing_estop)

## 25 定位与重复往返控制

**呈现效果**

完成回零后，可以从电脑指定目标位置，让电机移动到该位置；也可以设置两点和重复次数，执行往返任务。电脑能查询运动状态、配置和日志，运动期间仍可处理停止、限位和急停。较长的帮助文字分批发送，避免输出消息拖住其他工作。

**技术实现与新知识**

以位置、往返、回零和安全任务组织运动流程，报告模块保存待发送请求并按TX缓冲余量分批输出，避免长消息阻塞主循环与减速更新。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/25_stepper_safe_system_integration)

## 26 氨气数据接收与状态显示

**呈现效果**

电脑可以查看传感器发来的原始数据、解析后的浓度，以及预热、有效或数据过期等状态。接收到错误或不完整数据时，程序记录错误；已有读数变旧时会标明，传感器读取与运动任务分别继续处理。

**技术实现与新知识**

独立UART按帧头、长度、校验与字段校验提交完整浓度，维护数据年龄和预热状态。协议层与接收任务分离；当前默认ZE03，ZE801分支仍待协议确认。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/26_ze801_nh3_uart_sensor)

## 27 固定亮度与呼吸灯

**呈现效果**

LED可以关闭、保持指定亮度，或持续渐亮渐暗。电脑可以调整呼吸灯的周期，并查询当前亮度和模式；灯光运行时，电机和串口任务仍继续处理。

**技术实现与新知识**

Timer4硬件PWM维持输出，LED任务根据系统时间更新占空比，实现固定亮度与可调周期呼吸模式，将载波产生与亮度包络更新分离。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/27_nonblocking_pwm_breathing_led)

## 28 旋钮调光与电压显示

**呈现效果**

选择旋钮控制模式后，转动旋钮就能改变LED亮度。电脑可以同时显示原始读数、换算电压、数据更新时间和对应亮度，便于观察旋钮变化。

**技术实现与新知识**

周期读取电位器ADC结果，以参考电压换算毫伏并映射PWM占空比，同时保存采样时间与有效标记，将模拟采集、数据状态与调光输出分离。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/28_potentiometer_led_brightness)

## 29 两路输入读取与调光来源切换

**呈现效果**

电脑可以同时查看两路输入的数值、电压和更新时间，并选择由哪一路控制LED亮度。切换来源后，灯光跟随新选中的输入，另一通道仍保留读取记录。

**技术实现与新知识**

共享ADC通过MUX轮换扫描通道并保存独立采样快照，调光模块选择指定来源，处理通道切换与数据有效性，扩展为多路采集结构。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/29_adc_register_dual_channel)

## 30 旋钮读数平滑与范围校准

**呈现效果**

电脑可以对照查看原始读数、平均后的读数、校准结果和最终亮度。可以调整平滑程度、亮度变化门槛和旋钮两端的有效范围，观察灯光抖动减少了多少，以及转动后响应是否变慢。

**技术实现与新知识**

每路输入分别维护滑动平均窗口、校准端点和滞回阈值，经过校准与映射后更新亮度保持值，比较滤波稳定性与阶跃响应延迟。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/30_adc_filter_hysteresis_calibration)

## 31 环境光自动控制LED

**呈现效果**

选择自动模式后，环境变暗时灯亮，环境变亮时灯灭。开灯与关灯使用不同条件，光线在交界处轻微变化时不必反复切换；电脑还能查看光线读数、开关设置和切换次数。

**技术实现与新知识**

对光敏输入进行平均与归一化，根据不同开关阈值执行滞回控制，并检查极性与数据新鲜度，将传感器采样接入非阻塞自动灯光任务。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/31_light_sensor_auto_led)

## 32 蜂鸣器音阶、旋律与单音

**呈现效果**

电脑可以让蜂鸣器播放一个指定音高和时长的声音、完整音阶或预设短旋律，也可以中途停止。播放过程中仍能处理其他任务；状态信息显示正在播放的音符、频率和剩余时间。

**技术实现与新知识**

定时器产生指定频率方波，PROGMEM音符表保存频率与时长，播放状态机按时间切换音符、休止与间隔，使音乐节奏调度独立于声波产生。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/32_nonblocking_buzzer_notes)

## 33 外部事件计数与接收方式比较

**呈现效果**

电脑同时显示两种接收方式记录的外部信号次数、每秒事件数量以及两种计数的差值。按钮模式还单独统计有效按压次数；输入信号变快时，可以观察是否出现漏记。计数可以启动、停止和清零。

**技术实现与新知识**

INT5与主循环轮询同时累计同一输入的事件，周期计算速率与计数差；按钮模式另做消抖，结合原子快照比较边沿捕获与轮询的接收能力。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/33_external_interrupt_event_counter)

## 34 超声波距离与接近提醒

**呈现效果**

电脑显示前方物体的距离、上次测量时间和测量状态。可以比较原始、平均和中值处理后的距离；物体接近设定范围时，电脑输出接近提醒。收不到回声、回声超时或信号异常时，会报告对应状态。

**技术实现与新知识**

以状态机推进触发、等待回波上升沿和下降沿，测量脉宽并换算距离，加入超时、均值/中值滤波及滞回提醒，区分测量错误与有效距离变化。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/34_nonblocking_ultrasonic_distance)

## 35 连接设备地址扫描

**呈现效果**

电脑逐步列出通信线路上有回应的设备地址，并显示扫描进度、找到的设备数量和用时。扫描可以取消；出现无回应、等待超时或线路被拉低等情况时，可以查看对应统计和错误信息。

**技术实现与新知识**

I²C状态机逐个探测7位地址，根据ACK/NACK记录设备，分步处理START、地址与STOP事务，并统计超时、总线错误、取消与重试。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/35_nonblocking_i2c_scanner)

## 36 MPU-6050身份与三轴加速度读取

**呈现效果**

电脑可以查看传感器身份信息，以及X、Y、Z三个方向的加速度读数。可以单次读取或连续采样，并查看数据是否仍然新鲜。读取出错时保留上一份完整结果、标注数据年龄并尝试恢复；连续失败达到上限后停止重试并报告。

**技术实现与新知识**

依次完成MPU-6050身份核对、复位、配置读回和三轴采样，只提交完整结果；采用有限重试与退避，并仅在有效采样后清除连续失败次数。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/36_nonblocking_i2c_sensor_registers)

## 37 SPI收发核对与电压输出

**呈现效果**

电脑可以启动多组收发测试，比较发出的数据与接回的数据，显示通过、差异或错误统计。连接MCP4921电压输出模块后，可以发送目标数值，并查看最后发送值和理论电压。测试与电压设置共用通信接口，程序会协调使用顺序。

**技术实现与新知识**

SPI环回任务分段发送测试模式并逐字节核对，DAC任务发送MCP4921控制字，两者通过总线所有权协调；区分收发自检、传输完成与模拟输出实测。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/37_nonblocking_spi_loopback_dac)

## 38 OLED多页运行界面

**呈现效果**

装置的小屏幕可以切换运动、传感器和系统三个页面。页面分别显示电机位置与速度、距离和加速度等读数，以及计数和通信状态；急停、限位、数据过期或设备失败会有提示。画面按变化更新，显示通信出错时，电脑输出重试信息。

**技术实现与新知识**

SSD1306界面从状态快照逐行渲染至帧缓冲，以脏区域追踪和分步I²C传输完成局部刷新，并用有限次退避重试处理显示故障，约束刷新工作量。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/38_nonblocking_oled_dashboard)

## 39 舵机角度、速度与旋钮跟随

**呈现效果**

电脑可以设置舵机目标角度和转动速度，也可以选择让目标角度跟随某一路旋钮。程序逐步改变给出的角度，不是瞬间跳到新设定。急停或旋钮数据失效时会停止控制信号，并在电脑上说明原因，需要新的命令才能恢复。

**技术实现与新知识**

将角度映射为舵机脉宽，按固定时间步长限速更新命令，同时检查ESTOP与ADC数据年龄；异常时停止脉冲，明确目标角度、输出命令与实际反馈的区别。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/39_nonblocking_servo_control)

## 40 自动往返与整机运行管理

**呈现效果**

先让电机回零，再从电脑设置A、B两个位置和往返次数。启动后先到A点，随后按A到B再回A的顺序完成规定次数，最后停在A点。电脑显示当前阶段、已完成次数、目标和按步数记录的位置，并保存运行结果；正常完成、手动停止和故障结束有不同说明。自动任务与手动操作受到协调，运行中不能随意改计划。传感器、显示和其他模块仍各自运行，系统汇总它们的状态。

**技术实现与新知识**

整机状态机协调回零、计划设置、定位和自动往返，任务仲裁约束手动/自动操作，runlog保存进度与结束原因；各外设任务独立推进并由系统层汇总状态。

[查看对应代码](https://github.com/yuxuan-518/atmega2560-embedded-foundations/tree/main/code/40_mega_integrated_control_system)
