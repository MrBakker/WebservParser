NAME := libparser.a
DBNAME := libparser_debug.a

EXEC_NAME := parser
DBEXEC_NAME := parser_debug

CXX := c++  # or g++-12
DIR := objs/
DBDIR := db_objs/
CXXFLAGS := -Wall -Wextra -Werror -Wpedantic -Wshadow -std=c++20 -MMD
CXXDBFLAGS := $(CXXFLAGS) -g3 -fsanitize=address,undefined,leak -DDEBUG_MODE -D_GLIBCXX_ASSERTIONS -DFD_TRACKING
MAKEFLAGS += -j $(shell nproc)

LIB_SRCS := config/arena.cpp \
	config/config.cpp \
	config/lexer.cpp \
	config/parser.cpp \
	config/parserExceptions.cpp \
	config/types/consts.cpp \
	config/types/path.cpp \
	config/types/size.cpp \
	config/types/timespan.cpp \
	config/rules/objectParser.cpp \
	config/rules/ruleParser.cpp \
	config/rules/ruleTemplates/autoindexRule.cpp \
	config/rules/ruleTemplates/cgiExtensionRule.cpp \
	config/rules/ruleTemplates/cgiRule.cpp \
	config/rules/ruleTemplates/cgiTimeoutRule.cpp \
	config/rules/ruleTemplates/defineRule.cpp \
	config/rules/ruleTemplates/errorpageRule.cpp \
	config/rules/ruleTemplates/includeRule.cpp \
	config/rules/ruleTemplates/indexRule.cpp \
	config/rules/ruleTemplates/locationRule.cpp \
	config/rules/ruleTemplates/maxBodySizeRule.cpp \
	config/rules/ruleTemplates/methodsRule.cpp \
	config/rules/ruleTemplates/portRule.cpp \
	config/rules/ruleTemplates/returnRule.cpp \
	config/rules/ruleTemplates/rootRule.cpp \
	config/rules/ruleTemplates/serverconfigRule.cpp \
	config/rules/ruleTemplates/servernameRule.cpp \
	config/rules/ruleTemplates/uploadstoreRule.cpp

EXEC_SRCS := main.cpp

LIB_OBJS := $(addprefix $(DIR), $(LIB_SRCS:.cpp=.o))
LIB_DEPS := $(LIB_OBJS:%.o=%.d)
LIB_DBOBJS := $(addprefix $(DBDIR), $(LIB_SRCS:.cpp=.o))
LIB_DBDEPS := $(LIB_DBOBJS:%.o=%.d)

EXEC_OBJS := $(addprefix $(DIR), $(EXEC_SRCS:.cpp=.o))
EXEC_DEPS := $(EXEC_OBJS:%.o=%.d)
EXEC_DBOBJS := $(addprefix $(DBDIR), $(EXEC_SRCS:.cpp=.o))
EXEC_DBDEPS := $(EXEC_DBOBJS:%.o=%.d)

all: $(NAME)

$(NAME): $(LIB_OBJS)
	ar rcs $@ $^
	@echo "\033[1;32m$@ static library created!\033[0m"

$(DIR)%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

debug: $(DBNAME)

$(DBNAME): $(LIB_DBOBJS)
	ar rcs $@ $^
	@echo "\033[1;32m$@ debug static library created!\033[0m"

$(DBDIR)%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXDBFLAGS) -c $< -o $@

clean:
	rm -rf $(EXEC_NAME).*
	rm -rf $(DBEXEC_NAME).*
	rm -rf $(DIR)
	rm -rf $(DBDIR)

fclean: clean
	rm -f $(EXEC_NAME)
	rm -f $(DBEXEC_NAME)
	rm -f $(NAME)
	rm -f $(DBNAME)

re: fclean all

dbrun: debug $(EXEC_DBOBJS)
	@echo "\033[1;34mRunning $(DBEXEC_NAME)...\033[0m"
	$(CXX) $(CXXDBFLAGS) -o $(DBEXEC_NAME) $(EXEC_DBOBJS) $(DBNAME)
	./$(DBEXEC_NAME)

run: all $(EXEC_OBJS)
	@echo "\033[1;34mRunning $(EXEC_NAME)...\033[0m"
	$(CXX) $(CXXFLAGS) -o $(EXEC_NAME) $(EXEC_OBJS) $(NAME)
	./$(EXEC_NAME)

-include $(LIB_DEPS)
-include $(LIB_DBDEPS)
-include $(EXEC_DEPS)
-include $(EXEC_DBDEPS)

.PHONY: all clean fclean re debug dbrun run
