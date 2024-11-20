#!/bin/bash

# Function to send a command to the server and print the response
send_cmd() {
    local cmd="$1"
    echo -e "$cmd" | nc localhost 5000
}

# Test cases
send_cmd "GET Super Mario"
send_cmd "SET Super Mario"
send_cmd "DEL Super"
send_cmd "GET Super"

send_cmd "SET Link Zelda"
send_cmd "GET Link"
send_cmd "DEL Link"
send_cmd "GET Link"

send_cmd "SET Pikachu Thunderbolt"
send_cmd "GET Pikachu"
send_cmd "DEL Pikachu"
send_cmd "GET Pikachu"

send_cmd "SET Pacman Ghosts"
send_cmd "SET Donkey Kong"
send_cmd "SET Sonic Rings"
send_cmd "GET Donkey"

send_cmd "DEL Donkey"
send_cmd "DEL Pacman"
send_cmd "DEL Sonic"