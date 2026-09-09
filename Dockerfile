FROM gcc:14 AS build

WORKDIR /app

COPY . .

RUN gcc \
    -std=c11 \
    -Wall \
    -Wextra \
    -Wpedantic \
    -pthread \
    -Iinclude \
    src/main.c \
    src/server.c \
    src/protocol.c \
    src/state.c \
    -o cci-server


FROM debian:bookworm-slim

WORKDIR /app

COPY --from=build /app/cci-server ./cci-server

EXPOSE 9000

CMD ["./cci-server", "9000"]