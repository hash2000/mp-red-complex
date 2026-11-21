#Requires -Version 5.1
<#
.SYNOPSIS
	Собирает и запускает один тестовый исполняемый файл Qt Test.

.DESCRIPTION
	Позволяет запустить весь тест или одну тестовую функцию, а также передать
	произвольные аргументы Qt Test (например, -v2 или -datatags). По умолчанию
	перед запуском собирается только указанная цель.

.EXAMPLE
	.\run-test.ps1 TestEngine

.EXAMPLE
	.\run-test.ps1 -Name TestEngine -TestFunction testContext_VariablesLazyMode

.EXAMPLE
	.\run-test.ps1 -Name TestLazyTreeView -NoBuild -QtArguments '-v2'

.EXAMPLE
	.\run-test.ps1 -Name TestEngine -ListFunctions
#>

[CmdletBinding()]
param(
	[Parameter(Mandatory = $true, Position = 0)]
	[string]$Name,

	[string]$BuildDirectory,
	[switch]$NoBuild,
	[string]$TestFunction,
	[switch]$ListFunctions,
	[string]$Platform,
	[string[]]$QtArguments = @(),
	[switch]$NoVsEnvironment
)

. (Join-Path $PSScriptRoot '_common.ps1')

if ($Name -notmatch '^[A-Za-z][A-Za-z0-9_]*$') {
	throw "Некорректное имя тестовой цели '$Name'. Ожидается имя цели CMake, например TestEngine."
}
if ($ListFunctions -and $TestFunction) {
	throw 'Параметры -ListFunctions и -TestFunction несовместимы.'
}

$buildDirectory = Resolve-RedComplexBuildDirectory -BuildDirectory $BuildDirectory

if (-not $NoBuild) {
	$buildExitCode = Invoke-RedComplexTestBuild -BuildDirectory $buildDirectory -Targets @($Name) -NoVsEnvironment:$NoVsEnvironment
	if ($buildExitCode -ne 0) {
		Write-Host ''
		Write-Host ("Сборка завершилась с кодом {0}." -f $buildExitCode) -ForegroundColor Red
		exit $buildExitCode
	}
}

$executable = Find-RedComplexTestExecutable -BuildDirectory $buildDirectory -TestName $Name
if (-not $executable) {
	throw "Не найден исполняемый файл теста '$Name.exe' в '$buildDirectory'. Проверьте имя цели и соберите тесты."
}

Initialize-RedComplexTestEnvironment -BuildDirectory $buildDirectory -Platform $Platform

$runArguments = @()
if ($ListFunctions) {
	$runArguments += '-functions'
}
elseif ($TestFunction) {
	# В Qt 6 имя функции передаётся позиционным аргументом, а не ключом -testfunction.
	$runArguments += $TestFunction
}
$runArguments += $QtArguments

if ($ListFunctions) {
	Write-Host ("Функции теста {0}:" -f $Name) -ForegroundColor Cyan
}
elseif ($TestFunction) {
	Write-Host ("Запуск {0} :: {1}" -f $Name, $TestFunction) -ForegroundColor Cyan
}
else {
	Write-Host ("Запуск {0}" -f $Name) -ForegroundColor Cyan
}

Push-Location -LiteralPath $buildDirectory
try {
	$exitCode = Invoke-RedComplexNativeCommand -FilePath $executable -Arguments $runArguments
}
finally {
	Pop-Location
}

if (-not $ListFunctions) {
	if ($exitCode -eq 0) {
		Write-Host ''
		Write-Host 'Тест прошёл.' -ForegroundColor Green
	}
	else {
		Write-Host ''
		Write-Host ("Тест завершился с кодом {0}." -f $exitCode) -ForegroundColor Red
	}
}

exit $exitCode
