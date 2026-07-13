# Vpn-Ofmes ПРОТОТИП!

Профессиональный VPN-клиент для Windows на **C++20** и **Qt 6**, в тёмной
чёрно-фиолетовой цветовой гамме. Приложение не реализует собственный
VPN-протокол или криптографию — оно управляет официальными открытыми
VPN-движками (**WireGuard for Windows** и **OpenVPN**) через их
задокументированные интерфейсы командной строки / management-протокол.

![theme](https://img.shields.io/badge/theme-dark%20purple-7B2EFF)
![lang](https://img.shields.io/badge/C%2B%2B-20-blue)
![qt](https://img.shields.io/badge/Qt-6-41cd52)

## Возможности

- Подключение / отключение к VPN одним нажатием, с выбором сервера.
- Поддержка **WireGuard** (через `wireguard.exe /installtunnelservice`) и
  **OpenVPN** (через официальный `openvpn.exe` + его management-интерфейс).
- Поля адреса сервера, логина и пароля (для OpenVPN; WireGuard использует
  ключи, зашитые в конфиг, поэтому поля логина/пароля для него отключены).
- Индикатор состояния подключения с анимацией (пульсация во время
  подключения/переподключения).
- Индикатор скорости загрузки/отдачи (реальные счётчики трафика ОС).
- Отображение публичного IP-адреса до и после подключения.
- Автоматическое переподключение при обрыве связи (с настраиваемым числом
  попыток и интервалом).
- Сворачивание в системный трей Windows, значок меняется по статусу.
- Журнал событий (Logs) с цветовой подсветкой уровня сообщения.
- Экран настроек: поведение трея, автопереподключение, автообновление.
- Проверка обновлений по JSON-манифесту (`config/version.json`).
- Безопасное хранение сохранённых учётных данных через Windows DPAPI.

## Архитектура

```
Vpn-Ofmes/
├── src/                    Реализация (.cpp)
│   ├── core/                Бизнес-логика, не зависящая от UI
│   └── ui/                   Виджеты и экраны
├── include/vpnofmes/       Заголовки (.h), та же структура что и src/
│   ├── core/
│   └── ui/
├── ui/                     Место для .ui-файлов Qt Designer (не используется,
│                            весь интерфейс построен в коде на C++)
├── resources/               resources.qrc — упаковка иконок/стилей в бинарник
├── icons/                    Минималистичные SVG-иконки
├── styles/                   dark_purple.qss — единый файл темы приложения
├── config/                   Конфигурация по умолчанию + примеры .conf/.ovpn
├── logs/                     Каталог для логов при разработке
├── CMakeLists.txt
└── README.md
```

### Модули core/

| Класс                | Назначение |
|----------------------|------------|
| `Logger`             | Singleton-логгер: пишет в файл и рассылает записи в UI. |
| `ConfigManager`      | Загрузка/сохранение настроек и списка серверов (JSON). |
| `ServerProfile`       | Модель данных VPN-сервера (адрес, протокол, шаблон конфига). |
| `CredentialStore`     | Хранение логина/пароля, зашифрованных через Windows DPAPI. |
| `VpnEngine`           | Абстрактный интерфейс движка VPN. |
| `WireGuardEngine`     | Обёртка над официальным `wireguard.exe`. |
| `OpenVpnEngine`       | Обёртка над официальным `openvpn.exe` + management-протокол. |
| `ConnectionManager`   | Оркестрация движка + логика автопереподключения. |
| `NetworkMonitor`      | Скорость загрузки/отдачи (IP Helper API на Windows). |
| `IpAddressService`    | Определение публичного IP через HTTPS-запрос. |
| `UpdateChecker`       | Сравнение версии с удалённым JSON-манифестом. |

### Модули ui/

`MainWindow` — композиционный корень приложения: создаёт все core-сервисы,
собирает окно (навигационная панель слева + `QStackedWidget` с тремя
экранами) и соединяет сигналы/слоты. `HomePage`, `SettingsPage`, `LogsPage` —
три экрана. `StatusIndicator`, `SpeedWidget`, `TrayManager` — переиспользуемые
UI-компоненты.

## О VPN-движках

Vpn-Ofmes **не** реализует WireGuard или OpenVPN самостоятельно:

- **WireGuard**: используется официальный клиент для Windows
  (`wireguard.exe`), устанавливающий и удаляющий туннель как службу через
  документированные ключи `/installtunnelservice` и `/uninstalltunnelservice`.
  Всё шифрование и туннелирование выполняются драйвером WireGuard.
- **OpenVPN**: используется официальный бинарник `openvpn.exe`, запускаемый
  с включённым **management interface** (как это делает OpenVPN-GUI), через
  который приложение получает события состояния и отправляет команду
  корректного завершения (`signal SIGTERM`).

Для работы приложения на целевой машине должны быть установлены:
- [WireGuard for Windows](https://www.wireguard.com/install/) — для профилей WireGuard;
- [OpenVPN](https://openvpn.net/community-downloads/) — для профилей OpenVPN.

Демонстрационные (нерабочие, без реальных ключей) шаблоны конфигураций лежат
в `config/samples/`. Перед реальным использованием замените их значениями,
которые выдаёт ваш VPN-сервер или провайдер.

## Сборка (Windows, Visual Studio 2022)

### Требования

1. **Visual Studio 2022** с компонентом "Desktop development with C++".
2. **Qt 6** (6.4+) для MSVC, с модулями `Widgets`, `Network`, `Svg`.
   Проще всего поставить через [Qt Online Installer](https://www.qt.io/download-qt-installer).
3. **CMake** 3.21+ (входит в состав Visual Studio 2022 либо отдельно с [cmake.org](https://cmake.org/download/)).
4. Для реальных подключений — [WireGuard for Windows](https://www.wireguard.com/install/) и/или [OpenVPN](https://openvpn.net/community-downloads/).

### Через Visual Studio 2022 (рекомендуется)

1. Откройте папку `Vpn-Ofmes` в Visual Studio 2022 (`File → Open → Folder…`) —
   VS распознает `CMakeLists.txt` и настроит проект автоматически.
2. Укажите путь к Qt6, если CMake не находит его сам: добавьте в
   `CMakeSettings.json` (или в переменные окружения) переменную
   `CMAKE_PREFIX_PATH`, например:
   `C:\Qt\6.7.0\msvc2019_64`
3. Выберите конфигурацию `x64-Release` и нажмите **Build → Build All**.
4. Готовый `VpnOfmes.exe` появится в каталоге `out/build/x64-Release/`,
   рядом с ним будут скопированы `styles/`, `config/`.

### Через командную строку (CMake + MSVC)

```powershell
git clone https://github.com/gosha8046/vpn-ofmes.git
cd vpn-ofmes

cmake -B build -S . -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_PREFIX_PATH="C:\Qt\6.7.0\msvc2019_64"

cmake --build build --config Release
```

### Запуск

```powershell
cd build\Release
VpnOfmes.exe
```

При первом запуске приложение создаёт рабочую копию конфигурации в
`%APPDATA%\Vpn-Ofmes\Vpn-Ofmes\` (настройки, список серверов, логи,
зашифрованные учётные данные), не затрагивая каталог установки.

### Сборка релиза (инсталлятор / архив)

Проект настроен на CPack: после сборки в Release-конфигурации выполните

```powershell
cmake --build build --config Release --target package
```

Это создаст в `build/` ZIP-архив `Vpn-Ofmes-<версия>-win64.zip`, а при
наличии NSIS — ещё и `.exe`-инсталлятор. Если рядом с Qt найден
`windeployqt.exe`, шаг установки автоматически подтянет необходимые Qt DLL.

## Проверка на этой машине (Linux, без GUI)

Сборка и часть кода (всё, кроме DPAPI и IP Helper API, которые компилируются
только под Windows через `#ifdef Q_OS_WIN`) были дополнительно
скомпилированы и прогнаны с `qt6-base-dev` на Linux-песочнице для проверки
логики (создание конфигурации, чтение/запись JSON, работа сигналов/слотов,
построение UI), поскольку в этой среде нет Windows/Visual Studio/реального
экрана для полноценного GUI-теста. Итоговую сборку и ручную проверку на
реальном Windows-окружении необходимо выполнить дополнительно.

## Лицензии сторонних компонентов

Vpn-Ofmes не включает исходный код WireGuard или OpenVPN — оба
устанавливаются пользователем отдельно и запускаются как независимые
процессы. Ознакомьтесь с их лицензиями:
[WireGuard (GPLv2)](https://www.wireguard.com/) и
[OpenVPN (GPLv2 / OpenVPN Inc. EULA для сборок с сайта)](https://openvpn.net/).
