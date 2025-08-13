# 🌐 Real‑Time IoT Telemetry Platform

![License](https://img.shields.io/badge/license-MIT-green.svg)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![gRPC](https://img.shields.io/badge/gRPC-enabled-ff69b4.svg)
![Docker](https://img.shields.io/badge/Docker-ready-blue.svg)

> **Распределённая система сбора, агрегации и анализа телеметрии от IoT‑устройств в реальном времени**  
> Поддержка потокового взаимодействия, алертов и мониторинга через Prometheus + Grafana.

---

## 📑 Оглавление

- [Описание](#-описание)
- [Архитектура](#-архитектура)
- [Ключевые возможности](#-ключевые-возможности)
- [Технологический стек](#-технологический-стек)
- [Структура репозитория](#-структура-репозитория)
- [Переменные окружения](#-переменные-окружения)
- [Зависимости](#-зависимости)
- [Проверка окружения](#-проверка-окружения)
- [Быстрый старт](#-быстрый-старт)
- [Примеры API](#-примеры-api)
- [Мониторинг и алерты](#-мониторинг-и-алерты)
- [Лицензия](#-лицензия)

---

## 📝 Описание

**Real-Time IoT Telemetry Platform** — набор C++20-сервисов для приёма и анализа данных от IoT-датчиков (температура, влажность и т.д.) в реальном времени.  
Потоки телеметрии принимаются по **gRPC streaming**, шардируются и буферизуются в **Redis Streams**. Аналитический сервис читает из consumer-groups, агрегирует метрики (avg/min/max/stddev), генерирует алерты и экспонирует метрики для **Prometheus**, визуализация — в **Grafana**.

---

## 🏗 Архитектура

```mermaid
flowchart TD
    subgraph Devices["IoT Devices"]
        D1[Sensor 1]
        D2[Sensor 2]
        Dn[Sensor N]
    end

    subgraph EdgeService["Edge Service (gRPC)"]
        E1[[StreamTelemetry API]]
        E2[Batch & Validate]
        E3[Redis Streams Producer]
    end

    subgraph Redis["Redis Streams"]
        R1[(telemetry_stream)]
    end

    subgraph AnalyticsService["Analytics Service"]
        A1[Consumer Group Reader]
        A2[Sliding Window Aggregator]
        A3[Alert Manager]
        A4[Prometheus Exporter]
    end

    subgraph Monitoring["Monitoring Stack"]
        M1[Prometheus]
        M2[Grafana Dashboard]
        M3[Loki + Promtail]
    end

    Devices -->|gRPC streaming| E1 --> E2 --> E3 --> R1
    R1 --> A1 --> A2 --> A4 --> M1
    A1 --> A3
    M1 --> M2
    EdgeService -. logs .-> M3
    AnalyticsService -. logs .-> M3
```

---

## 🚀 Ключевые возможности

- 📡 Высокопроизводительный приём потоков телеметрии по **gRPC streaming**
- ⚡ Шардирование и батч-запись в **Redis Streams** (pipeline) для снижения overhead
- 📊 Онлайн-агрегации (avg/min/max/stddev) и алерты по порогам
- 📈 Экспорт метрик в **Prometheus**, готовые дашборды в **Grafana**
- 🐳 Контейнеризация через **Docker / Docker Compose**
- 📝 Централизованное логирование через **Loki + Promtail**, структурированные логи (spdlog)

---

## 🛠 Технологический стек

- **Язык:** C++20
- **Сетевое взаимодействие:** gRPC, Protobuf
- **Очередь/буферизация:** Redis Streams (redis-plus-plus, hiredis)
- **Метрики:** Prometheus C++ Client
- **Логирование:** spdlog, Loki + Promtail
- **Мониторинг:** Prometheus, Grafana
- **Сборка:** CMake, Conan
- **Инфраструктура:** Docker, docker-compose

---

## 🗂 Структура репозитория

- edge_service/ — асинхронный gRPC-сервер, шардинг, батч-производитель Redis Streams
- analytics_service/ — consumer-groups, агрегатор окон, алерты, экспорт метрик
- proto/ — protobuf-схемы и (при необходимости) сгенерированный код
- prometheus_metrics/ — регистрация и описание метрик
- logger_factory/ — фабрика логгеров (spdlog)
- monitoring/ — конфиги Prometheus/Grafana/Loki/Promtail, дашборды
- docker-compose.yml, Dockerfile* — контейнеризация

---

## ⚙ Переменные окружения

| Переменная                 | Описание                       | Значение по умолчанию         |
|----------------------------|--------------------------------|-------------------------------|
| REDIS_HOST                 | URI/адрес Redis                | tcp://redis:6379              | 
| STREAM_NAME                | Имя Redis Stream               | telemetry_stream              | 
| CONSUMER_GROUP             | Группа потребителей            | analytics_group               | 
| EDGE_SERVICE_ADDRESS       | Адрес Edge Service             | 0.0.0.0:50051                 | 
| ANALYTICS_SERVICE_ADDRESS  | Адрес Analytics Service        | 0.0.0.0:50052                 | 
| PROMETHEUS_ADDRESS         | HTTP endpoint метрик           | 0.0.0.0:8081                  | 
| BATCH_SIZE                 | Размер батча записи в Redis    | 100                           | 
| WRITE_TIMEOUT_MS           | Таймаут записи в Redis         | 10                            |               
| LOG_LEVEL                  | Уровень логирования            | info                          |
| LOG_DIR                    | Путь для логов (если включено) | ./logs                        |

> Значения можно переопределить в .env.

---

## 🔧 Зависимости

Перед запуском убедитесь, что у вас установлены следующие инструменты:

| Компонент          | Минимальная версия | Назначение                          | Установка                              |
|--------------------|--------------------|-------------------------------------|----------------------------------------|
| **Docker**         | 20.10+             | Контейнеризация и запуск сервисов   | [docker.com](https://www.docker.com/)  |
| **Docker Compose** | 1.29+              | Оркестрация многоконтейнерного окружения | входит в Docker Desktop           |
| **CMake**          | 3.20+              | Сборка C++‑проектов                 | `sudo apt install cmake` или [cmake.org](https://cmake.org/) |
| **Conan**          | 2.0+               | Управление зависимостями C++        | `pip install conan` или [conan.io](https://conan.io/) |
| **Python**         | 3.8+               | Примеры gRPC‑клиентов               | `sudo apt install python3`             |
| **pip**            | latest             | Установка Python‑библиотек          |                |

> 💡 Все основные зависимости уже включены в Docker‑контейнеры.  
> Но если вы хотите собирать проект локально — потребуется CMake и Conan.

---

## 🧪 Проверка окружения

```bash
# Проверка версий
docker --version
docker-compose --version
cmake --version
conan --version
python3 --version
```

---

## ⚡ Быстрый старт

```bash
# 1) Клонировать репозиторий
git clone https://github.com/realht/IoT_monitor.git
cd IoT_monitor

# 2) (Опционально) Подредактировать .env при необходимости
# REDIS_HOST=tcp://redis:6379
# EDGE_SERVICE_ADDRESS=0.0.0.0:50051
# ANALYTICS_SERVICE_ADDRESS=0.0.0.0:50052
# PROMETHEUS_ADDRESS=0.0.0.0:8081
# ... логирование

# 3) Запустить всю платформу
docker-compose up --build

```

- **Edge Service:** localhost:50051
- **Analytics Service:** localhost:50052
- **Prometheus:** http://localhost:9090
- **Grafana:** http://localhost:3000 (anonymous enabled)
- **Loki:** http://localhost:3100

---

## 📦 Локальная сборка без Docker

```bash
conan install . --build=missing -s compiler.cppstd=20
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

## 🔌 Примеры API

### 📚 Python‑библиотеки для примеров

Если вы хотите запустить примеры API‑клиентов на Python, установите:

```bash
pip install grpcio grpcio-tools protobuf
```

1) **Edge Service**: отправка телеметрии (gRPC stream)

```python
# examples/edge_client.py
import grpc, time
import edge_service_pb2 as pb
import edge_service_pb2_grpc as api
from google.protobuf.timestamp_pb2 import Timestamp

channel = grpc.insecure_channel("localhost:50051")
stub = api.EdgeServiceStub(channel)

def telemetry_stream():
    for i in range(10):
        ts = Timestamp(seconds=int(time.time()))
        yield pb.SensorData(
            device_id="sensor-1",
            temperature=25.5 + i,
            humidity=40.0 + i,
            timestamp=ts
        )
        time.sleep(0.5)

ack = stub.StreamTelemetry(telemetry_stream())
print("Server response:", ack)

```

2) **Analytics Service**: подписка на статистику (server streaming)

```python
# examples/analytics_stats_client.py
import grpc
import analytics_service_pb2 as pb
import analytics_service_pb2_grpc as api

channel = grpc.insecure_channel("localhost:50052")
stub = api.AnalyticsServiceStub(channel)

req = pb.StatRequest(
    device_id=["sensor-1"],
    aggregation=pb.StatRequest.AVG,
    window_seconds=30
)

for resp in stub.GetRealtimeStats(req):
    print("metrics:", dict(resp.metrics), "ts:", resp.timestamp)
```

3) **Analytics Service**: подписка на алерты

```python
# examples/analytics_alerts_client.py
import grpc
import analytics_service_pb2 as pb
import analytics_service_pb2_grpc as api

channel = grpc.insecure_channel("localhost:50052")
stub = api.AnalyticsServiceStub(channel)

sub = pb.AlertSubscription(
    device_ids=["sensor-1"],
    metric_types=["temperature"],
    severity_levels=["critical", "warning"]
)

for alert in stub.SubscribeToAlerts(sub):
    print(f"[ALERT] {alert.device_id} {alert.metric_type}={alert.current_value} severity={alert.severity}")
```

> Proto-файлы находятся в proto/.

---

## 📊 Мониторинг и алерты

- **Метрики Prometheus:** доступны по адресу http://localhost:8081/metrics
- **Grafana Dashboard:** автоматически разворачивается, доступен на http://localhost:3000
- **Loki:** централизованное логирование, просмотр через Grafana Explore

**Пример Grafana Dashboard:** ![Скриншот интерфейса](Grafan_Dashboard.png)

---
## 📜 Лицензия

Этот проект распространяется по лицензии MIT — свободной и открытой.  
Вы можете использовать, изменять и распространять код, при условии сохранения уведомления об авторских правах.  
Полный текст лицензии — в файле [LICENSE](LICENSE).

---
