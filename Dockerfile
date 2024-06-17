FROM ubuntu:22.04

RUN apt-get update
RUN apt install -y wget gnupg g++
# 配置spot的apt下载源
RUN echo 'deb [trusted=true] https://download.opensuse.org/repositories/home:/adl/xUbuntu_22.04/ ./' >/etc/apt/sources.list
RUN wget -q -O - https://www.lrde.epita.fr/repo/debian.gpg | apt-key add -
RUN apt-get update
# 这个依赖好像apt里可查到的版本太低了，要手动安装
RUN wget http://archive.ubuntu.com/ubuntu/pool/main/libt/libtool/libltdl7_2.4.6-14_amd64.deb
RUN dpkg -i libltdl7_2.4.6-14_amd64.deb
RUN rm -f libltdl7_2.4.6-14_amd64.deb
RUN apt install -y libspot-dev spot

WORKDIR /app

CMD ["/bin/bash"]