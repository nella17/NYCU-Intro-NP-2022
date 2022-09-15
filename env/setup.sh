#!/bin/bash

apt-get update
yes | unminimize
apt-get install -y tini iproute2 iputils-ping net-tools netcat
apt-get install -y openssh-server sudo vim grep gawk rsync tmux man manpages manpages-dev manpages-posix manpages-posix-dev diffutils
apt-get install -y gcc gcc-multilib g++ g++-multilib gdb make yasm nasm tcpdump libcapstone-dev python3
apt-get install -y libc6-dbg dpkg-dev
apt-get install -y curl git zsh
#apt-get install -y qemu-user-static gcc-mips64-linux-gnuabi64
#apt-get install -y musl
#ln -s /lib/x86_64-linux-musl/libc.so /usr/lib/libc.musl-x86_64.so.1
# /var/run/sshd: required on ubuntu
# mkdir /var/run/sshd

# locale
apt-get install -y locales
LANGUAGE en_US.UTF-8
LANG en_US.UTF-8
LC_ALL en_US.UTF-8
echo "en_US.UTF-8 UTF-8" > /etc/locale.gen
/usr/sbin/locale-gen

# gen ssh-keys, allow empty password
#ssh-keygen -t dsa -f /etc/ssh/ssh_host_dsa_key
#ssh-keygen -t rsa -f /etc/ssh/ssh_host_rsa_key
# echo 'PermitEmptyPasswords yes' >> /etc/ssh/sshd_config
# sed -i 's/nullok_secure/nullok/' /etc/pam.d/common-auth

# add user/group, empty password, allow sudo
# groupadd -g 1000 chuang
# useradd --uid 1000 --gid 1000 --groups root,sudo,adm,users --create-home --password '' --shell /bin/bash chuang
# echo '%sudo ALL=(ALL) ALL' >> /etc/sudoers

# disable priviledged port
echo 'net.ipv4.ip_unprivileged_port_start=0' > /etc/sysctl.d/50-unprivileged-ports.conf
sysctl --system
