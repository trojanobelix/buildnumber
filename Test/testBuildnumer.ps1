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

function Write-TestHeaderToFile {

    param(
        [string]$FileName,
        [int]$Major,
        [int]$Minor,
        [int]$Build,
        [string]$Mode = "PRE"
    )

@"
#define VERSION_FORMAT "_0.00.000"
#define BUILD_MODE_$Mode
#define VER_MAJOR $Major
#define VER_MINOR $Minor
#define BUILDNUMBER $Build
"@ | Set-Content $FileName
}

function Test-GeneratedFile {

    param(
        [string]$Name,
        [string]$FileName,
        [int]$ExpectedMajor,
        [int]$ExpectedMinor,
        [int]$ExpectedBuild
    )

    if ($LASTEXITCODE -ne 0)
    {
        Write-Host "[FAIL] $Name (ExitCode=$LASTEXITCODE)" -ForegroundColor Red
        $script:Failed++
        return
    }

    if (!(Test-Path $FileName))
    {
        Write-Host "[FAIL] $Name (File not found)" -ForegroundColor Red
        $script:Failed++
        return
    }

    $txt = Get-Content $FileName -Raw

    $ok =
        ($txt -match "#define\s+VER_MAJOR\s+$ExpectedMajor\b") -and
        ($txt -match "#define\s+VER_MINOR\s+$ExpectedMinor\b") -and
        ($txt -match "#define\s+BUILDNUMBER\s+$ExpectedBuild\b")

    if ($ok)
    {
        Write-Host "[PASS] $Name" -ForegroundColor Green
        $script:Passed++
    }
    else
    {
        Write-Host "[FAIL] $Name (Content Mismatch)" -ForegroundColor Red
        $script:Failed++
    }
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
        Args       = "-reset -pre"
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
        Args       = "-reset -post"
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
	
	@{
		Name = "Help"
		Args = "-h"
		Code = 0
	}
	
	@{
    Name = "Reset Without Mode"
    Args = "-reset"
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
# Ungültiger Pfad
# ============================================================================

Remove-Item test.h -ErrorAction SilentlyContinue

& $Exe "X:\this\path\does\not\exist" test.h -pre

Assert-ExitCode `
    "Invalid Path" `
    -1

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
# Custom Filename
# ============================================================================

Remove-Item "custom.h" -ErrorAction SilentlyContinue

Write-TestHeaderToFile `
    -FileName "custom.h" `
    -Major 1 `
    -Minor 2 `
    -Build 3 `
    -Mode PRE

& $Exe custom.h -pre

Test-GeneratedFile `
    -Name "Custom Filename" `
    -FileName "custom.h" `
    -ExpectedMajor 1 `
    -ExpectedMinor 2 `
    -ExpectedBuild 4

# ============================================================================
# Path + Filename
# ============================================================================

$TestDir = ".\TestOutput"

Remove-Item $TestDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $TestDir | Out-Null

@"
#define VERSION_FORMAT "_0.00.000"
#define BUILD_MODE_PRE

#define VER_MAJOR 5
#define VER_MINOR 6
#define BUILDNUMBER 7
"@ | Set-Content "$TestDir\pathtest.h"

& $Exe $TestDir pathtest.h -pre

Test-GeneratedFile `
    -Name "Path And Filename" `
    -FileName "$TestDir\pathtest.h" `
    -ExpectedMajor 5 `
    -ExpectedMinor 6 `
    -ExpectedBuild 8

# ============================================================================
# Create Missing File
# ============================================================================

Remove-Item "does_not_exist.h" -ErrorAction SilentlyContinue

& $Exe does_not_exist.h -pre
Test-GeneratedFile `
    -Name "Create Missing File" `
    -FileName "does_not_exist.h" `
    -ExpectedMajor 0 `
    -ExpectedMinor 0 `
    -ExpectedBuild 1

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