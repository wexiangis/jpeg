# 交叉编译器选择(没有则注释掉)
# cross:=arm-linux-gnueabihf
# cross:=mips-linux-gnu

# 编译器配置
CC:=gcc
ifdef cross
	CC=$(cross)-gcc
endif

#
CFLAG = -lm

# 源文件夹列表
DIR_SRC = src
DIR_OBJ = obj

# 头文件目录列表
INC = -I$(DIR_SRC)

# obj中的.o文件统计
obj += ${patsubst %.c,$(DIR_OBJ)/%.o,${notdir ${wildcard $(DIR_SRC)/*.c}}}

# 文件夹编译及.o文件转储
%.o:../$(DIR_SRC)/%.c
	@$(CC) -Wall -c $< $(INC) $(CFLAG) -o $@

# 目标编译
target: $(obj)
	@$(CC) -Wall -o app $(obj) $(INC) $(CFLAG)

clean:
	@rm ./obj/* app out.* -rf
