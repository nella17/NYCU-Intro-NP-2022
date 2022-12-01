#!/bin/bash
diff --color -u <(sha1sum files/send/* | sed 's/send//') <(sha1sum files/save/* | sed 's/save//')
