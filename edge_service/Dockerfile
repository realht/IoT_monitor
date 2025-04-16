# edge_service/Dockerfile
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

# Копируем только необходимые файлы
# Явно копируем файлы из корня проекта
COPY ./conanfile.txt .
COPY ./CMakeLists.txt .
COPY ./proto ./proto
COPY ./parse ./parse
COPY ./config ./config
COPY ./analytics_service ./analytics_service
COPY ./edge_service ./edge_service

# Устанавливаем зависимости
RUN conan install . --build=missing -s compiler.cppstd=20
RUN cmake . -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
RUN cmake --build . --target edge_service -j$(nproc)

# Проверка наличия бинарника
RUN ls -l edge_service

FROM ubuntu:22.04 AS final
COPY --from=builder /app/edge_service /usr/local/bin/

RUN chmod +x /usr/local/bin/edge_service

RUN ls -l /usr/local/bin/

CMD ["/usr/local/bin/edge_service"]
