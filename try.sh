#!/bin/bash

gcc monitor.c -o monitor
gcc directory.c -o directory


watched_dir="$1"

./monitor $watched_dir &

monitor_pid=$!

sleep 2

./directory $watched_dir

sleep 2

# Commands to test the monitor program
mkdir "$watched_dir/folder1"
echo "Content for f1.txt" > "$watched_dir/f1.txt"
echo "Content for f2.txt" > "$watched_dir/f2.txt"
ls "$watched_dir/folder1"
cat "$watched_dir/f1.txt"
echo "Hello" >> "$watched_dir/f2.txt"
touch "$watched_dir/f1.txt"
rm "$watched_dir/f1.txt"
chmod u=w "$watched_dir/folder1"
rm -r "$watched_dir/folder1"

# Wait for a bit to let the monitor log all changes
sleep 2

kill -s SIGTERM $monitor_pid

wait $monitor_pid

rm monitor directory
