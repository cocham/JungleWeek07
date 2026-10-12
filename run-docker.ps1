# Pintos Docker Container Launcher Script (PowerShell)
$containerName = "pintos-week07"

# Check if Docker engine is running
Write-Host "[1/3] Checking Docker engine..." -ForegroundColor Cyan
docker version > $null 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "Docker is not running. Starting Docker Desktop..." -ForegroundColor Yellow
    $dockerPath = "C:\Users\$env:USERNAME\AppData\Local\Programs\DockerDesktop\Docker Desktop.exe"
    if (Test-Path $dockerPath) {
        Start-Process $dockerPath
    } else {
        Start-Process "Docker Desktop.exe" -ErrorAction SilentlyContinue
    }
    
    Write-Host "Waiting for Docker daemon to become ready..." -ForegroundColor Yellow
    $retry = 0
    while ($retry -lt 30) {
        Start-Sleep -Seconds 2
        docker version > $null 2>&1
        if ($LASTEXITCODE -eq 0) { break }
        $retry++
        Write-Host "." -NoNewline
    }
    Write-Host ""
}

# Check docker status again
docker version > $null 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Could not connect to Docker. Please ensure Docker Desktop is running and try again." -ForegroundColor Red
    exit 1
}

# Ensure container is up
Write-Host "[2/3] Starting Pintos container ($containerName)..." -ForegroundColor Cyan
docker compose up -d

# Attach to container
Write-Host "[3/3] Attaching to container shell..." -ForegroundColor Green
Write-Host "To exit, type 'exit'. Container will keep running in the background." -ForegroundColor DarkGray
docker exec -it -w /workspaces/pintos_22.04_lab_docker/pintos $containerName bash
