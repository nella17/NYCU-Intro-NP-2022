# INP111 Homework 01 Week #7 (2022-10-27)

Date: 2022-10-27
<span style="color:red">Due Date: 2022-11-17</span>

[TOC]

# Minimized IRC Server (Daemon)

This homework is the extension of our previous lab, the chat server. Instead of working with a proprietary protocol, we aim to implement a server compliant with real-world clients running the IRC protocol. The IRC protocol is standardized in [RFC 1459](https://www.rfc-editor.org/rfc/rfc1459.html). Although it has been updated several times, we choose to follow the simplest (oldest) version to implement this homework.

:::danger
A complete IRC server can interconnect with other servers. To minimize the cost of implementing this homework, you do not need to handle interconnections with other servers. 
:::

The specification of this homework is much more complicated than our previous lab. Please read it carefully and ensure that your implementation works appropriately with our selected console-based IRC client, [weechat](https://weechat.org/). The weechat client can be installed in Debian/Ubuntu Linux using the command ``sudo apt install weechat`` and in Mac OS using the command ``brew install weechat`` (You have to install [``homebrew``](https://brew.sh/) package manager first).

## Brief Descriptions of the Specification

You have to read [RFC 1459](https://www.rfc-editor.org/rfc/rfc1459.html) for the details of commands and response/error messages. Here we simply provide brief descriptions of the required commands and response/error messages you must implement in this homework.

### Commands (received from a Client)

The required commands are listed below. Note that command arguments enclosed by brackets are optional.

```
NICK <nickname>
USER <username> <hostname> <servername> <realname>
PING [<message>]
LIST [<channel>]
JOIN <channel>
TOPIC <channel> [<topic>]
NAMES [<channel>]
PART <channel>
USERS
PRIVMSG <channel> <message>
QUIT
```

You can find the details of each command in Section 4 of [RFC 1459](https://www.rfc-editor.org/rfc/rfc1459.html).

:::success
Additional remarks for the commands listed above are summarized as follows. These remarks might make your implementation simpler.

- A ``<nickname>`` cannot contain spaces.

- A ``<channel>`` must be prefixed with a pond (#) symbol, e.g., ``#channel``.

- The last parameter must be prefixed with a colon (':').

- By default, there is no response from `NICK` and `USER` commands unless there are errors. But the server must send the message of the day (motd) to a client once both commands are accepted.

- `JOIN` a channel is always a success. If the channel does not exist, your server should automatically create the channel.

- For successful `JOIN` and `PART` commands, the server must respond `:<nickname> <the command & params received from the server>` first, followed by the rest of the responses.

- `PRIVMSG` can be only used to send messages into a channel in our implementation. You do not have to implement sending a message to a user privately.
:::

### Server Messages (sent from the Server)

The general form of a server responded message is:

```
:<prefix> NNN [<nickname>] [<params> ... ]
```

The required response and error messages are listed below. You can find the details of each response/error message in Section 6 of [RFC 1459](https://www.rfc-editor.org/rfc/rfc1459.html).

```
(321) RPL_LISTSTART
(322) RPL_LIST
(323) RPL_LISTEND
(331) RPL_NOTOPIC
(332) RPL_TOPIC
(353) RPL_NAMREPLY
(366) RPL_ENDOFNAMES
(372) RPL_MOTD
(375) RPL_MOTDSTART
(376) RPL_ENDOFMOTD
(392) RPL_USERSSTART
(393) RPL_USERS
(394) RPL_ENDOFUSERS

(401) ERR_NOSUCHNICK
(403) ERR_NOSUCHCHANNEL
(411) ERR_NORECIPIENT
(412) ERR_NOTEXTTOSEND
(421) ERR_UNKNOWNCOMMAND
(431) ERR_NONICKNAMEGIVEN
(436) ERR_NICKCOLLISION
(442) ERR_NOTONCHANNEL
(451) ERR_NOTREGISTERED
(461) ERR_NEEDMOREPARAMS
```

:::success
Additional remarks for the general form of the server responded messages are summarized as follows. These remarks might make your implementation simpler.

- The prefix can be a fixed string, e.g., ``mircd`` or your preferred server name. 

- ``NNN`` is the response/error code composed of three digits.

- ``<nickname>`` is required when it is known for an associated connection.

- The number of ``<params>`` depends on the corresponding response/error code.

- The last parameter must be prefixed with a colon (':').

- To deliver a `PRIVMSG` message received from `<userX>`, the received message from `<userX>` is prepended with the prefix `:<userX>` and then delivered to all users in the channel. The resulted message should look like `:<userX> PRIVMSG #<channel> :<message>`. 
:::

## The IRC Client

We use the console-based IRC client, [weechat](https://weechat.org/), to test your implementation. You may use it to play with our sample implementation running at `inp111.zoolab.org` port `10004`. The relevant weechat commands are summarized as follows.

:::danger
Note that weechat, by default, stores user configurations and data in `~/.config/weechat` and `~/.local/share/weechat` and disallows multiple instances to run simultaneously. To run multiple weechat instances on the same machine, you must use the `-d` option to specify a dedicated directory for each running instance.
:::

- List available servers: `/server`

- Add a server: `/server add <servername> <hostname>/<port>`
For example, `/server add mircd inp111.zoolab.org/10004` adds a server named `mircd` running at `inp111.zoolab.org` port `10004`.

- Set user nickname for a server: `/set irc.server.<servername>.nicks "<nickname>"`
For example, `/set irc.server.mircd.nicks "user1"`.

- Connect to a server: `/connect <servername>`
Connect to an added server named `<servername>`. For example, `/connect mircd`.

- List users on the server: `/users`

- List channels on the server: `/list`

- Join a channel: `/join <#channel>`

- Leave a channel: `/part`

- Close a buffer and leave a channel (or a server): `/close`

- Set channel topic: `/topic <topic>`. You can only do this when you are in a channel.

- Terminate weechat: `/quit`

- Send messages in a channel: Simply type the message you want to send, and all the users in the channel should receive the message.

## Demonstration

1. [20 pts] Connect to the server and receive the message of the day (motd).

:::success
You may use the following commands to emulate two IRC users connecting to the same server. Suppose the server runs at `localhost` port `10004`. The command for *User1* and *User2* should be run in two terminals, respectively.
- User1
```
weechat -d ./user1 -r '/server add mircd localhost/10004; /set irc.server.mircd.nicks "user1"; /connect mircd'
```
- User2
```
weechat -d ./user2 -r '/server add mircd localhost/10004; /set irc.server.mircd.nicks "user2"; /connect mircd'
```
If you want to clear the settings for the users, remove the directories `user1` and `user2` in the current working directory before running the above commands.
:::

2. [15 pts] List users on the server: weechat command `/users`

1. [15 pts] List available channels: weechat command `/list`

1. [20 pts] Join a channel successfully: weechat command `/join #chan1`

1. [15 pts] Get and set channel topic: weechat command `/topic hello, world!`

1. [15 pts] Send messages to a channel. Users can simply type messages in a channel.

To simplify the demo process, you may run the following commands iteratively for the two users after they have connected to the server.

- User1
```
/join #chal1
/topic hello, world!
<wait for user2 to join, and then send some messages>
/part
/close
/quit
```

- User2
```
/users
/list
/join #chal1
/topic
<send some messages>
/part
/close
/quit
```

Here is a sample [pcap file](https://inp111.zoolab.org/hw01/mircd.pcap) for you to inspect the messages exchanged between a server and two clients. A few screenshots are also available here for your reference.

- User1 and user2 connect to the server [top: user1; bottom: user2]
![weechat-connected](https://inp111.zoolab.org/hw01/01-weechat-conencted.png)

- User1 created a channel `#chal1`, and user2 listed the channel(s) [top: user1; bottom: user2].
![weechat-connected](https://inp111.zoolab.org/hw01/02-weechat-create-channel-and-list.png)

- User2 joined channel `#chal1` and sent a message [top: user1; bottom: user2].
![weechat-connected](https://inp111.zoolab.org/hw01/03-weechat-join-channel-and-send.png)


