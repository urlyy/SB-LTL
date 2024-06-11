# SB-LTL (Spot Based LTL Checker)

<details>
  <summary>选择语言（Choose your language）</summary>
  <ul>
    <li><a href="README.en.md">English</a></li>
    <li><a href="README.md">中文</a></li>
  </ul>
</details>

## introduction

一个基于 [Spot](https://spot.lre.epita.fr/index.html) 的运行时验证 LTL 性质的小工具<br>
主要是检测输入输出的 events 对应的 trace 是否能被 buchi 接收<br>
目前还比较简陋，还未实现对 c 程序的插桩
，只能手动填入所有程序的输入输出 events<br>
目前该程序面向的被测程序是是：每轮一次输入，一次输出，交替进行<br>
我个人的规范是：如输入 4,则 `event` 表示为
`i4`，输出 7,则`event`表示为`o7`

## install

1. 安装 Spot

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

2. g++(应该是这么装的吧，我直接写的)
   ```
   apt install g++
   ```

## run

```
chmod +x run.sh
./run.sh
```

例子：

```
LTL: F(i1 & X(o2))

events : [ "i3","o4","i1","o2" ]

output:
==================== ap in LTL
i1: 0
o2: 4
==================== ap in LTL
============================= Automata
Initial state: 0
State 0:
  edge(0 -> 0)
    label = !i1
    acc sets = {}
  edge(0 -> 1)
    label = i1
    acc sets = {}
State 1:
  edge(1 -> 2)
    label = o2
    acc sets = {}
  edge(1 -> 0)
    label = !i1 & !o2
    acc sets = {}
  edge(1 -> 1)
    label = i1
    acc sets = {}
State 2:
  edge(2 -> 2)
    label = 1
    acc sets = {0}
accept state ID: { 2, }
============================= Automata
cur_state_id: 0 , cur_event: o4
cur_state_id: 0 , cur_event: i1
cur_state_id: 1 , cur_event: o2
accept: true
```

## TODO

- 解决 prefix 接收即判断接收的方法
- 完善 Dockerfile
