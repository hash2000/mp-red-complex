#Requires -Version 5.1
<#
.SYNOPSIS
	Собирает переносимый архив релиза RedComplex.

.DESCRIPTION
	Готовит каталог развёртывания из уже собранного Launcher и упаковывает его
	в zip: исполняемый файл, библиотеки Qt и зависимости из vcpkg, плагины Qt,
	эталонная конфигурация (src\Configurations), запускалка RedComplex.cmd,
	LICENSE и README.txt.

	Скрипт не собирает проект: перед запуском соберите цель Launcher.

	Используется и локально, и в CI (.github/workflows/release.yml), поэтому
	не зависит от переменных окружения GitHub Actions.

.PARAMETER Version
	Версия для имени архива, например 0.1.0. Ведущая «v» отбрасывается.

.PARAMETER BuildDirectory
	Каталог сборки с Launcher.exe. По умолчанию — штатный build\x64-Clang-Debug,
	иначе единственный сконфигурированный каталог в build.

.PARAMETER OutputDirectory
	Каталог для готового архива. По умолчанию — artifacts в корне репозитория.

.PARAMETER NoQtDeploy
	Не вызывать windeployqt. Недостающие библиотеки и плагины Qt всё равно
	копируются из vcpkg вручную, но runtime компилятора в архив не попадёт.

.PARAMETER KeepStaging
	Не удалять промежуточный каталог развёртывания (удобно для проверки).

.EXAMPLE
	.\scripts\package-release.ps1 -Version 0.1.0

.EXAMPLE
	cmake --build build\x64-Clang-Debug --target Launcher
	.\scripts\package-release.ps1 -Version 0.1.0-local -KeepStaging
#>
[CmdletBinding()]
param(
	[Parameter(Mandatory = $true, Position = 0)]
	[string]$Version,

	[string]$BuildDirectory,

	[string]$OutputDirectory,

	[switch]$NoQtDeploy,

	[switch]$VerifyDependencies,

	[switch]$KeepStaging
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot '_common.ps1')

$root = Get-RedComplexRepositoryRoot

function Get-RedComplexPeDependency {
	# Возвращает имена DLL из таблицы импорта PE-файла (через dumpbin).
	param(
		[Parameter(Mandatory = $true)]
		[string]$Path,

		[Parameter(Mandatory = $true)]
		[string]$Dumpbin
	)

	$output = & $Dumpbin /nologo /dependents $Path 2>&1
	$dependencies = New-Object 'System.Collections.Generic.List[string]'
	foreach ($line in $output) {
		$text = $line.ToString()
		if ($text -match '^\s+(?<name>[A-Za-z0-9_.\-]+\.dll)\s*$') {
			$dependencies.Add($matches['name'])
		}
	}
	return $dependencies
}

if (-not $BuildDirectory) {
	$BuildDirectory = Resolve-RedComplexBuildDirectory
}
elseif (-not [IO.Path]::IsPathRooted($BuildDirectory)) {
	$BuildDirectory = Join-Path $root $BuildDirectory
}

if (-not (Test-Path -LiteralPath $BuildDirectory)) {
	throw "Каталог сборки '$BuildDirectory' не найден."
}
$build = (Resolve-Path -LiteralPath $BuildDirectory).Path

$exePath = Join-Path $build 'Launcher.exe'
if (-not (Test-Path -LiteralPath $exePath)) {
	throw "Не найден '$exePath'. Соберите цель Launcher: cmake --build '$build' --target Launcher"
}

# --- Параметры сборки: тип конфигурации и триплет vcpkg ---------------------

$buildType = Get-RedComplexCMakeCacheValue -BuildDirectory $build -Name 'CMAKE_BUILD_TYPE'
if (-not $buildType) {
	$buildType = 'Release'
}
$isDebug = $buildType -like '*Debug*'

$triplet = Get-RedComplexCMakeCacheValue -BuildDirectory $build -Name 'VCPKG_TARGET_TRIPLET'
if (-not $triplet) {
	$triplet = 'x64-windows'
}

$installedRoot = Join-Path (Join-Path $build 'vcpkg_installed') $triplet
if (-not (Test-Path -LiteralPath $installedRoot)) {
	throw "Не найден каталог зависимостей '$installedRoot'. Конфигурация должна выполняться с vcpkg toolchain."
}

# vcpkg раскладывает библиотеки раздельно: release в bin, debug в debug\bin.
# Берём только тот вариант, который соответствует конфигурации сборки, иначе
# в архив попадут и release-, и debug-копии Qt.
$libraryDirectories = @()
if ($isDebug) {
	$libraryDirectories += (Join-Path $installedRoot 'debug\bin')
}
else {
	$libraryDirectories += (Join-Path $installedRoot 'bin')
}

$pluginsRoot = Join-Path $installedRoot 'Qt6\plugins'
if ($isDebug -and (Test-Path -LiteralPath (Join-Path $installedRoot 'debug\Qt6\plugins'))) {
	$pluginsRoot = Join-Path $installedRoot 'debug\Qt6\plugins'
}
if (-not (Test-Path -LiteralPath $pluginsRoot)) {
	throw "Не найден каталог плагинов Qt '$pluginsRoot'."
}

# Часть рантайм-зависимостей Qt vcpkg держит не в bin, а в tools: например,
# brotlidec.dll и bz2.dll лежат в tools\Qt6\bin. Без них Launcher.exe не
# запустится, поэтому эти каталоги тоже используются как источник DLL.
$toolRoots = @()
$toolsRoot = Join-Path $installedRoot 'tools'
if (Test-Path -LiteralPath $toolsRoot) {
	foreach ($toolDirectory in Get-ChildItem -LiteralPath $toolsRoot -Directory) {
		# tools\tcl — утилиты sqlite3, приложению не нужны.
		if ($toolDirectory.Name -eq 'tcl') {
			continue
		}
		$toolRoots += $toolDirectory.FullName
	}
}

# --- Имена и каталоги -------------------------------------------------------

$packageVersion = $Version.Trim()
if ($packageVersion.StartsWith('v') -or $packageVersion.StartsWith('V')) {
	$packageVersion = $packageVersion.Substring(1)
}
if ($packageVersion -notmatch '^[0-9A-Za-z][0-9A-Za-z._-]*$') {
	throw "Недопустимая версия '$Version'. Ожидается что-то вроде 0.1.0 или 0.2.0-beta."
}

$packageName = "RedComplex-$packageVersion-win64"

if ($OutputDirectory) {
	if (-not [IO.Path]::IsPathRooted($OutputDirectory)) {
		$OutputDirectory = Join-Path $root $OutputDirectory
	}
	$outputRoot = $OutputDirectory
}
else {
	$outputRoot = Join-Path $root 'artifacts'
}

New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
$outputRoot = (Resolve-Path -LiteralPath $outputRoot).Path

$staging = Join-Path $outputRoot $packageName
$archivePath = Join-Path $outputRoot ($packageName + '.zip')

if (Test-Path -LiteralPath $staging) {
	Remove-Item -LiteralPath $staging -Recurse -Force
}
New-Item -ItemType Directory -Path $staging -Force | Out-Null

Write-Host "Версия:      $packageVersion"
Write-Host "Сборка:      $build ($buildType, $triplet)"
Write-Host "Развёртывание: $staging"

# --- 1. Исполняемый файл ----------------------------------------------------

Copy-Item -LiteralPath $exePath -Destination (Join-Path $staging 'Launcher.exe')

# --- 2. windeployqt: библиотеки Qt, плагины и runtime компилятора -----------

$windeployqt = Join-Path $installedRoot 'tools\Qt6\bin\windeployqt.exe'
if (-not $NoQtDeploy -and (Test-Path -LiteralPath $windeployqt)) {
	$arguments = @('--no-translations', '--no-system-d3d-compiler', '--plugindir', $staging, '--dir', $staging)
	if ($isDebug) {
		$arguments = @('--debug') + $arguments
	}
	else {
		$arguments = @('--release') + $arguments
	}
	$arguments += $exePath

	# windeployqt запускает qtpaths, поэтому каталог инструментов Qt должен быть в PATH.
	$previousPath = $env:PATH
	$env:PATH = "$(Join-Path $installedRoot 'tools\Qt6\bin');$(Join-Path $installedRoot 'bin');$previousPath"
	try {
		$exitCode = Invoke-RedComplexNativeCommand -FilePath $windeployqt -Arguments $arguments
	}
	finally {
		$env:PATH = $previousPath
	}

	if ($exitCode -ne 0) {
		Write-Warning "windeployqt завершился с кодом ${exitCode}: недостающие файлы Qt будут скопированы из vcpkg вручную."
	}
}
elseif (-not $NoQtDeploy) {
	Write-Warning "Не найден '$windeployqt': библиотеки Qt будут скопированы из vcpkg вручную."
}

# --- 3. Зависимости приложения и Qt из каталогов сборки и vcpkg -------------

# Копируются только отсутствующие файлы, чтобы не перетирать результат
# windeployqt и не тащить в архив отладочные файлы.
$copiedLibraries = 0
foreach ($directory in (@($build) + $libraryDirectories)) {
	if (-not (Test-Path -LiteralPath $directory)) {
		continue
	}
	foreach ($library in Get-ChildItem -LiteralPath $directory -Filter '*.dll' -File) {
		$target = Join-Path $staging $library.Name
		if (-not (Test-Path -LiteralPath $target)) {
			Copy-Item -LiteralPath $library.FullName -Destination $target
			$copiedLibraries++
		}
	}
}

# То же для tools: там лежат зависимости инструментов Qt (brotli, bzip2 и др.),
# которые нужны и самому приложению. Для Debug-сборки этот источник не нужен:
# отладочные зависимости уже лежат в debug\bin, а release-копии только
# раздували бы архив.
if (-not $isDebug) {
	foreach ($directory in $toolRoots) {
		foreach ($library in Get-ChildItem -LiteralPath $directory -Filter '*.dll' -File -Recurse) {
			$target = Join-Path $staging $library.Name
			if (-not (Test-Path -LiteralPath $target)) {
				Copy-Item -LiteralPath $library.FullName -Destination $target
				$copiedLibraries++
			}
		}
	}
}
if ($copiedLibraries -gt 0) {
	Write-Host "Добавлено DLL из vcpkg: $copiedLibraries"
}

# --- 4. Плагины Qt: platforms, styles, tls, imageformats и прочие ----------

foreach ($category in Get-ChildItem -LiteralPath $pluginsRoot -Directory) {
	$targetCategory = Join-Path $staging $category.Name
	New-Item -ItemType Directory -Path $targetCategory -Force | Out-Null
	foreach ($plugin in Get-ChildItem -LiteralPath $category.FullName -Filter '*.dll' -File) {
		Copy-Item -LiteralPath $plugin.FullName -Destination (Join-Path $targetCategory $plugin.Name) -Force
	}
}

# --- 5. Эталонная конфигурация и файлы сопровождения -----------------------

$configurations = Join-Path $root 'src\Configurations'
if (-not (Test-Path -LiteralPath (Join-Path $configurations 'bootstrap.yaml'))) {
	throw "Не найдена эталонная конфигурация в '$configurations'."
}
$stagingConfigurations = Join-Path $staging 'Configurations'
New-Item -ItemType Directory -Path $stagingConfigurations -Force | Out-Null
Copy-Item -Path (Join-Path $configurations '*') -Destination $stagingConfigurations -Recurse -Force

foreach ($extra in @('RedComplex.cmd', 'README.txt')) {
	$source = Join-Path (Join-Path $root 'packaging') $extra
	if (-not (Test-Path -LiteralPath $source)) {
		throw "Не найден файл упаковки '$source'."
	}
	Copy-Item -LiteralPath $source -Destination (Join-Path $staging $extra)
}

Copy-Item -LiteralPath (Join-Path $root 'LICENSE') -Destination (Join-Path $staging 'LICENSE')

$versionInfo = New-Object 'System.Collections.Generic.List[string]'
$versionInfo.Add("RedComplex $packageVersion")
$versionInfo.Add("Сборка: $buildType, $triplet")

# Версии портов vcpkg задаёт builtin-baseline в манифесте: по нему видно,
# из каких именно версий библиотек собран пакет.
$manifestPath = Join-Path $root 'vcpkg.json'
if (Test-Path -LiteralPath $manifestPath) {
	$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
	if ($manifest.PSObject.Properties.Name -contains 'builtin-baseline') {
		$versionInfo.Add("vcpkg baseline: $($manifest.'builtin-baseline')")
	}
}

$versionInfo.Add("Упаковано: $(Get-Date -Format 'yyyy-MM-dd HH:mm')")
Set-Content -LiteralPath (Join-Path $staging 'version.txt') -Value $versionInfo -Encoding utf8

# --- 6. Проверка комплектности ---------------------------------------------

$qtCore = if ($isDebug) { 'Qt6Cored.dll' } else { 'Qt6Core.dll' }
$required = @('Launcher.exe', $qtCore, 'Configurations\bootstrap.yaml')
$missing = @()
foreach ($relative in $required) {
	if (-not (Test-Path -LiteralPath (Join-Path $staging $relative))) {
		$missing += $relative
	}
}

# Имена плагинов в Debug-сборке Qt получают суффикс d (qwindowsd.dll),
# поэтому проверяем маску, а не точное имя файла.
$platformPlugins = @(Get-ChildItem -LiteralPath (Join-Path $staging 'platforms') -Filter 'qwindows*.dll' -File -ErrorAction SilentlyContinue)
if ($platformPlugins.Count -eq 0) {
	$missing += 'platforms\qwindows*.dll'
}

if ($missing.Count -gt 0) {
	throw "В каталоге развёртывания отсутствуют обязательные файлы: $($missing -join ', ')."
}

# Плагин TLS нужен для HTTPS в FetchApi; без него приложение запустится,
# но сетевые запросы по https будут падать.
$tlsPlugins = @(Get-ChildItem -LiteralPath (Join-Path $staging 'tls') -Filter 'qopensslbackend*.dll' -File -ErrorAction SilentlyContinue)
if ($tlsPlugins.Count -eq 0) {
	Write-Warning 'В архив не попал плагин TLS (tls\qopensslbackend*.dll): HTTPS-запросы могут не работать.'
}

# --- 7. Проверка зависимостей -----------------------------------------------

if ($VerifyDependencies) {
	$dumpbinCommand = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
	$dumpbin = if ($dumpbinCommand) { $dumpbinCommand.Source } else { $null }
	if (-not $dumpbin) {
		# dumpbin ставится вместе с Visual Studio: подхватываем его окружение.
		Initialize-RedComplexVsEnvironment
		$dumpbinCommand = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
		$dumpbin = if ($dumpbinCommand) { $dumpbinCommand.Source } else { $null }
	}

	if (-not $dumpbin) {
		Write-Warning 'dumpbin не найден: проверка зависимостей пропущена.'
	}
	else {
		# Все собранные DLL и exe + их импорт: любая внешняя DLL, которой нет
		# в пакете и в System32, означает, что сборка не запустится у пользователя.
		$stagedLibraries = @{}
		$binaries = New-Object 'System.Collections.Generic.List[System.IO.FileInfo]'
		$binaries.Add((Get-Item -LiteralPath (Join-Path $staging 'Launcher.exe')))
		foreach ($file in Get-ChildItem -LiteralPath $staging -Filter '*.dll' -File -Recurse) {
			$stagedLibraries[$file.Name.ToLowerInvariant()] = $true
			$binaries.Add($file)
		}

		# Эти DLL подгружаются по необходимости на стороне Windows, их может не быть.
		$optionalDependencies = @('d3dcompiler_47.dll', 'opengl32sw.dll', 'libegl.dll', 'libglesv2.dll')
		$missingDependencies = New-Object 'System.Collections.Generic.List[string]'

		foreach ($binary in $binaries) {
			foreach ($dependency in (Get-RedComplexPeDependency -Path $binary.FullName -Dumpbin $dumpbin)) {
				$key = $dependency.ToLowerInvariant()
				if ($stagedLibraries.ContainsKey($key)) { continue }
				if ($key -like 'api-ms-win-*') { continue }
				if ($optionalDependencies -contains $key) { continue }
				if (Test-Path -LiteralPath (Join-Path (Join-Path $env:SystemRoot 'System32') $dependency)) { continue }
				if (-not $missingDependencies.Contains($dependency)) {
					$missingDependencies.Add($dependency)
				}
			}
		}

		if ($missingDependencies.Count -gt 0) {
			throw ("В пакете нет DLL, от которых зависят Launcher.exe/Qt: " + ($missingDependencies -join ', ') +
				'. Добавьте каталог с ними в источник DLL в package-release.ps1.')
		}
		Write-Host "Проверка зависимостей: $($binaries.Count) файлов, все внешние DLL в пакете." -ForegroundColor Green
	}
}

# --- 8. Упаковка ------------------------------------------------------------

if (Test-Path -LiteralPath $archivePath) {
	Remove-Item -LiteralPath $archivePath -Force
}

$tar = Get-Command -Name 'tar.exe' -ErrorAction SilentlyContinue
if ($tar) {
	# bsdtar из Windows 10/11: заметно быстрее Compress-Archive на тысячах файлов.
	& $tar.Source -a -c -f $archivePath -C $outputRoot $packageName
	if ($LASTEXITCODE -ne 0) {
		throw "Не удалось создать архив '$archivePath' (tar, код $LASTEXITCODE)."
	}
}
else {
	Compress-Archive -Path $staging -DestinationPath $archivePath -CompressionLevel Optimal
}

if (-not $KeepStaging) {
	Remove-Item -LiteralPath $staging -Recurse -Force
}

$sizeMb = [math]::Round((Get-Item -LiteralPath $archivePath).Length / 1MB, 1)

Write-Host ''
Write-Host "Готов архив: $archivePath ($sizeMb МБ)" -ForegroundColor Green

if ($env:GITHUB_OUTPUT) {
	Add-Content -Path $env:GITHUB_OUTPUT -Value "archive=$archivePath"
}
