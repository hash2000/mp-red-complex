#Requires -Version 5.1
<#
.SYNOPSIS
	Собирает и запускает набор тестов RedComplex через ctest.

.DESCRIPTION
	По умолчанию собирает и запускает все тесты. Набор выбирается параметром -Suite.
	Сборка выполняется только для тестов выбранного набора, затем ctest запускает
	зарегистрированные тесты по точному имени.

.EXAMPLE
	.\run-tests.ps1

.EXAMPLE
	.\run-tests.ps1 -Suite ui -Parallel 8

.EXAMPLE
	.\run-tests.ps1 -Suite engine -NoBuild -CtestArguments '--repeat', 'until-fail:3'
#>

[CmdletBinding()]
param(
	[string]$Suite = 'all',
	[string]$BuildDirectory,
	[switch]$NoBuild,
	[int]$Parallel = 1,
	[string]$Platform,
	[string]$JUnit,
	[string[]]$CtestArguments = @(),
	[switch]$NoVsEnvironment
)

. (Join-Path $PSScriptRoot '_common.ps1')

Assert-RedComplexTestSuite -Name $Suite

$buildDirectory = Resolve-RedComplexBuildDirectory -BuildDirectory $BuildDirectory
$targets = @(Get-RedComplexTestSuite -Name $Suite)
if ($targets.Count -eq 0) {
	throw "Набор тестов '$Suite' пуст."
}

if (-not $NoBuild) {
	$buildExitCode = Invoke-RedComplexTestBuild -BuildDirectory $buildDirectory -Targets $targets -NoVsEnvironment:$NoVsEnvironment
	if ($buildExitCode -ne 0) {
		Write-Host ''
		Write-Host ("Сборка завершилась с кодом {0}." -f $buildExitCode) -ForegroundColor Red
		exit $buildExitCode
	}
}

$missing = @()
foreach ($target in $targets) {
	if (-not (Find-RedComplexTestExecutable -BuildDirectory $buildDirectory -TestName $target)) {
		$missing += $target
	}
}
if ($missing.Count -gt 0) {
	$names = $missing -join ', '
	throw "Не найдены исполняемые файлы тестов: $names. Соберите тесты (scripts\build-tests.ps1) или уберите -NoBuild."
}

Initialize-RedComplexTestEnvironment -BuildDirectory $buildDirectory -Platform $Platform

Assert-RedComplexCommand -Name 'ctest' -Hint 'Установите CMake 4.1+ и добавьте его в PATH.'

$escapedNames = $targets | ForEach-Object { [regex]::Escape($_) }
$testFilter = '^(' + ($escapedNames -join '|') + ')$'

$ctestArguments = @(
	'--test-dir', $buildDirectory,
	'--output-on-failure',
	'--no-tests=error',
	'-R', $testFilter
)

if ($Parallel -gt 1) {
	$ctestArguments += @('--parallel', $Parallel)
}

if ($JUnit) {
	$junitPath = if ([IO.Path]::IsPathRooted($JUnit)) {
		$JUnit
	}
	else {
		Join-Path (Get-Location).Path $JUnit
	}

	$junitDirectory = Split-Path -Parent $junitPath
	if ($junitDirectory -and -not (Test-Path -LiteralPath $junitDirectory)) {
		New-Item -ItemType Directory -Path $junitDirectory -Force | Out-Null
	}

	$ctestArguments += @('--output-junit', $junitPath)
}

if ($CtestArguments) {
	$ctestArguments += $CtestArguments
}

$suiteTitle = if ($Suite -eq 'all') { 'все наборы' } else { $Suite }
Write-Host ''
Write-Host ("Запуск тестов: {0} ({1} шт.)" -f $suiteTitle, $targets.Count) -ForegroundColor Cyan
Write-Host ("Каталог сборки: {0}" -f $buildDirectory)
Write-Host ("Фильтр ctest: {0}" -f $testFilter)
Write-Host ''

$exitCode = Invoke-RedComplexNativeCommand -FilePath 'ctest' -Arguments $ctestArguments
if ($exitCode -eq 0) {
	Write-Host ''
	Write-Host 'Все выбранные тесты прошли.' -ForegroundColor Green
}
else {
	Write-Host ''
	Write-Host ("Тесты завершились с кодом {0}." -f $exitCode) -ForegroundColor Red
}

exit $exitCode
