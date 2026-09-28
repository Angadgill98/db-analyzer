#!/bin/bash

set -e

BASE_DIR="$HOME/dev/proj/dbanalyzer"

echo "=== Starting sender ==="
cd "$BASE_DIR/sender"
go run . &
SENDER_PID=$!

echo "Waiting for /tmp/go_agent.sock..."

while [ ! -S /tmp/go_agent.sock ]; do
    sleep 0.5
done

chmod 777 /tmp/go_agent.sock

echo "=== Sender socket ready ==="


echo "=== Building collector ==="
cd "$BASE_DIR/collector"
gcc collector.c -o collector -luv

echo "=== Starting collector ==="
./collector &
COLLECTOR_PID=$!

echo "Waiting for /tmp/my_socket..."

while [ ! -S /tmp/my_socket ]; do
    sleep 0.5
done

chmod 777 /tmp/my_socket

echo "=== Collector socket ready ==="


echo "=== Building PostgreSQL extension ==="
cd "$BASE_DIR/extension"

make
sudo make install

echo "=== Restarting PostgreSQL ==="
sudo systemctl restart postgresql

echo "=== Everything started successfully ==="

wait $SENDER_PID $COLLECTOR_PID