#!/bin/bash
name=${1-"user"}
d="/tmp/weechat/inp111/$(echo "$name" | sha1sum | awk '{ print $1 }')"
weechat -d $d -r "/server add mircd inp111.zoolab.org/10004; /set irc.server.mircd.nicks \"$name\"; /connect mircd"
