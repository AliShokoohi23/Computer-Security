import  { useState, useEffect } from 'react';
import Data from '../Models/data';
import PacketData from '../Models/packetData';

function DisplayData() {
    const [data, setData] = useState<Data[] | never[]>([]);
    const [packet, setPacket] = useState<PacketData[] | never[]>([]);

    // Fetch data from the Flask server
    useEffect(() => {
        fetch('http://localhost:5000/data')  // Replace with the actual IP of your Flask servers
            .then(response => response.json())
            .then(data => setData(data))
            .catch(error => console.error('Error fetching data:', error));
        
        fetch('http://localhost:5000/view_packets')
            .then(response => response.json())
            .then(packet => setPacket(packet))
            .then(response => console.log(response))
            .catch(error => console.error('Error fetching data:', error));
            console.log(packet)
            // console.log()
            
    }, []);

    return (
        <div>
            <h1>Victim Data</h1>
            <table border={1}>
                <thead>
                    <tr>
                            <>
                            <th>index</th>
                            <th>Timestamp</th>
                            <th>OS</th>
                            <th>Processor</th>
                            <th>Time</th>
                            <th>DiskUsage</th>
                            <th>MemoryUsage</th>
                            <th>CPULoad (Last 1 minute)</th>
                            <th>CPULoad (Last 5 minutes)</th>
                            <th>CPULoad (Last 15 minutes)</th>
                            <th>RunningProcesses</th>
                            <th>InstalledPackages</th>
                            <th>Uptime</th>
                            </>
                        
                    </tr>
                </thead>
                <tbody>
                    {data.map((data:Data, index:number) => (
                        <tr key={index}>
                            <td>{index}</td>
                            <td>{data.Timestamp}</td>
                            <td >{data.OS}</td>
                            <td >{data.Processor}</td>
                            <td >{data.Time}</td>
                            <td >{data.DiskUsage}</td>
                            <td>{data.MemoryUsage}</td>
                            <td>{data.CPULoad.split(",")[0]}</td>
                            <td>{data.CPULoad.split(",")[1]}</td>
                            <td>{data.CPULoad.split(",")[2]}</td>
                            <td>{data.RunningProcesses}</td>
                            <td>{data.InstalledPackages}</td>
                            <td>{data.Uptime}</td>
                        </tr>
                    ))}
                </tbody>
            </table>

            <h1>Packets Data</h1>
            <table border={1}>
                <thead>
                    <tr>
                            <>
                            <th>index</th>
                            <th>Timestamp</th>
                            <th>Src IP</th>
                            <th>Dest IP</th>
                            <th>Src Port</th>
                            {/* <th>Dest Port</th> */}
                            {/* <th>PID</th> */}
                            </>
                        
                    </tr>
                </thead>
                <tbody>
                    {packet.map((data:PacketData, index:number) => (
                        <tr key={index}>
                            <td>{index}</td>
                            <td>{data.Timestamp}</td>
                            <td >{data.SrcIP}</td>
                            <td >{data.DstIP}</td>
                            <td >{data.SrcPort}</td>
                            {/* <td >{data.DstPort}</td> */}
                            {/* <td>{data.PID}</td> */}
                        </tr>
                    ))}
                </tbody>
            </table>
        </div>
    );
}

export default DisplayData;
