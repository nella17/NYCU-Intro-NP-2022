#pragma once

enum RPL {
    WELCOME     = 001,
    LUSERCLIENT = 251,
    LISTSTART   = 321,
    LIST        = 322,
    LISTEND     = 323,
    NOTOPIC     = 331,
    TOPIC       = 332,
    NAMREPLY    = 353,
    ENDOFNAMES  = 366,
    MOTD        = 372,
    MOTDSTART   = 375,
    ENDOFMOTD   = 376,
    USERSSTART  = 392,
    USERS       = 393,
    ENDOFUSERS  = 394,
};

enum ERR {
    NOSUCHNICK          = 401,
    NOSUCHCHANNEL       = 403,
    NOORIGIN            = 409,
    NORECIPIENT         = 411,
    NOTEXTTOSEND        = 412,
    UNKNOWNCOMMAND      = 421,
    NONICKNAMEGIVEN     = 431,
    ERRONEUSNICKNAME    = 432,
    NICKCOLLISION       = 436,
    NOTONCHANNEL        = 442,
    NOTREGISTERED       = 451,
    NEEDMOREPARAMS      = 461,
    ALREADYREGISTRED    = 462,
};
