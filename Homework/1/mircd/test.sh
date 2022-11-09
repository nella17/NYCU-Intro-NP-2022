#!/bin/bash
name=${1-"user"}
d="/tmp/weechat/$(echo "$name" | sha1sum | awk '{ print $1 }')"
weechat -d $d -r "/server add mircd localhost/10004; /set irc.server.mircd.nicks \"$name\"; /connect mircd"
