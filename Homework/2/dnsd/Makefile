PROGRAM		= dnsd
PORT		?= 1053
CONFIG		?= ./conf/config.txt

SDIR		:= src
IDIR		:= include

CXX			?= g++
INCLUDES	:= -I $(IDIR)
C_FLAGS		:= -Wall -Wextra -Wshadow -std=c++17
L_FLAGS		:= -O2
RC_FLAGS	:= 
DC_FLAGS	:= -g -DDEBUG -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_GLIBCXX_DEBUG_PEDANTIC_ASSERT

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

$(EXE): dirs $(OBJS)
	$(CXX) $(CXXFLAGS) $(L_FLAGS) $(OBJS) -o $@

-include $(DEPS)

$(ODIR)/%.o: $(SDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MP -MMD -c $< -o $@

dirs:
	@mkdir -p $(BDIR) $(ODIR)

clean:
	$(RM) -r obj bin