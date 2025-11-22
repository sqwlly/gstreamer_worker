param(
    [string] = "1.24.4",
    [string] = "\..\third_party",
    [switch]
)

Continue = "Stop"

 = Resolve-Path -Path  -ErrorAction SilentlyContinue
if (-not ) {
     = New-Item -ItemType Directory -Path  -Force | Select-Object -ExpandProperty FullName
}

 = "gstreamer-.tar.xz"
 = Join-Path  
 = "https://gstreamer.freedesktop.org/src/gstreamer/"

if ((Test-Path ) -and -not ) {
    Write-Host "Archive already downloaded: "
} else {
    Write-Host "Downloading "
    Invoke-WebRequest -Uri  -OutFile 
}

 = Join-Path  "gstreamer-"
if ((Test-Path ) -and -not ) {
    Write-Host "GStreamer source already extracted: "
    return
}

if (Test-Path ) {
    Remove-Item -Recurse -Force 
}

Write-Host "Extracting archive to "
& tar -xf  -C 

Write-Host "Done. Sources located at "
