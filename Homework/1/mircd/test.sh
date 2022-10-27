#!/bin/bash
name=${1-"user"}
weechat -d `mktemp -d` -r "/server add mircd localhost/10004; /set irc.server.mircd.nicks \"$name\"; /connect mircd"
