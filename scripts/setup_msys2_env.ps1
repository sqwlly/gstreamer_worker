# MSYS2 环境变量配置
# 将此文件内容添加到 PowerShell 配置文件 ($PROFILE)
# 如果配置文件不存在，运行: New-Item -Path $PROFILE -Type File -Force

$env:MSYS2_HOME = "C:\msys64"
$env:PATH = "$env:MSYS2_HOME\ucrt64\bin;$env:MSYS2_HOME\usr\bin;$env:PATH"

