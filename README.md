# SB-LTL (Spot Based LTL Checker)

<a href="README.md">中文</a> | <a href="README.en.md">English</a>

## introduction

- 一个基于 [Spot](https://spot.lre.epita.fr/index.html) 的运行时验证 LTL 性质的小工具
- 主要是检测输入输出的 events 对应的 trace 是否能被 buchi 接收
- 目前该程序面向的被测程序是是：每轮一次输入，一组输出，交替进行
- 目前还比较简陋，还未实现对 c 程序的插桩，只能手动填入所有程序的输入输出 events
- 我个人的规范是：如输入 4,则 `event` 表示为`i4`，输出 7,则`event`表示为`o7`
- 该程序认为 `printf(xx)` 是命题，而一个自动机的状态是多个命题的布尔组合，如 `state_1=i1,state_2=[o2,o3,o4]`。这个需要测试人员手动在待测代码中在程序输入的位置插桩代码`printf("ixx")`，在关键的代码下一行插桩`printf("oxx")`。而插桩代码需要在一次输入后收集`i event`，并收集两次输入之间的所有`o event`
- 本项目是一个学习 LTL 的项目，所以可能有疏漏的地方，欢迎指正

## install

- 只有 spot 和 g++两个依赖
- spot 的安装不能参考教程，如下

  ```
  # 这个官方的貌似有点问题
  # wget -q -O - https://www.lrde.epita.fr/repo/debian.gpg | apt-key add -
  # echo 'deb http://www.lrde.epita.fr/repo/debian/ stable/' >> /etc/apt/sources.list

  # 参考下面这个issue的下载方式
  # https://gitlab.lre.epita.fr/spot/spot/-/issues/544
  echo 'deb [trusted=true] https://download.opensuse.org/repositories/home:/adl/xUbuntu_22.04/ ./' >/etc/apt/sources.list
  apt update
  apt upgrade
  apt install spot libspot-dev
  ```

3. 具体的参考 Dockerfile

## run

```
chmod +x build.sh
chmod +x run.sh
# 打包成镜像
docker build -t sb-ltl:urlyy .
# 挂载项目代码并运行
docker run --rm -v /home/urlyy/桌面/ltl-fuzzer/SB-LTL:/app -it sb-ltl:urlyy
# 在容器内运行
./run.sh
```

# Example

- LTL : `F(i1 & X(o2 & !o3))`
- events : `{{"i1"},{"o2","o3"},{"i1"},{"o2","o4"},{"i1"},{"o2","o3"}}`
- output :

  ```
  ==================== formula to automata
  LTL is F(i1 & X(o2 & !o3))
  ==================== ap in LTL
  i1: 0
  o2: 5
  o3: 6
  ============================= Automata
  Initial state: 1
  State 0:
    edge(0 -> 0)
      label = 1
      acc sets = {0}
  State 1:
    edge(1 -> 1)
      label = !i1
      acc sets = {}
    edge(1 -> 2)
      label = i1
      acc sets = {}
  State 2:
    edge(2 -> 0)
      label = o2 & !o3
      acc sets = {}
    edge(2 -> 1)
      label = (!i1 & !o2) | (!i1 & o3)
      acc sets = {}
    edge(2 -> 2)
      label = (i1 & !o2) | (i1 & o3)
      acc sets = {}
  accept state ID: { 0, }
  ============================= Check
  cur_state_id: 1 , cur_event: [i1,]
  1->1 trans:!i1
  cond_check: (i1:0)

  1->2 trans:i1
  cond_check: (i1:1)

  cur_state_id: 2 , cur_event: [o3,o2,]
  2->0 trans:o2 & !o3
  cond_check: (o2:1)
  cond_check: (o3:0)

  2->1 trans:(!i1 & !o2) | (!i1 & o3)
  cond_check: (o2:0)

  cond_check: (o2:1)
  cond_check: (o3:1)

  cur_state_id: 1 , cur_event: [i1,]
  1->1 trans:!i1
  cond_check: (i1:0)

  1->2 trans:i1
  cond_check: (i1:1)

  cur_state_id: 2 , cur_event: [o4,o2,]
  2->0 trans:o2 & !o3
  cond_check: (o2:1)

  cur_state_id: 0 , cur_event: [i1,]
  0->0 trans:1
  cur_state_id: 0 , cur_event: [o3,o2,]
  0->0 trans:1
  final_state_id: 0
  accept: true
  ```

## TODO

- 解决 prefix 接收即判断接收的方法
- 在另外一个项目中基于本项目，结合 插桩+fuzz 进行真正的运行时验证(即类似 ltlfuzzer 项目一样的效果，但脱离 afl 专注 LTL)（难度较大，不考虑这个了）
