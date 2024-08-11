# SB-LTL (Spot Based LTL Checker)

<a href="README.md">中文</a> | <a href="README.en.md">English</a>

## Introduction

- 一个基于 [Spot](https://spot.lre.epita.fr/index.html) 的运行时验证 LTL 性质的小工具
- 主要是检测输入输出的 events 对应的 trace 是否能被 buchi 接收
- 目前还比较简陋，还未实现对 c 程序的插桩，只能手动填入程序的产生的一系列事件
- 本项目是一个学习 LTL 的项目，所以可能有疏漏的地方，欢迎指正

## Install

环境：Ubuntu22.04

- 只有 spot 和 g++两个依赖
- spot 的安装不能参考教程，如下

  ```
  # 这个官方的貌似有点问题
  # wget -q -O - https://www.lrde.epita.fr/repo/debian.gpg | apt-key add -
  # echo 'deb http://www.lrde.epita.fr/repo/debian/ stable/' >> /etc/apt/sources.list

  # 参考下面这个issue的下载方式
  # https://gitlab.lre.epita.fr/spot/spot/-/issues/544
  echo 'deb [trusted=true] https://download.opensuse.org/repositories/home:/adl/xUbuntu_22.04/ ./' >> /etc/apt/sources.list
  apt update
  apt upgrade
  apt install spot libspot-dev
  ```

3. 具体的参考 Dockerfile

## Run

```
chmod +x build.sh
chmod +x run.sh
# 打包成镜像
docker build -t sb-ltl:urlyy .
# 挂载项目代码并运行
docker run --rm -v dir_path/SB-LTL:/app -it sb-ltl:urlyy
# 在容器内运行
./run.sh
```

# Example

- LTL : `F(a & F(b))`
- events : `["c","!a","a","a","a","c","b","c","a","c"]`
- output :

  ```
  ==================== formula to automata
  LTL is F(a & F(b))
  ==================== ap in LTL
  a: 0
  b: 1
  ============================= Automata
  Initial state: 2
  State 0:
  edge(0 -> 0)
    label = 1
    acc sets = {0}
  State 1:
  edge(1 -> 0)
    label = b
    acc sets = {}
  edge(1 -> 1)
    label = !b
    acc sets = {}
  State 2:
  edge(2 -> 0)
    label = a & b
    acc sets = {}
  edge(2 -> 1)
    label = a & !b
    acc sets = {}
  edge(2 -> 2)
    label = !a
    acc sets = {}
  accept state ID: { 0, }
  ============================= getTrace
  ======== cur_state_id: 2 , cur_event: !a
  2->0 trans:a & b
  cond_check: (b:1, a:1, )
  2->1 trans:a & !b
  cond_check: (b:0, a:1, )
  2->2 trans:!a
  cond_check: (a:0, )
  ======== cur_state_id: 2 , cur_event: a
  2->0 trans:a & b
  cond_check: (b:1, a:1, )
  2->1 trans:a & !b
  cond_check: (b:0, a:1, )
  ======== cur_state_id: 1 , cur_event: a
  1->0 trans:b
  cond_check: (b:1, )
  1->1 trans:!b
  cond_check: (b:0, )
  ======== cur_state_id: 1 , cur_event: a
  1->0 trans:b
  cond_check: (b:1, )
  1->1 trans:!b
  cond_check: (b:0, )
  ======== cur_state_id: 1 , cur_event: b
  1->0 trans:b
  cond_check: (b:1, )
  ======== cur_state_id: 0 , cur_event: a
  0->0 trans:1
  2 0
  2 0
  1 0
  1 0
  1 0
  0 1
  0 1
  ============================= extract_prefix_apath
  aPath: 2 > 1 > 0 >
  accept: true
  ```

## TODO

- 在另外一个项目中基于本项目，结合 插桩+fuzz 进行真正的运行时验证(即类似 [ltlfuzzer](https://github.com/ltlfuzzer/LTL-Fuzzer) 项目一样的效果，但脱离 afl 专注 LTL)（难度较大，不考虑这个了）
- 进一步学习，修改用语。比如目前没分清 event 和 proposition 的区别。
