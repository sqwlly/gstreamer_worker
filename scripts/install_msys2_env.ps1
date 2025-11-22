# Auto-install MSYS2 environment variables to PowerShell profile
# Set code page to UTF-8
chcp 65001 | Out-Null
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

Write-Host "Configuring MSYS2 environment variables..." -ForegroundColor Green

# Check if PowerShell profile exists
if (-not (Test-Path $PROFILE)) {
    Write-Host "Creating PowerShell profile: $PROFILE" -ForegroundColor Yellow
    $profileDir = Split-Path $PROFILE -Parent
    if (-not (Test-Path $profileDir)) {
        New-Item -Path $profileDir -ItemType Directory -Force | Out-Null
    }
    New-Item -Path $PROFILE -Type File -Force | Out-Null
}

# Read existing configuration
$existingContent = ""
if (Test-Path $PROFILE) {
    $existingContent = Get-Content $PROFILE -Raw -Encoding UTF8
}

# Check if MSYS2 configuration already exists
if ($existingContent -match "MSYS2_HOME") {
    Write-Host "MSYS2 configuration already exists, skipping..." -ForegroundColor Yellow
} else {
    # Add MSYS2 configuration
    $msys2Config = @"

# MSYS2 Environment Variables
`$env:MSYS2_HOME = "C:\msys64"
`$env:PATH = "`$env:MSYS2_HOME\ucrt64\bin;`$env:MSYS2_HOME\usr\bin;`$env:PATH"
"@
    
    Add-Content -Path $PROFILE -Value $msys2Config -Encoding UTF8
    Write-Host "Successfully added MSYS2 configuration to PowerShell profile!" -ForegroundColor Green
    Write-Host "Profile location: $PROFILE" -ForegroundColor Cyan
    Write-Host "Please restart PowerShell or run: . `$PROFILE" -ForegroundColor Yellow
}

Write-Host "`nGit Bash configuration:" -ForegroundColor Green
Write-Host "Please manually add the following to ~/.bashrc or ~/.bash_profile:" -ForegroundColor Yellow
Write-Host "export MSYS2_HOME=/c/msys64" -ForegroundColor Cyan
Write-Host 'export PATH="$MSYS2_HOME/ucrt64/bin:$MSYS2_HOME/usr/bin:$PATH"' -ForegroundColor Cyan
