#!/bin/bash
# 自动安装 MSYS2 环境变量到 Git Bash 配置文件

echo "正在配置 MSYS2 环境变量..."

# 确定配置文件路径（优先使用 .bashrc）
if [ -f ~/.bashrc ]; then
    CONFIG_FILE=~/.bashrc
elif [ -f ~/.bash_profile ]; then
    CONFIG_FILE=~/.bash_profile
else
    CONFIG_FILE=~/.bashrc
    touch "$CONFIG_FILE"
    echo "创建配置文件: $CONFIG_FILE"
fi

# 检查是否已存在 MSYS2 配置
if grep -q "MSYS2_HOME" "$CONFIG_FILE" 2>/dev/null; then
    echo "检测到已存在 MSYS2 配置，跳过添加。"
else
    # 添加 MSYS2 配置
    cat >> "$CONFIG_FILE" << 'EOF'

# MSYS2 环境变量配置
export MSYS2_HOME=/c/msys64
export PATH="$MSYS2_HOME/ucrt64/bin:$MSYS2_HOME/usr/bin:$PATH"
EOF
    
    echo "已成功添加 MSYS2 配置到 Git Bash 配置文件！"
    echo "配置文件位置: $CONFIG_FILE"
    echo "请运行: source $CONFIG_FILE 或重新打开 Git Bash"
fi

