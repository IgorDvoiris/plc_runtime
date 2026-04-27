

# CLAUDE.md

## Project Context

**system** — детерминированный runtime-каркас на C++20 для исполнения пользовательских программ на языке ST (Structured Text) согласно стандарту IEC 61131-3. Пользовательские программы компилируются компилятором **Matiec** в нативный код (shared libraries `.so` с `-fPIC`) и загружаются динамически в runtime.

### Ключевая цепочка исполнения

```
main.cpp (Composition Root)
  → ComponentManager (lifecycle)
    → LoggerComponent          ← НЕ РЕАЛИЗОВАН
    → EventManagerComponent    ← НЕ РЕАЛИЗОВАН
    → SchedulerComponent → Scheduler (tick-loop)
    → TaskManagerComponent → TaskManager → Task[]
  → PluginLoader
    → libsystem_monitor.so (SystemMonitorPlugin)
    → libiec_runtime.so (IecRuntimePlugin)
      → IecProgram → MatiecLoader
        → libst_logic.so (Matiec-compiled ST program)
  → MotorSimulation (IO thread)
  → IoBuffer ↔ ProcessImage ↔ IecRawBuffer
```

### Роли основных подсистем

| Подсистема | Роль |
|---|---|
| `ProcessImage` | Байтовый снимок I/O, единственная точка обмена данными между IEC-программой и внешним миром |
| `IoBuffer` | Thread-safe двойной буфер на границе IO-потока и tick-loop |
| `IecRawBuffer` | Типизированный адаптер поверх ProcessImage для Matiec glueVars |
| `MatiecLoader` | Загрузчик `.so`, разрешение символов Matiec, трансфер данных через glueVars |
| `IecProgram` | Единица исполнения IEC — обёртка над MatiecLoader, реализует `ITickable` |
| `Scheduler` | Детерминированный tick-loop с поддержкой интервалов |
| `TaskManager` | Группировка `ITickable` в именованные задачи (`Task`) |
| `SystemBus` | Типобезопасный Service Locator для IoC |
| `SystemMonitor` | Watchdog: детекция overrun задач |

---

## 1. Архитектурные принципы (Design Patterns)

### 1.1 Component-Oriented Architecture

Все логические сущности runtime представлены как компоненты с явным жизненным циклом (`IComponent`):
- `init(SystemBus&)` — регистрация сервисов, разрешение зависимостей
- `start()` — начало работы
- `stop()` — освобождение ресурсов

Порядок: init — прямой, start — прямой, stop — **обратный** (LIFO).

**Диаграмма классов подтверждает**: `ComponentManager` хранит `IComponent[]`, к нему подключаются `LoggerComponent`, `EventManagerComponent`, `SchedulerComponent`, `TaskManagerComponent`.

### 1.2 Inversion of Control / Service Locator

`SystemBus` выполняет роль центрального реестра сервисов (`map<std::type_index, T>`). Все зависимости разрешаются через `bus.getService<T>()`. Прямое создание зависимостей внутри компонентов запрещено.

**Диаграмма подтверждает**: `SystemBus` — центральный хаб, на который ссылаются `ComponentManager`, `Scheduler`, `TaskManager`, `EventManager`.

### 1.3 Plugin Architecture

Расширения загружаются как `.so` через `dlopen`/`dlsym`. Каждый плагин экспортирует `extern "C"` функции:
```cpp
IPlugin* createPlugin();
void     destroyPlugin(IPlugin*);
```
Для IEC Runtime дополнительно:
```cpp
void setProcessImage(ProcessImage*);
```

### 1.4 Composition Root

`main.cpp` — единственное место, где допускается создание компонентов, сборка системы и wiring зависимостей.

### 1.5 Deterministic Tick-Loop

Вся логика исполняется синхронно внутри `scheduler->tick(nowMs)`. Асинхронность ограничена уровнем IO (отдельный поток симуляции), но IEC Runtime видит только синхронные данные через ProcessImage.

### 1.6 Process Image Pattern (IEC 61131-3)

Классический паттерн промышленных контроллеров:
1. **Начало цикла**: копирование входов из IO → ProcessImage
2. **Исполнение логики**: IEC-программа работает только с ProcessImage
3. **Конец цикла**: копирование выходов ProcessImage → IO

### 1.7 Adapter Pattern

`IecRawBuffer` — адаптер между плоским байтовым `ProcessImage` и типизированной моделью Matiec glueVars.

### 1.8 Observer / Callback Pattern

`SystemMonitorPlugin` использует callback-модель для уведомления об overrun:
```cpp
monitor->onOverrun([](const TaskTimingInfo& info) { ... });
```

**Паттерн Observer обязателен** для будущих подсистем:
- `EventManager` (диаграмма: `map<EventType, IEventListener>`) — подписка на типизированные события
- `IecDiagnostics` — пассивное наблюдение за состоянием исполнения
- Alarm system — по аналогии с overrun callbacks

### 1.9 Strategy Pattern

`ITickable` — единый интерфейс исполняемой единицы. `Task`, `IecProgram`, `MotorControlTask` — все реализуют один контракт `tick()`.

**Диаграмма подтверждает**: `Scheduler` хранит `ITickable[]`, `Task` хранит `ITickable[]`, `IecProgram` реализует `ITickable`.

### 1.10 Composite Pattern

`Task` реализует `ITickable` и содержит `ITickable[]` — классический Composite. Scheduler не различает одиночную задачу и группу.

### 1.11 Mediator Pattern

`SystemBus` выступает медиатором: компоненты не знают друг о друге, общаются только через шину сервисов.

---

## 2. Архитектурные границы (Architectural Boundaries)

Диаграмма классов чётко разделяет систему на **два слоя** с явной границей на уровне `IecProgram`:

### 2.1 Upper Layer: Core Runtime

```
┌─────────────────────────────────────────────────────────┐
│                    CORE RUNTIME LAYER                    │
│                                                         │
│  ComponentManager ─── IComponent[]                      │
│    ├── LoggerComponent                                  │
│    ├── EventManagerComponent ─── EventManager            │
│    ├── SchedulerComponent ─── Scheduler ─── ITickable[] │
│    └── TaskManagerComponent ─── TaskManager ─── Task[]  │
│                                                         │
│  SystemBus (map<type_index, T>)                         │
└──────────────────────────┬──────────────────────────────┘
                           │
                     IecProgram  ← граница слоёв
                           │
┌──────────────────────────┴──────────────────────────────┐
│                  IEC 61131-3 RUNTIME LAYER               │
│                                                         │
│  IecExecutionEngine                                     │
│    └── IecExecutionContext                               │
│          ├── IecDiagnostics                              │
│          ├── RuntimeContext                               │
│          └── IecExpression[]                             │
│                                                         │
│  IecIoBinding ──── IecIoEndpoint ──── IecValueRef       │
│                                                         │
│  IecMemory ──── IecVariable[]                           │
│                    ├── IecType                           │
│                    └── IecValueRef ──── IecValue         │
└─────────────────────────────────────────────────────────┘
```

### 2.2 Правила пересечения границ

| Правило | Описание |
|---|---|
| **IecProgram — единственный мост** | `IecProgram` реализует `ITickable` (Core) и владеет `IecExecutionEngine`, `IecMemory`, `IecIoBinding` (IEC Layer) |
| **Core не знает об IEC** | `Scheduler`, `TaskManager`, `ComponentManager` не имеют зависимостей на IEC-сущности |
| **IEC не знает о Core** | `IecExecutionEngine`, `IecMemory`, `IecExpression` не знают о `Scheduler` и `Task` |
| **SystemBus — точка интеграции** | `IIecRuntime` регистрируется в `SystemBus`, но IEC-внутренности не экспортируются |
| **IO Binding — мост к Hardware** | `IecIoEndpoint` реализуется System Layer, используется IEC Layer через `IecIoBinding` |

### 2.3 Hardware Layer (не реализован)

Согласно ТЗ (`system_layer.md`), ниже IEC Runtime Layer располагается:

```
┌─────────────────────────────────────────┐
│           HARDWARE / SYSTEM LAYER        │
│                                         │
│  DeviceManager ─── Device[]             │
│  FieldbusManager ─── Fieldbus[]         │
│  IO Endpoints (реализация IecIoEndpoint)│
└─────────────────────────────────────────┘
```

**Ключевой принцип**: IEC Runtime Layer видит только абстрактные `IecIoEndpoint`, не конкретные устройства.

---

## 3. Жёсткие ограничения (Hard Constraints)

### 3.1 Тайминги

| Ограничение | Значение | Источник |
|---|---|---|
| Базовый tick главного цикла | **10 ms** | `main.cpp: tickInterval{10}` |
| IecTask интервал | **20 ms** | `main.cpp: createTask("IecTask", 20)` |
| StatusTask интервал | **100 ms** | `main.cpp: createTask("StatusTask", 100)` |
| Watchdog IecTask | 20 ms ± 30 ms | `monitor->watchTask("IecTask", 20, 30)` |
| Watchdog StatusTask | 100 ms ± 15 ms | `monitor->watchTask("StatusTask", 100, 15)` |

**Инвариант**: `tick()` **не должен блокироваться**. Любой overrun детектируется `WatchdogChecker`.

### 3.2 Детерминизм

- IEC Runtime **не является отдельным процессом**
- Потоки, таймеры и асинхронность вводятся **осознанно и ограниченно**
- Порядок вызова задач определяется порядком регистрации в Scheduler
- Конфигурация не изменяется во время исполнения

### 3.3 Протокол загрузки IEC-программы

Строгий порядок:
1. `setProcessImage(&processImage)` — **до** `createPlugin()`
2. `createPlugin()` → `IecRuntimePlugin`
3. `iecPlugin->init(bus)` — регистрация `IIecRuntime` в SystemBus
4. `iecRuntime->loadProgram(soPath, instanceName, taskName)` — загрузка `.so`
5. `iecPlugin->start()`

Нарушение порядка → `nullptr` / fatal error.

### 3.4 Matiec ABI-контракт

Загружаемая `.so` **обязана** экспортировать:
- `config_init__()` — обязательный
- `config_run__(uint64_t)` — обязательный
- `glueVars()` — обязательный

Опциональные:
- `updateTime()` — для TON/TOF/TP таймеров
- `bool_input`, `bool_output`, `byte_input`, `byte_output`, `int_input`, `int_output`, `dint_input`, `dint_output`, `lint_input`, `lint_output`

### 3.5 Безопасность

- `ProcessImage` доступен **только из tick-loop потока** без синхронизации
- Обмен с IO-потоком **только через `IoBuffer`** с `std::mutex`
- Плагины загружаются с `RTLD_LOCAL` — изоляция символов
- `-fvisibility=hidden` — экспортируются только `extern "C"` точки входа

### 3.6 Порядок shutdown

```
simulation.stop()
manager.stopAll()         // обратный порядок компонентов
monitorLoader.unloadAll() // обратный порядок плагинов
iecPlugin->stop()
destroy(iecPlugin)
dlclose(iecHandle)
```

---

## 4. Инварианты реализованных классов

### `ProcessImage`
- Размер фиксирован: `PROCESS_IMAGE_SIZE = 256` байт для каждого из трёх разделов (inputs, outputs, memory)
- Zero-initialized при создании
- Не содержит синхронизации — доступ только из одного потока (tick-loop)
- **НЕ является Singleton** — создаётся на стеке в `main()`, передаётся по ссылке

### `IoBuffer`
- **Thread-safe**: все операции через `std::mutex` (по одному на inputs и outputs)
- Shadow-буфер: IO-поток пишет в shadow, tick-loop копирует атомарно
- Каждая `IEC_BOOL` занимает **один байт** (0 или 1)
- Адресация: `index = byteAddr * 8 + bitAddr`

### `IecRawBuffer`
- Ссылка на `ProcessImage` — не владеет памятью
- Раскладка типов в ProcessImage строго фиксирована:
  ```
  [0..63]    BOOL  (64 переменных, по 1 байту)
  [64..95]   BYTE  (32 переменных)
  [96..127]  INT   (16 × 2 байта)
  [128..159] DINT  (8 × 4 байта)
  [160..191] LINT  (4 × 8 байт)
  ```
- `static_assert(TOTAL_BYTES <= PROCESS_IMAGE_SIZE)` — компилятор гарантирует непереполнение

### `MatiecLoader`
- Владеет `dlopen` handle — RAII через деструктор
- Состояние: либо полностью загружен (все обязательные символы разрешены), либо полностью выгружен
- `run()` — идемпотентен при `configRun_ == nullptr`
- Повторная загрузка без предварительной выгрузки запрещена (`if (handle_) return false`)

### `IecProgram`
- Владеет `MatiecLoader`
- Хранит и инкрементирует `tickCount_` — монотонно возрастающий счётчик
- Цикл tick: `copyInputsFromBuffer → run → syncOutputsToBuffer`
- **По диаграмме** должен также владеть `IecExecutionEngine`, `IecMemory`, `IecIoBinding` — **не реализовано**

### `Scheduler`
- Коллекция `Entry{tickable, intervalMs, lastTickMs}`
- `tick(nowMs)`: задача исполняется если `nowMs - lastTickMs >= intervalMs` или `intervalMs == 0`
- Не владеет `ITickable*` — наблюдатель
- **Диаграмма подтверждает**: `Scheduler` → `ITickable[]`

### `Task`
- Именованная группа `ITickable*`
- Последовательное исполнение units в порядке добавления
- Не владеет units
- **Реализует Composite**: сам является `ITickable`, содержит `ITickable[]`

### `TaskManager`
- Владеет `Task` через `std::unique_ptr`
- Имена задач уникальны — повторное создание бросает `std::runtime_error`
- `bindScheduler()` — отложенная регистрация: все Task регистрируются при вызове
- **Диаграмма подтверждает**: `TaskManager` → `Task[]`, пунктирная связь к `Scheduler` через `bindScheduler()`

### `ComponentManager`
- Init/start: прямой порядок
- Stop: **обратный порядок** (LIFO)
- Не владеет компонентами (`IComponent*`)
- **Диаграмма подтверждает**: `ComponentManager` → `IComponent[]`

### `SystemBus`
- Регистрация одного сервиса на тип — повторная регистрация бросает `std::runtime_error`
- `getService<T>()` бросает `std::runtime_error` если сервис не найден
- Не владеет сервисами (хранит `void*`)
- **Диаграмма подтверждает**: `map<std::type_index, T>`

### `WatchdogChecker`
- Thread-safe: все операции через `std::mutex`
- Первый tick задачи — калибровочный (не проверяется overrun)
- Overrun = `actual > expected + tolerance`

---

## 5. Стандарты C++ и правила именования

### Стандарт
- **C++20** (`CMAKE_CXX_STANDARD 20`, `CMAKE_CXX_STANDARD_REQUIRED ON`)
- Флаги: `-Wall -Wextra -Wpedantic`

### Именование

| Сущность | Стиль | Пример |
|---|---|---|
| Классы / Структуры | PascalCase | `ProcessImage`, `IecProgram`, `TaskManager` |
| Интерфейсы | `I` + PascalCase | `IComponent`, `ITickable`, `IScheduler`, `IIecRuntime` |
| Методы | camelCase | `addTickable()`, `loadProgram()`, `copyInputsToProcessImage()` |
| Приватные поля | camelCase + `_` | `scheduler_`, `handle_`, `tickCount_` |
| Константы / constexpr | UPPER_SNAKE_CASE | `PROCESS_IMAGE_SIZE`, `BOOL_OFFSET`, `GLUE_BUFFER_SIZE` |
| Namespace | PascalCase | `MotorIO`, `MotorTaskIO`, `MatiecSymbols` |
| Файлы .hpp/.cpp | PascalCase | `ComponentManager.hpp`, `IecProgram.cpp` |
| Plugin exports | camelCase extern "C" | `createPlugin`, `destroyPlugin`, `setProcessImage` |
| Guard | `#pragma once` | Повсеместно |

### Организация кода

- Header-only: `ProcessImage.hpp`, `IoBuffer.hpp`, `SystemBus.hpp`, `IecRawBuffer.hpp`, все интерфейсы
- Разделение .hpp/.cpp: все классы с нетривиальной реализацией
- Плагины в `plugins/<name>/include` + `plugins/<name>/src`
- Симуляция в `simulation/`

---

## 6. Циклограмма (Timing)

### Механизм выдерживания цикла

```cpp
using clock = std::chrono::steady_clock;
const ms tickInterval{10};       // базовый период
auto nextTick = clock::now() + tickInterval;

while (g_running) {
    auto nowMs = ...;
    ioBuffer.copyInputsToProcessImage(processImage);   // 1. Read inputs
    scheduler->tick(nowMs);                              // 2. Execute tasks
    ioBuffer.copyOutputsFromProcessImage(processImage); // 3. Write outputs
    std::this_thread::sleep_until(nextTick);            // 4. Wait
    nextTick += tickInterval;                            // 5. Advance
}
```

### Диаграмма одного цикла (10 ms base tick)

```
t=0ms        t=10ms       t=20ms       t=30ms
|            |            |            |
|-- tick ----|-- tick ----|-- tick ----|
|            |            |            |
| copyIn     | copyIn     | copyIn     |
| sched.tick | sched.tick | sched.tick |
|  ├ IecTask |  ├ (skip)  |  ├ IecTask | ← 20ms interval
|  └ (skip)  |  └ (skip)  |  └ (skip)  |
| copyOut    | copyOut    | copyOut    |
| sleep      | sleep      | sleep      |
```

### Scheduler внутренняя логика

```
Scheduler::tick(nowMs):
  for each Entry:
    if intervalMs == 0 OR (nowMs - lastTickMs >= intervalMs):
      entry.tickable->tick()
      entry.lastTickMs = nowMs
```

**Важно**: используется `std::chrono::steady_clock` (монотонные часы), не `system_clock`. Метод `sleep_until` + инкрементальный `nextTick` — drift compensation.

### Поток данных внутри IecProgram::tick()

```
1. loader_.copyInputsFromBuffer(rawBuf_)
   ├── bool_input[byte][bit] → *var = rawBuf[byte*8+bit]
   ├── byte_input memcpy
   ├── int_input memcpy
   ├── dint_input memcpy
   └── lint_input memcpy

2. loader_.run(tickCount_++)
   ├── updateTime()          // обновить __CURRENT_TIME
   └── config_run__(tick)    // исполнить ST-логику

3. loader_.syncOutputsToBuffer(rawBuf_)
   ├── dst[byte*8+bit] = *boolOutput[byte][bit]
   ├── byte_output memcpy
   ├── int_output memcpy
   ├── dint_output memcpy
   └── lint_output memcpy
```

---

## 7. Типы данных: соответствие IEC 61131-3 ↔ C++

| IEC 61131-3 | C++ (Matiec) | Размер | Раздел в ProcessImage |
|---|---|---|---|
| `BOOL` | `uint8_t` (IEC_BOOL) | 1 byte (0/1) | `[0..63]` |
| `BYTE` | `uint8_t` (IEC_BYTE) | 1 byte | `[64..95]` |
| `INT` | `int16_t` (IEC_INT) | 2 bytes | `[96..127]` |
| `DINT` | `int32_t` (IEC_DINT) | 4 bytes | `[128..159]` |
| `LINT` | `int64_t` (IEC_LINT) | 8 bytes | `[160..191]` |
| `REAL` | `float` | 4 bytes | Не реализовано в `IecRawBuffer` |

**Критически важно**: в Matiec `IEC_BOOL` = **1 байт** (не 1 бит). Каждая BOOL-переменная занимает отдельный байт со значением 0 или 1.

### Планируемые IEC типы (из `iec_types.md`)

Система типов `IecType` должна поддерживать встроенные типы как **глобальные неизменяемые дескрипторы**:
```cpp
BuiltinTypes::BOOL
BuiltinTypes::INT
BuiltinTypes::REAL
```

### Адресация IEC → ProcessImage

```
%IX<byte>.<bit> → ProcessImage.inputs[byte * 8 + bit]
%QX<byte>.<bit> → ProcessImage.outputs[byte * 8 + bit]
%MX<byte>.<bit> → ProcessImage.memory[byte * 8 + bit]
```

### Matiec glueVars layout

```c
IEC_BOOL *bool_input[N][8];   // указатели на переменные внутри .so
IEC_BOOL *bool_output[N][8];
```

Это массив **указателей** — `MatiecLoader` разыменовывает их при копировании.

---

## 8. Механизм I/O (Image Table)

### Двухуровневая буферизация

```
IO Thread (MotorSimulation)
    │
    ▼
┌──────────┐   mutex   ┌──────────────┐   memcpy   ┌──────────────┐
│ Physical │ ────────► │   IoBuffer   │ ──────────► │ ProcessImage │
│   I/O    │           │ (shadow buf) │             │ (tick-loop)  │
└──────────┘           └──────────────┘             └──────────────┘
                                                           │
                                                    ┌──────┴──────┐
                                                    │ IecRawBuffer │
                                                    │  (adapter)   │
                                                    └──────┬──────┘
                                                           │
                                                    ┌──────┴──────┐
                                                    │ MatiecLoader │
                                                    │  glueVars    │
                                                    └─────────────┘
```

### Порядок операций в главном цикле

1. `ioBuffer.copyInputsToProcessImage(processImage)` — атомарный memcpy под mutex
2. `scheduler->tick(nowMs)` — IEC-программа читает/пишет ProcessImage
3. `ioBuffer.copyOutputsFromProcessImage(processImage)` — атомарный memcpy под mutex

### Гарантии

- IO-поток **никогда** не обращается к `ProcessImage` напрямую
- tick-loop **никогда** не обращается к `IoBuffer.shadow` без memcpy
- Во время исполнения IEC-программы входы/выходы **стабильны** (snapshot semantics)

---

## 9. Механизм выделения памяти

### Стратегия

| Компонент | Аллокация | Владение |
|---|---|---|
| `ProcessImage` | Stack (в `main`) | `main` |
| `IoBuffer` | Stack (в `main`) | `main` |
| `Task` | Heap (`make_unique`) | `TaskManager` |
| `IecProgram` | Heap (`make_unique`) | `IecRuntimePlugin` |
| `Scheduler`, `TaskManager` | Inline в Component | Соответствующий Component |
| `.so` handles | `dlopen` / `dlclose` | `MatiecLoader` или `main` |
| Сервисы в SystemBus | Указатели (no ownership) | Создатель |

### Принципы

- **Никаких глобальных аллокаций** в tick-loop (hot path)
- `ProcessImage` — фиксированный размер на стеке, без динамической памяти
- Все `std::vector` заполняются **до** start фазы
- Единственное исключение: `g_processImage` — глобальный указатель в плагине IEC Runtime (передаётся из main через `setProcessImage`)

### Планируемая модель памяти IEC (из `iec_memory.md`)

Каждый `IecProgram` должен владеть `IecMemory`, содержащей:
- `VAR` — внутренние переменные (состояние между циклами)
- `VAR_INPUT` — входные (обновляются до исполнения)
- `VAR_OUTPUT` — выходные (обновляются при исполнении)

Память строго локальна для Program — разделение между Program запрещено.

### Потенциальные проблемы

- `std::string` в `Task::name_` — heap allocation при создании, но только в фазе init
- `std::unordered_map` в `SystemBus` и `WatchdogChecker` — dynamic allocation, но только в init/registration фазе
- В hot path (tick) аллокаций **нет**

---

## 10. Обработка ошибок

### Текущая модель

| Ситуация | Обработка |
|---|---|
| Сервис не найден в SystemBus | `std::runtime_error` (throw) |
| Дублирование сервиса | `std::runtime_error` (throw) |
| `dlopen` failure | Возврат `false` / `nullptr` + stderr |
| Обязательный символ не найден | `MatiecLoader::load()` → `false`, unload |
| Опциональный символ не найден | Предупреждение в stdout, продолжение |
| Plugin createPlugin() = null | stderr + `return 1` |
| Task не найден | `std::runtime_error` |
| Tick overrun | Callback через `WatchdogChecker`, не прерывает исполнение |
| SIGINT | `g_running = false`, graceful shutdown |

### Планируемая диагностика (из `iec_diagnostics.md`)

`IecDiagnostics` должен отслеживать состояние через `IecRuntimeState`:
- `Idle` — Program не выполняется
- `Running` — выполняется цикл
- `Error` — зафиксирована ошибка

Интеграция через `IecExecutionContext`:
```
Scheduler → Task → IecProgram → ExecutionEngine → ExecutionContext → Diagnostics
```

### Отсутствует (на текущем этапе)

- Recovery / restart при ошибках
- Error codes вместо exceptions на hot path
- Structured logging ошибок
- Fallback при недоступности IO
- Graceful degradation при overrun
- Rollback при частичной инициализации

---

## 11. Зависимости

### Системные

| Библиотека | Тип | Назначение |
|---|---|---|
| `libdl` (`-ldl`) | Shared | `dlopen`, `dlsym`, `dlclose` — загрузка плагинов и IEC-программ |
| `libpthread` (`-lpthread`) | Shared | `std::thread`, `std::mutex` |
| POSIX signals | System | `SIGINT` handling |

### Стандартная библиотека C++20

- `<chrono>` — тайминг (`steady_clock`, `sleep_until`)
- `<thread>` — IO thread, sleep
- `<mutex>` — `IoBuffer`, `WatchdogChecker`
- `<atomic>` — `g_running`, `MotorSimulation::running_`
- `<functional>` — callbacks
- `<memory>` — `unique_ptr`
- `<optional>` — `WatchdogChecker` return type
- `<typeindex>` — `SystemBus` type-safe lookup

### Внешние (runtime)

| Артефакт | Формат | Назначение |
|---|---|---|
| `libst_logic.so` | Matiec-compiled shared library | Пользовательская ST-программа |
| `libiec_runtime.so` | Plugin shared library | IEC Runtime plugin |
| `libsystem_monitor.so` | Plugin shared library | System Monitor plugin |

### Header-only (в проекте)

- `ProcessImage.hpp`, `IoBuffer.hpp`, `SystemBus.hpp`, `IecRawBuffer.hpp`
- Все интерфейсы: `IComponent.hpp`, `IScheduler.hpp`, `IPlugin.hpp`, `ITaskManager.hpp`, `IIoSystem.hpp`, `IIecRuntime.hpp`, `ISystemMonitor.hpp`

---

## 12. Основные ошибки, которых следует избегать

### 12.1 Нарушение порядка инициализации IEC Runtime

```
❌ createPlugin() → setProcessImage()    // g_processImage == nullptr
✅ setProcessImage() → createPlugin()    // OK
```

### 12.2 Доступ к ProcessImage из IO-потока

```
❌ simulation thread: processImage.inputs[0] = 1;
✅ simulation thread: ioBuffer.setBoolInput(0, 0, true);
```

### 12.3 Адресация: путаница byte/bit

```
❌ processImage.inputs[0]  // "это %IX0.0"  — только если BOOL
✅ index = byteAddr * 8 + bitAddr  // %IX0.0 → inputs[0], %IX0.1 → inputs[1], %IX1.0 → inputs[8]
```

### 12.4 Блокирующие операции в tick()

```
❌ void MyTask::tick() { std::this_thread::sleep_for(100ms); }
❌ void MyTask::tick() { file.write(...); }
✅ void MyTask::tick() { /* pure computation on ProcessImage */ }
```

### 12.5 Регистрация дублирующих сервисов

```
❌ bus.registerService<IScheduler>(&sched1);
   bus.registerService<IScheduler>(&sched2);  // throws!
```

### 12.6 Забыть bindScheduler() перед start

`TaskManager::bindScheduler()` регистрирует все существующие Task в Scheduler. Без этого вызова задачи не будут исполняться.

### 12.7 Обращение к Matiec символам как к значениям, а не указателям

```c
// Matiec glueVars:
IEC_BOOL *bool_input[N][8];  // это УКАЗАТЕЛИ на переменные

❌ bool_input[0][0] = value;   // перезапись указателя!
✅ *bool_input[0][0] = value;  // запись значения
```

### 12.8 Утечка dlopen handle

Каждый `dlopen()` должен иметь соответствующий `dlclose()`. `MatiecLoader` делает это в деструкторе. Для IEC Runtime handle в `main.cpp` — manual management.

### 12.9 Нарушение порядка shutdown

Компоненты останавливаются в обратном порядке. Плагины выгружаются после `stopAll()`. `dlclose` для IEC runtime — последний.

### 12.10 Alignment при reinterpret_cast в IecRawBuffer

`reinterpret_cast<IEC_INT*>(image_.inputs + INT_OFFSET)` — корректен только если `INT_OFFSET` выровнен на `alignof(int16_t)`. Текущие offset'ы: `BOOL_OFFSET=0, BYTE_OFFSET=64, INT_OFFSET=96, DINT_OFFSET=128, LINT_OFFSET=160` — все кратны 8, alignment гарантирован.

### 12.11 Нарушение архитектурных границ (из диаграммы)

```
❌ Scheduler напрямую вызывает IecExecutionEngine
❌ IecMemory знает о TaskManager
❌ IecIoBinding создаёт Device
✅ Вся связь IEC ↔ Core только через IecProgram
✅ Вся связь IEC ↔ Hardware только через IecIoEndpoint
```

---

## 13. Соответствие текущей реализации требованиям заказчика

### Реализовано полностью

| Требование | Статус | Файлы |
|---|---|---|
| Component-Oriented Architecture | ✅ | `IComponent.hpp`, `ComponentManager.*` |
| SystemBus (Service Locator) | ✅ | `SystemBus.hpp` |
| Deterministic Scheduler | ✅ | `Scheduler.*`, `IScheduler.hpp` |
| Task Manager | ✅ | `TaskManager.*`, `ITaskManager.hpp` |
| Process Image | ✅ | `ProcessImage.hpp` |
| IO Buffer (thread-safe) | ✅ | `IoBuffer.hpp` |
| IEC Runtime Plugin | ✅ | `plugins/iec_runtime/` |
| Matiec integration (glueVars, config_init, config_run) | ✅ | `MatiecLoader.*` |
| IEC Program execution (tick-based) | ✅ | `IecProgram.*` |
| Dynamic loading .so (fPIC) | ✅ | `PluginLoader.*`, `MatiecLoader.*` |
| System Monitor / Watchdog | ✅ | `plugins/system_monitor/` |
| Plugin architecture | ✅ | `IPlugin.hpp`, `PluginLoader.*` |

### Реализовано частично / отличается от диаграммы

| Требование из ТЗ (.md) / Диаграммы | Текущий статус | Что есть | Что отсутствует |
|---|---|---|---|
| **IecProgram (полная структура)** | ⚠️ Частично | `MatiecLoader` + `IecRawBuffer` | Должен владеть `IecExecutionEngine`, `IecMemory`, `IecIoBinding` (диаграмма) |
| **IecExecutionEngine** | ⚠️ Matiec заменяет | `config_run__()` через MatiecLoader | Собственный engine с `IecExecutionContext`, `IecExpression[]`, `IecDiagnostics` |
| **IO Binding** | ⚠️ Прямой memcpy | `IecRawBuffer` + `MatiecLoader::copyInputs/syncOutputs` | Абстрактные `IecIoEndpoint`, `IecIoBinding`, `IecValueRef` |
| **System Layer / Devices** | ⚠️ Только концепция | `IIoSystem` интерфейс, `MotorSimulation` | `Device`, `DeviceManager`, `NullDevice`, fieldbus |
| **Configuration Layer** | ⚠️ Только документ | — | `SystemConfig`, `DeviceConfig`, `FieldbusConfig`, `ConfigApplier` |
| **Lifecycle Controller** | ⚠️ Фактически в main.cpp | Последовательность init→start→stop в `main.cpp` | `LifecyclePhase`, `LifecycleController`, state machine |

### Не реализовано (присутствует на диаграмме, но отсутствует в коде)

| Сущность из диаграммы | Документ ТЗ | Приоритет | Описание |
|---|---|---|---|
| **`EventManager`** + `EventManagerComponent` | `architecture.md` §8 | 🔴 Высокий | `map<EventType, IEventListener>` — типизированные синхронные события |
| **`LoggerComponent`** | `architecture.md` §7 | 🔴 Высокий | Мост Core↔System logging, регистрирует `ILogger` в SystemBus |
| **`IecExecutionEngine`** | `iec_execution.md` | 🟡 Средний | Хранит `IecExpression[]`, управляет `beginCycle/endCycle` |
| **`IecExecutionContext`** | `iec_execution.md` | 🟡 Средний | Содержит `IecDiagnostics` + `RuntimeContext` |
| **`IecDiagnostics`** | `iec_diagnostics.md` | 🟡 Средний | `IecRuntimeState` (Idle/Running/Error), cycle hooks |
| **`RuntimeContext`** | Диаграмма | 🟡 Средний | Часть `IecExecutionContext`, точка расширения |
| **`IecExpression`** / `IecAssignment` | `iec_expressions.md` | 🟡 Средний | Базовый интерфейс `evaluate()`, модель вычислений |
| **`IecMemory`** | `iec_memory.md` | 🟡 Средний | Агрегат VAR + VAR_INPUT + VAR_OUTPUT |
| **`IecVariable`** | `iec_memory.md`, `iec_types.md` | 🟡 Средний | Имя + `IecType` + `IecValueRef` |
| **`IecType`** / `BuiltinTypes` | `iec_types.md` | 🟡 Средний | Неизменяемые дескрипторы типов (BOOL, INT, REAL) |
| **`IecValue`** / `IecValueRef` | `iec_values.md` | 🟡 Средний | Хранение и ссылка на значение |
| **`IecIoBinding`** | `io_binding.md` | 🟡 Средний | Менеджер input/output bindings для Program |
| **`IecIoEndpoint`** | `io_binding.md` | 🟡 Средний | `read(IecValueRef)` / `write(IecValueRef)` |
| **`Device`** / `DeviceManager` | `system_layer.md` | 🟠 Ниже | Абстракция устройств, init/shutdown/name |
| **`FieldbusManager`** / `FieldbusConfig` | `system_layer.md`, `configuration.md` | 🟠 Ниже | Коммуникационные шины |
| **`LifecycleController`** / `LifecyclePhase` | `lifecycle.md` | 🟠 Ниже | State machine фаз системы |
| **`SystemConfig`** / `ConfigApplier` | `configuration.md` | 🟠 Ниже | Декларативное описание + применение |

---

## 14. Дорожная карта развития

### Фаза 1: Core Runtime — заполнение пробелов диаграммы (🔴 Высокий приоритет)

```
1.1 EventManager + EventManagerComponent
    Файлы: core/Event.hpp, core/IEventListener.hpp,
            core/EventManager.hpp, core/EventManager.cpp,
            core/EventManagerComponent.hpp, core/EventManagerComponent.cpp
    - map<EventType, vector<IEventListener*>>
    - Синхронная доставка (в tick-потоке)
    - Регистрация в SystemBus как IEventManager
    - Регистрация в ComponentManager

1.2 LoggerComponent
    Файлы: core/ILogger.hpp, core/LoggerComponent.hpp, core/LoggerComponent.cpp,
            system/ISystemLog.hpp, system/posix/PosixLog.hpp, system/posix/PosixLog.cpp
    - ISystemLog → PosixLog (stdout/stderr)
    - LoggerComponent: мост Core↔System
    - Замена всех std::cout на bus.getService<ILogger>()
```

### Фаза 2: IEC Type System + Memory Model (🟡 Средний приоритет)

```
2.1 IecType + BuiltinTypes
    Файлы: plugins/iec_runtime/include/IecType.hpp
    - Неизменяемые дескрипторы: BOOL, INT, REAL, BYTE, DINT, LINT
    - Без RTTI, без dynamic_cast

2.2 IecValue + IecValueRef
    Файлы: plugins/iec_runtime/include/IecValue.hpp
    - Базовый класс + конкретные (IecBoolValue, IecIntValue, ...)
    - IecValueRef — non-owning reference

2.3 IecVariable
    Файлы: plugins/iec_runtime/include/IecVariable.hpp
    - name + IecType& + IecValueRef

2.4 IecMemory
    Файлы: plugins/iec_runtime/include/IecMemory.hpp,
            plugins/iec_runtime/src/IecMemory.cpp
    - Агрегация: vars_, inputVars_, outputVars_
    - Владение IecVariable через unique_ptr
    - Интеграция с IecProgram (IecProgram владеет IecMemory)
```

### Фаза 3: IEC Execution Engine + Diagnostics (🟡 Средний приоритет)

```
3.1 IecExpression + IecAssignment
    Файлы: plugins/iec_runtime/include/IecExpression.hpp
    - virtual void evaluate() = 0
    - IecAssignment: target := source через IecValueRef

3.2 IecDiagnostics
    Файлы: plugins/iec_runtime/include/IecDiagnostics.hpp,
            plugins/iec_runtime/src/IecDiagnostics.cpp
    - IecRuntimeState enum {Idle, Running, Error}
    - beginCycle() / endCycle() / setError()

3.3 IecExecutionContext
    Файлы: plugins/iec_runtime/include/IecExecutionContext.hpp
    - Содержит IecDiagnostics + RuntimeContext
    - begin/end hooks

3.4 IecExecutionEngine
    Файлы: plugins/iec_runtime/include/IecExecutionEngine.hpp,
            plugins/iec_runtime/src/IecExecutionEngine.cpp
    - vector<IecExpression*>
    - beginCycle → evaluate(expr[i]) → endCycle
    - Интеграция: IecProgram владеет IecExecutionEngine

3.5 Рефакторинг IecProgram
    - IecProgram получает: IecExecutionEngine, IecMemory, IecIoBinding
    - MatiecLoader интегрируется через IecExecutionEngine
    - tick(): readInputs → engine.execute(context) → writeOutputs
```

### Фаза 4: IO Binding абстракция (🟡 Средний приоритет)

```
4.1 IecIoEndpoint interface
    Файлы: plugins/iec_runtime/include/IecIoEndpoint.hpp
    - virtual read(IecValueRef) / write(IecValueRef)

4.2 IecIoBinding manager
    Файлы: plugins/iec_runtime/include/IecIoBinding.hpp,
            plugins/iec_runtime/src/IecIoBinding.cpp
    - addInputBinding(IecVariable&, IecIoEndpoint&)
    - addOutputBinding(IecVariable&, IecIoEndpoint&)
    - readInputs() / writeOutputs()

4.3 ProcessImageEndpoint
    - Реализация IecIoEndpoint поверх ProcessImage + IecRawBuffer
    - Адаптер: текущий memcpy оборачивается в endpoint
```

### Фаза 5: System Layer (🟠 Ниже)

```
5.1 Device + DeviceManager + NullDevice
5.2 FieldbusManager + FieldbusConfig
5.3 Реальные IecIoEndpoint (GPIO, SPI/I2C, Modbus, EtherCAT)
```

### Фаза 6: Configuration + Lifecycle (🟠 Ниже)

```
6.1 SystemConfig + DeviceConfig + FieldbusConfig structs
6.2 ConfigApplier: config → DeviceManager + FieldbusManager
6.3 LifecyclePhase enum + LifecycleController
6.4 Вынос orchestration из main.cpp в LifecycleController
6.5 JSON/YAML parser integration
```

### Фаза 7: Robustness + Production

```
7.1 Error handling (error codes на hot path)
7.2 Structured logging через ILogger
7.3 Retain / persistent memory (%M*)
7.4 Online change preparation
7.5 Real-time (mlockall, SCHED_FIFO)
7.6 Remote monitoring / OPC-UA
```

---

## Appendix A: Верификация диаграммы классов vs. код

| Элемент диаграммы | В коде | Соответствие |
|---|---|---|
| `ComponentManager` → `IComponent[]` | ✅ `vector<IComponent*>` | Полное |
| `SchedulerComponent` → `ComponentManager` | ✅ Регистрируется через `registerComponent()` | Полное |
| `TaskManagerComponent` → `ComponentManager` | ✅ Регистрируется через `registerComponent()` | Полное |
| `LoggerComponent` → `ComponentManager` | ❌ Не реализован | **Missing** |
| `EventManagerComponent` → `ComponentManager` | ❌ Не реализован | **Missing** |
| `EventManager` (`map<EventType, IEventListener>`) | ❌ Не реализован | **Missing** |
| `SystemBus` (`map<type_index, T>`) | ✅ `unordered_map<type_index, void*>` | Полное |
| `Scheduler` → `ITickable[]` | ✅ `vector<Entry>` | Полное |
| `TaskManager` → `Task[]` | ✅ `vector<unique_ptr<Task>>` | Полное |
| `Task` → `ITickable[]` | ✅ `vector<ITickable*>` | Полное |
| `TaskManager` ⇢ `Scheduler` (dashed) | ✅ `bindScheduler()` | Полное |
| `IecProgram` ← `ITickable` | ✅ Наследует `ITickable` | Полное |
| `IecProgram` ← `TaskManager` (dashed) | ✅ Через `addToTask()` | Полное |
| `IecProgram` → `IecExecutionEngine` | ❌ Не реализовано (используется MatiecLoader) | **Diverged** |
| `IecExecutionEngine` → `IecExecutionContext` | ❌ Не реализовано | **Missing** |
| `IecExecutionContext` → `IecDiagnostics` | ❌ Не реализовано | **Missing** |
| `IecExecutionContext` → `RuntimeContext` | ❌ Не реализовано | **Missing** |
| `IecExecutionEngine` → `IecExpression[]` | ❌ Не реализовано | **Missing** |
| `IecProgram` → `IecIoBinding` | ❌ Не реализовано (прямой memcpy) | **Diverged** |
| `IecIoBinding` → `IecIoEndpoint` | ❌ Не реализовано | **Missing** |
| `IecIoEndpoint` → `IecValueRef` | ❌ Не реализовано | **Missing** |
| `IecProgram` → `IecMemory` | ❌ Не реализовано | **Missing** |
| `IecMemory` → `IecVariable[]` | ❌ Не реализовано | **Missing** |
| `IecVariable` → `IecType` | ❌ Не реализовано | **Missing** |
| `IecVariable` → `IecValueRef` (dashed) | ❌ Не реализовано | **Missing** |
| `IecValueRef` → `IecValue` | ❌ Не реализовано | **Missing** |

### Итого

- **Полное соответствие**: 10 из 27 элементов (верхняя часть диаграммы — Core Runtime)
- **Diverged** (реализовано иначе): 2 элемента (`IecProgram`→`IecExecutionEngine`, `IecProgram`→`IecIoBinding`)
- **Missing**: 15 элементов (вся нижняя часть диаграммы — IEC Runtime Layer)

**Вывод**: Core Runtime Layer полностью соответствует диаграмме. IEC Runtime Layer реализован как прямой мост к Matiec (`MatiecLoader`), минуя планируемую абстрактную модель. Фазы 2–4 дорожной карты направлены на устранение этого расхождения.

---

## Appendix B: Обязательные паттерны (из диаграммы)

Следующие паттерны зафиксированы диаграммой как **архитектурный стандарт** и должны соблюдаться при развитии кода:

| Паттерн | Где применяется | Обязательность |
|---|---|---|
| **Observer** | `EventManager` → `IEventListener`; `WatchdogChecker` → `WatchdogCallback`; будущие Alarms | Обязателен для всех event/notification подсистем |
| **Composite** | `Task` → `ITickable[]` (Task сам является ITickable) | Обязателен: задачи компонуются |
| **Strategy** | `ITickable::tick()` — единый интерфейс исполнения | Обязателен: любая исполняемая единица = ITickable |
| **Mediator** | `SystemBus` — центральный реестр сервисов | Обязателен: компоненты не знают друг о друге |
| **Adapter** | `IecRawBuffer` (ProcessImage → Matiec types); будущий `ProcessImageEndpoint` | Обязателен на границах слоёв |
| **RAII** | `MatiecLoader` (`dlopen`/`dlclose`); `PluginLoader`; `unique_ptr<Task>` | Обязателен для всех ресурсов |
| **Non-Singleton** | `ProcessImage` — **не** Singleton, создаётся на стеке и передаётся по ссылке | Обязателен: глобальное состояние запрещено |

---

## Appendix C: Сборка и запуск

```bash
# Основной бинарник
mkdir build && cd build
cmake .. && make

# Плагин IEC Runtime
cd plugins/iec_runtime
mkdir build && cd build
cmake .. && make
cp libiec_runtime.so ../../../build/

# Плагин System Monitor
cd plugins/system_monitor
mkdir build && cd build
cmake .. && make
cp libsystem_monitor.so ../../../build/

# Компиляция ST программы (Matiec)
iec2c -I /path/to/matiec/lib motor_control.st
gcc -shared -fPIC -o libst_logic.so POUS.c Res0.c Config0.c ...
cp libst_logic.so build/

# Запуск
cd build
./system
```

## Appendix D: Карта адресов для примера MotorControl

```
INPUTS (ProcessImage.inputs[]):
  %IX0.0 → inputs[0]  = btnStart
  %IX0.1 → inputs[1]  = btnStop
  %IX0.2 → inputs[2]  = fault

OUTPUTS (ProcessImage.outputs[]):
  %QX1.0 → outputs[8]  = motorRun
  %QX1.1 → outputs[9]  = lampGreen
  %QX1.2 → outputs[10] = lampRed
```