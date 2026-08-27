# Test-BuildNumber.ps1

$Exe = ".\BuildNumber.exe"
$TestFile = "buildnumber.h"

$script:Passed = 0
$script:Failed = 0

function Remove-TestFile {

    if (Test-Path $TestFile) {
        Remove-Item $TestFile -Force
    }
}

function Get-VersionInfo {

    $text = Get-Content $TestFile -Raw

	$major = [int]([regex]::Match($text, '#define\s+VER_MAJOR\s+(\d+)').Groups[1].Value)
    $minor = [int]([regex]::Match($text, '#define\s+VER_MINOR\s+(\d+)').Groups[1].Value)
    $build = [int]([regex]::Match($text, '#define\s+BUILDNUMBER\s+(\d+)').Groups[1].Value)

    if ($text -match 'BUILD_MODE_PRE') {
        $mode = 'PRE'
    }
    elseif ($text -match 'BUILD_MODE_POST') {
        $mode = 'POST'
    }
    else {
        $mode = 'NONE'
    }

    [PSCustomObject]@{
        Major = $major
        Minor = $minor
        Build = $build
        Mode  = $mode
    }
}

function Assert-Version {

    param(
        [string]$Name,
        [int]$Major,
        [int]$Minor,
        [int]$Build,
        [string]$Mode
    )

    $v = Get-VersionInfo

    $ok =
        ($v.Major -eq $Major) -and
        ($v.Minor -eq $Minor) -and
        ($v.Build -eq $Build) -and
        ($v.Mode  -eq $Mode)

    if ($ok) {

        Write-Host "[PASS] $Name" -ForegroundColor Green
        $script:Passed++
    }
    else {

        Write-Host "[FAIL] $Name" -ForegroundColor Red
        Write-Host "Expected: $Major.$Minor.$Build  $Mode"
        Write-Host "Actual  : $($v.Major).$($v.Minor).$($v.Build)  $($v.Mode)"

        $script:Failed++
    }
}

function Assert-ExitCode {

    param(
        [string]$Name,
        [int]$ExitCode
    )

    if ($LASTEXITCODE -eq $ExitCode) {

        Write-Host "[PASS] $Name" -ForegroundColor Green
        $script:Passed++
    }
    else {

        Write-Host "[FAIL] $Name" -ForegroundColor Red
        Write-Host "Expected ExitCode: $ExitCode"
        Write-Host "Actual ExitCode  : $LASTEXITCODE"

        $script:Failed++
    }
}

function Run-BuildNumber {

    param(
        [string]$Arguments
    )

    Write-Host ""
    Write-Host "EXEC: $Exe $Arguments" -ForegroundColor Yellow

    $args = @()

    if ($Arguments.Length -gt 0) {
        $args += $Arguments.Split(' ')
    }

    & $Exe @args
}

# ------------------------------------------------------------
# Test 1
# ------------------------------------------------------------

Remove-TestFile

Run-BuildNumber "-pre"

Assert-Version `
    "New File PRE" `
    0 0 1 PRE

# ------------------------------------------------------------
# Test 2
# ------------------------------------------------------------

Run-BuildNumber "-pre"

Assert-Version `
    "PRE -> PRE" `
    0 0 2 PRE

# ------------------------------------------------------------
# Test 3
# ------------------------------------------------------------

Run-BuildNumber "-post"

Assert-Version `
    "PRE -> POST" `
    0 0 3 POST

# ------------------------------------------------------------
# Test 4
# ------------------------------------------------------------

Run-BuildNumber "-pre"

Assert-Version `
    "POST -> PRE" `
    0 0 3 PRE

# ------------------------------------------------------------
# Test 5
# ------------------------------------------------------------

Run-BuildNumber "-post"

Assert-Version `
    "POST after PRE" `
    0 0 4 POST

# ------------------------------------------------------------
# Test 6
# ------------------------------------------------------------

Run-BuildNumber "-post"

Assert-Version `
    "POST -> POST" `
    0 0 5 POST

# ------------------------------------------------------------
# Test 7 Reset PRE
# ------------------------------------------------------------

Run-BuildNumber "0 -pre"

Assert-Version `
    "Reset PRE" `
    0 0 1 PRE

# ------------------------------------------------------------
# Test 8 Reset POST
# ------------------------------------------------------------

Run-BuildNumber "0 -post"

Assert-Version `
    "Reset POST" `
    0 0 1 POST

# ------------------------------------------------------------
# Test 9 Build rollover
# ------------------------------------------------------------

@'
#define VERSION_FORMAT "_0.00.000"
#define BUILD_MODE_PRE

#define VER_MAJOR 3
#define VER_MINOR 1
#define BUILDNUMBER 999
'@ | Set-Content $TestFile

Run-BuildNumber "-pre"

Assert-Version `
    "Build rollover" `
    3 2 0 PRE

# ------------------------------------------------------------
# Test 10 Minor rollover
# ------------------------------------------------------------

@'
#define VERSION_FORMAT "_0.00.000"
#define BUILD_MODE_PRE

#define VER_MAJOR 3
#define VER_MINOR 99
#define BUILDNUMBER 999
'@ | Set-Content $TestFile

Run-BuildNumber "-pre"

Assert-Version `
    "Minor rollover" `
    4 0 0 PRE

# ------------------------------------------------------------
# Test 11 Max version
# ------------------------------------------------------------

@'
#define VERSION_FORMAT "_0.00.000"
#define BUILD_MODE_PRE

#define VER_MAJOR 99
#define VER_MINOR 99
#define BUILDNUMBER 999
'@ | Set-Content $TestFile

Run-BuildNumber "-pre"

Assert-ExitCode `
    "Maximum version reached" `
    -4

# ------------------------------------------------------------
# Test 12 Missing mode
# ------------------------------------------------------------

& $Exe

Assert-ExitCode `
    "Missing Mode" `
    -3

# ------------------------------------------------------------
# Ergebnis
# ------------------------------------------------------------

Write-Host ""
Write-Host "==================================" -ForegroundColor Cyan
Write-Host "PASSED : $Passed" -ForegroundColor Green
Write-Host "FAILED : $Failed" -ForegroundColor Red
Write-Host "==================================" -ForegroundColor Cyan

if ($Failed -eq 0) {
    Write-Host "ALL TESTS PASSED" -ForegroundColor Green
}
else {
    Write-Host "TEST FAILURES DETECTED" -ForegroundColor Red
}
# ============================================================================
# BuildNumber Regression Test
# ============================================================================

$Exe      = ".\BuildNumber.exe"
$TestFile = "buildnumber.h"

$script:Passed = 0
$script:Failed = 0

# ============================================================================
# Hilfsfunktionen
# ============================================================================

function Remove-TestFile {

    if (Test-Path $TestFile) {
        Remove-Item $TestFile -Force
    }
}

function Write-TestHeader {

    param(
        [int]$Major,
        [int]$Minor,
        [int]$Build,
        [string]$Mode = "PRE",
        [string]$VersionFormat = "_0.00.000"
    )

@"
#define VERSION_FORMAT "$VersionFormat"
#define BUILD_MODE_$Mode

#define VER_MAJOR $Major
#define VER_MINOR $Minor
#define BUILDNUMBER $Build
"@ | Set-Content $TestFile
}

function Get-VersionInfo {
	
    $content = Get-Content $TestFile

	$majorLine = ($content | Select-String '^#define VER_MAJOR ').Line
	$minorLine = ($content | Select-String '^#define VER_MINOR ').Line
	$buildLine = ($content | Select-String '^#define BUILDNUMBER ').Line

	$major = ($majorLine -split '\s+')[2]
	$minor = ($minorLine -split '\s+')[2]
	$build = ($buildLine -split '\s+')[2]
    if ($content -match 'BUILD_MODE_PRE') {
        $mode = 'PRE'
    }
    elseif ($content -match 'BUILD_MODE_POST') {
        $mode = 'POST'
    }
    else {
        $mode = 'NONE'
    }

    [PSCustomObject]@{
        Major = $major
        Minor = $minor
        Build = $build
        Mode  = $mode
    }
}

function Assert-Version {

    param(
        [string]$Name,
        [int]$Major,
        [int]$Minor,
        [int]$Build,
        [string]$Mode
    )

    $v = Get-VersionInfo

    $ok =
        ($v.Major -eq $Major) -and
        ($v.Minor -eq $Minor) -and
        ($v.Build -eq $Build) -and
        ($v.Mode  -eq $Mode)

    if ($ok) {

        Write-Host "[PASS] $Name" -ForegroundColor Green
        $script:Passed++
    }
    else {

        Write-Host "[FAIL] $Name" -ForegroundColor Red
        Write-Host "Expected: $Major.$Minor.$Build  $Mode"
        Write-Host "Actual  : $($v.Major).$($v.Minor).$($v.Build)  $($v.Mode)"

        $script:Failed++
    }
}

function Assert-ExitCode {

    param(
        [string]$Name,
        [int]$Expected
    )

    if ($LASTEXITCODE -eq $Expected) {

        Write-Host "[PASS] $Name" -ForegroundColor Green
        $script:Passed++
    }
    else {

        Write-Host "[FAIL] $Name" -ForegroundColor Red
        Write-Host "Expected ExitCode: $Expected"
        Write-Host "Actual ExitCode  : $LASTEXITCODE"

        $script:Failed++
    }
}

function Run-BuildNumber {

    param([string]$Arguments)

    $args = @()

    if ($Arguments) {
        $args += $Arguments.Split(' ')
    }

    & $Exe @args
}

# ============================================================================
# Zustandstests
# ============================================================================

$StateTests = @(

    @{
        Name       = "NONE -> PRE"
        Header     = $null
        Args       = "-pre"
        Major      = 0
        Minor      = 0
        Build      = 1
        Mode       = "PRE"
    }

    @{
        Name       = "NONE -> POST"
        Header     = $null
        Args       = "-post"
        Major      = 0
        Minor      = 0
        Build      = 1
        Mode       = "POST"
    }

    @{
        Name       = "PRE -> PRE"
        Header     = @{
            Major = 3
            Minor = 1
            Build = 123
            Mode  = "PRE"
        }
        Args       = "-pre"
        Major      = 3
        Minor      = 1
        Build      = 124
        Mode       = "PRE"
    }

    @{
        Name       = "PRE -> POST"
        Header     = @{
            Major = 3
            Minor = 1
            Build = 123
            Mode  = "PRE"
        }
        Args       = "-post"
        Major      = 3
        Minor      = 1
        Build      = 124
        Mode       = "POST"
    }

    @{
        Name       = "POST -> PRE"
        Header     = @{
            Major = 3
            Minor = 1
            Build = 124
            Mode  = "POST"
        }
        Args       = "-pre"
        Major      = 3
        Minor      = 1
        Build      = 124
        Mode       = "PRE"
    }

    @{
        Name       = "POST -> POST"
        Header     = @{
            Major = 3
            Minor = 1
            Build = 124
            Mode  = "POST"
        }
        Args       = "-post"
        Major      = 3
        Minor      = 1
        Build      = 125
        Mode       = "POST"
    }

    @{
        Name       = "Reset PRE"
        Header     = @{
            Major = 3
            Minor = 5
            Build = 999
            Mode  = "PRE"
        }
        Args       = "0 -pre"
        Major      = 0
        Minor      = 0
        Build      = 1
        Mode       = "PRE"
    }

    @{
        Name       = "Reset POST"
        Header     = @{
            Major = 3
            Minor = 5
            Build = 999
            Mode  = "POST"
        }
        Args       = "0 -post"
        Major      = 0
        Minor      = 0
        Build      = 1
        Mode       = "POST"
    }

    @{
        Name       = "Build Rollover"
        Header     = @{
            Major = 3
            Minor = 1
            Build = 999
            Mode  = "PRE"
        }
        Args       = "-pre"
        Major      = 3
        Minor      = 2
        Build      = 0
        Mode       = "PRE"
    }

    @{
        Name       = "Minor Rollover"
        Header     = @{
            Major = 3
            Minor = 99
            Build = 999
            Mode  = "PRE"
        }
        Args       = "-pre"
        Major      = 4
        Minor      = 0
        Build      = 0
        Mode       = "PRE"
    }
)

# ============================================================================
# Fehlerfälle
# ============================================================================

$ErrorTests = @(

    @{
        Name = "Missing Mode"
        Args = ""
        Code = -3
    }

    @{
        Name = "Double Mode PRE POST"
        Args = "-pre -post"
        Code = -3
    }

    @{
        Name = "Double Mode POST PRE"
        Args = "-post -pre"
        Code = -3
    }

    @{
        Name = "Too Many Parameters"
        Args = "a b c -pre"
        Code = -3
    }
)

# ============================================================================
# Zustandstests ausführen
# ============================================================================

foreach ($t in $StateTests) {

    Remove-TestFile

    if ($t.Header) {

        Write-TestHeader `
            -Major $t.Header.Major `
            -Minor $t.Header.Minor `
            -Build $t.Header.Build `
            -Mode  $t.Header.Mode
    }

    Run-BuildNumber $t.Args

    Assert-Version `
        $t.Name `
        $t.Major `
        $t.Minor `
        $t.Build `
        $t.Mode
}

# ============================================================================
# Fehlerfälle ausführen
# ============================================================================

foreach ($t in $ErrorTests) {

    Remove-TestFile

    Run-BuildNumber $t.Args

    Assert-ExitCode `
        $t.Name `
        $t.Code
}

# ============================================================================
# Max-Version
# ============================================================================

Write-TestHeader `
    -Major 99 `
    -Minor 99 `
    -Build 999 `
    -Mode PRE

Run-BuildNumber "-pre"

Assert-ExitCode `
    "Maximum Version Reached" `
    -4

# ============================================================================
# Zusammenfassung
# ============================================================================

Write-Host ""
Write-Host "========================================"
Write-Host "Passed : $Passed"
Write-Host "Failed : $Failed"
Write-Host "========================================"

if ($Failed -eq 0) {

    Write-Host "ALL TESTS PASSED" -ForegroundColor Green
}
else {

    Write-Host "TEST FAILURES DETECTED" -ForegroundColor Red
}