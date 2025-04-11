 # Стадия сборки
FROM ubuntu:22.04 AS builder

# Установка зависимостей с CMake
RUN apt update && apt install -y \
    build-essential \
    cmake \
    python3-pip \
    python3-venv \
    pkg-config \
    libssl-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

# Установка Conan 2.14.0
RUN pip install conan==2.14.0 && \
    conan profile detect

WORKDIR /app

# Копирование исходных файлов
COPY ./conanfile.txt .
COPY ./CMakeLists.txt .
COPY ./proto ./proto
COPY ./parse ./parse
COPY ./analytics_service ./analytics_service
COPY ./edge_service ./edge_service

# Установка зависимостей через Conan
RUN conan install . --build=missing -s compiler.cppstd=20
RUN cmake . -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
RUN cmake --build . --target analytics_service -j$(nproc)

# Проверка наличия бинарника
RUN ls -l analytics_service

# Стадия финального образа
FROM ubuntu:22.04 AS final
# Копирование исполняемого файла
COPY --from=builder /app/analytics_service /usr/local/bin/

CMD ["/usr/local/bin/analytics_service"]
