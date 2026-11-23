# mp-red-complex

## Документация

- [docs/README.md](docs/README.md) — контексты, документы, консоль и SystemRouter.

## Релизы

Сборка версий автоматизирована: тег вида `vX.Y.Z` запускает
[workflow](.github/workflows/release.yml), который собирает Release-сборку под
Windows x64 и прикладывает zip-архив к GitHub Release. Как выпустить версию,
проверить сборку без тега и что лежит в архиве — в
[docs/releases/README.md](docs/releases/README.md).

Локально тот же архив собирается скриптом:

```powershell
.\scripts\package-release.ps1 -Version 0.1.0-local
```

## Тесты

Скрипты запуска тестов описаны в [scripts/README.md](scripts/README.md):

```powershell
# Все тесты: сборка и ctest
.\scripts\run-tests.ps1

# Один набор или один тест
.\scripts\run-tests.ps1 -Suite ui
.\scripts\run-test.ps1 TestEngine
```
