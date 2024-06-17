# SB-LTL (Spot Based LTL Checker)

[中文](README.md) | [English](README.en.md)

## Introduction

- A small tool for runtime verification of LTL properties based on [Spot](https://spot.lre.epita.fr/index.html)
- Mainly detects whether the trace corresponding to input and output events can be accepted by a Büchi automaton
- The current program is designed for: one round of input, a set of outputs, alternating
- It is still quite rudimentary at the moment and has not yet implemented instrumentation for C programs; all program input and output events must be manually entered
- My personal convention is: if the input is 4, then the `event` is represented as `i4`, and if the output is 7, then the `event` is represented as `o7`
- The program considers `printf(xx)` as a proposition, and a state of an automaton is a Boolean combination of multiple propositions, such as `state_1=i1,state_2=[o2,o3,o4]`. Testers need to manually insert probe code `printf("ixx")` at the input location of the program to be tested, and insert probe code `printf("oxx")` below key code. The probe code needs to collect `i event` after one input and collect all `o event` between two inputs
- This project is a learning project for LTL, so there may be omissions. Corrections are welcome

## Install

- Only depends on spot and g++

- Spot installation cannot refer to the tutorial, as follows:

  ```
  The official one seems to have some issues
  wget -q -O - https://www.lrde.epita.fr/repo/debian.gpg | apt-key add -
  echo 'deb http://www.lrde.epita.fr/repo/debian/ stable/' >> /etc/apt/sources.list
  Refer to the following issue for download method
  https://gitlab.lre.epita.fr/spot/spot/-/issues/544
  echo 'deb [trusted=true] https://download.opensuse.org/repositories/home:/adl/xUbuntu_22.04/ ./' >/etc/apt/sources.list
  apt update
  apt upgrade
  apt install spot libspot-dev
  ```

3. For details, refer to the Dockerfile

## Run

```
chmod +x build.sh
chmod +x run.sh

# Package into an image
docker build -t sb-ltl:urlyy .

# Mount project code and run
docker run --rm -v /home/urlyy/Desktop/ltl-fuzzer/SB-LTL:/app -it sb-ltl:urlyy

# Run inside the container
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

- In another project, based on this project, combine instrumentation + fuzzing for real runtime verification (similar to the effect of the [ltlfuzzer](https://github.com/ltlfuzzer/LTL-Fuzzer), but focus on LTL without afl) (Difficult, not considered)
