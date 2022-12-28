PROGRAM		= dnsd
PORT		?= 1053
CONFIG		?= ./conf/config.txt

SDIR		:= src
IDIR		:= include

CXX			?= g++
INCLUDES	:= -I $(IDIR)
C_FLAGS		:= -Wall -Wextra -Wshadow -std=c++20 -fpermissive
L_FLAGS		:= -O2
RC_FLAGS	:= 
DC_FLAGS	:= -g -rdynamic -DDEBUG -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_GLIBCXX_DEBUG_PEDANTIC_ASSERT -fsanitize=address -fsanitize=undefined

ifeq ($(DEBUG), 1)
	CXXFLAGS := $(CXXFLAGS) $(C_FLAGS) $(DC_FLAGS)
	ODIR := obj/debug
	BDIR := bin/debug
else
	CXXFLAGS := $(CXXFLAGS) $(C_FLAGS) $(RC_FLAGS)
	ODIR := obj/release
	BDIR := bin/release
endif

EXE			:= $(BDIR)/$(PROGRAM)
SRCS		:= $(wildcard $(SDIR)/*.cpp)
OBJS		:= $(SRCS:$(SDIR)/%.cpp=$(ODIR)/%.o)
DEPS		:= $(SRCS:$(SDIR)/%.cpp=$(ODIR)/%.d)

run: $(EXE)
	$< $(PORT) $(CONFIG)

build: $(EXE)

docs:
	# TODO

all: build run

$(EXE): $(OBJS)
	$(CXX) $(CXXFLAGS) $(L_FLAGS) $(OBJS) -o $@

-include $(DEPS)

$(ODIR)/%.o: $(SDIR)/%.cpp | dirs
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MP -MMD -c $< -o $@

dirs:
	@mkdir -p $(BDIR) $(ODIR)
	test -f include/magic_enum.hpp || wget https://raw.githubusercontent.com/Neargye/magic_enum/v0.8.2/include/magic_enum.hpp -O include/magic_enum.hpp

clean:
	$(RM) -r obj bin
