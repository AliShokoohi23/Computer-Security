#!/bin/bash

server_ip="192.168.237.128"

local_packets_file="/tmp/capture.pcap"
system_info_file="/tmp/system_info.json"
# Define static key and IV (32-byte key for AES-256 and 16-byte IV)
#key="vcwgAyxSnOGMXFRcqG4AEaQuooJkeTfQ"  # 64 hex characters
#iv="5183666c72eec9e4"  # 32 hex characters

send_system_info() {
    os_version=$(uname -a)
    processor_model=$(cat /proc/cpuinfo | grep 'model name' | head -1)
    current_time=$(date)
    disk_usage=$(df -h / | awk 'NR==2 {print $5}')
    memory_usage=$(free -h | grep Mem | awk '{print $3 "/" $2}')
    cpu_load=$(uptime | grep -oP '(?<=load average: ).*')
    running_processes=$(ps aux --no-heading | wc -l)
    installed_packages=$(dpkg -l | wc -l)  # or use rpm -qa | wc -l for RPM-based systems
    system_uptime=$(uptime -p)

	# Create the payload of collected data
	data=$( jq -n \
        --arg OS "$os_version" \
        --arg Processor "$processor_model" \
        --arg Time "$current_time" \
        --arg DiskUsage "$disk_usage" \
        --arg MemoryUsage "$memory_usage" \
        --arg CPULoad "$cpu_load" \
        --arg RunningProcesses "$running_processes" \
        --arg InstalledPackages "$installed_packages" \
        --arg Uptime "$system_uptime" \
        '$ARGS.named'
    )

	#echo "$data" > /tmp/data.json

	# Encrypt using openssl with the converted binary key and IV
	#openssl enc -aes-256-cfb -in /tmp/data.json -out /tmp/data.enc -k $key -iv $iv 


	# Base64 encode the IV and ciphertext
	# ciphertext_base64=$(base64 < /tmp/data.enc)

	# Create the JSON payload with the ciphertext (IV is known)
	#payload=$( jq -n \
	#    --arg iv "$iv" \
	#    --arg ciphertext "$ciphertext_base64" \
	#    '{iv: $iv, ciphertext: $ciphertext}' 
	#)

	# Send the data to the server via HTTP POST (using curl)
	curl -X POST -H "Content-Type: application/json" -d "$data" http://$server_ip:5000/upload
}

send_packet_info() {
    # Use tcpdump to capture packets
    sudo tcpdump -nn -i eth0 -c 10 -w $local_packets_file 2> /dev/null
    
    if [[ -s /tmp/capture.pcap ]]; then
    	packet_data=$(sudo tcpdump -nn -r $local_packets_file)
        

        # Parse the packet data to extract IPs and port numbers
        src_ip=$(echo "$packet_data" | grep -oP '(?<=IP )[0-9.]+')
        dst_ip=$(echo "$packet_data" | grep -oP '(?<= > )[0-9.]+')
        src_port=$(echo "$packet_data" | grep -oP '(?<=\.)([0-9]+)(?=:)' | head -1)
        dst_port=$(echo "$packet_data" | grep -oP '(?<=:)[0-9]+(?= )' | head -1)
        pid=$(ps -eo pid,comm | grep tcpdump | awk '{print $1}')

        # Create JSON payload
        packet_json=$( jq -n \
            --arg SrcIP "$src_ip" \
            --arg DstIP "$dst_ip" \
            --arg SrcPort "$src_port" \
            --arg DstPort "$dst_port" \
            --arg PID "$pid" \
            '{SrcIP: $SrcIP, DstIP: $DstIP, SrcPort: $SrcPort, DstPort: $DstPort, PID: $PID}'
        )

        # Encrypt packet data
        #encrypted_packet_data=$(echo "$packet_json" | openssl enc -aes-256-cbc -base64 -K "$key" -iv "$iv")

        # Send the packet data to the server
        curl -X POST -d "$packet_json" http://$server_ip:5000/upload_packets
    fi
    rm -f $local_packets_file
}

check_server_connection() {
    curl -s --head http://$server_ip:5000 > /dev/null
    return $?
}

while true; do
    echo "Checking server connection..."
    
    if check_server_connection; then
        echo "Server is reachable. Sending data..."

        # Send system information
        send_system_info

        # Send packet information if network activity is detected
        send_packet_info

    fi

    # Sleep for 60 seconds before the next iteration
    sleep 60
done 
