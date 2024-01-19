#include "ns3/boolean.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/core-module.h"
#include "ns3/double.h"
// #include "ns3/flow-monitor-helper.h"
// #include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/uinteger.h"
#include "ns3/wifi-helper.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-utils.h"
#include "ns3/yans-wifi-helper.h"

#include <iomanip>
// #include <tuple.h>
// #include <enum.h>

#define NO_PROPAGATION_MODEL -1
#define IP4_OVERHEAD 20
#define UDP_OVERHEAD 8
#define WIFI_OVERHEAD 28
#define PAYLOAD_SIZE 1450

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiExperiment");

double
rate_to_s(double rate_Mbps, uint32_t packetSize_bytes)
{
    return (packetSize_bytes * 8.0) / (rate_Mbps * 1000.0 * 1000.0);
}

void
exportMapToCSV(const std::map<Time, double>& map_time_to_x,
               const std::string& path,
               const std::string& valueName)
{
    std::ofstream file(path);

    // Write the header
    file << "Time," + valueName + "\n";

    // Write the data
    for (const auto& pair : map_time_to_x)
    {
        file << pair.first.GetSeconds() << "," << pair.second << "\n";
    }

    file.close();
}

// TODO: (optional) make this a template class
class rx_dr_utils
{
  public:
    std::map<Time, double> map_time_to_rxDr_bps;

    double get_avg_rx_dr()
    {
        double sum = 0.0;
        for (const auto& pair : map_time_to_rxDr_bps)
        {
            sum += pair.second;
        }
        return sum / map_time_to_rxDr_bps.size();
    }

    // void MonitorSnifferRxCallback(std::string context,
    //                               Ptr<const Packet> packet,
    //                               uint16_t channelFreqMhz,
    //                               WifiTxVector txVector,
    //                               MpduInfo aMpdu,
    //                               SignalNoiseDbm signalNoise,
    //                               uint16_t staId)
    // {
    //     static uint32_t num_packets = 0;
    //     static Time lastTime = Seconds(0); // Time of the last packet (initially 0)
    //     Time now = Simulator::Now();       // Current time
    //     Time interval = now - lastTime;    // Time interval between packets

    //     if (interval.GetSeconds() == 0) // handle first packet to avoid divide by zero
    //     {
    //         map_time_to_rxDr_bps[now] = 0;
    //         return;
    //     }

    //     if (num_packets > 40 && num_packets < 60)
    //         std::cout << "RxOkCallback: " << context << " " << packet->GetSize() << " bytes at "
    //                   << now.GetSeconds() << "s" << std::endl;
    //     if (num_packets > 40 && num_packets < 60)
    //         std::cout << "rate:" << packet->GetSize() << "*8/" << interval.GetSeconds() << "="
    //                   << packet->GetSize() / interval.GetSeconds() / 1000 / 1000 << "Mbps"
    //                   << std::endl;

    //     map_time_to_rxDr_bps[now] = packet->GetSize() * 8 / interval.GetSeconds();

    //     num_packets++;

    //     lastTime = now; // Update the time of the last packet

    //     // double rx_dr = txVector.GetTxMode().GetDataRate();
    //     // double rx_dr = txVector.GetTxMode().GetPhyRate();
    // }

    void RxOkCallback(std::string context,
                      Ptr<const Packet> packet,
                      double snr,
                      WifiMode mode,
                      enum WifiPreamble preamble)
    {
        // double rx_dr = mode.GetPhyRate();
        // // double rx_dr = mode.GetDataRate();
        static Time lastTime = Seconds(0); // Time of the last packet (initially 0)
        Time now = Simulator::Now();       // Current time
        Time interval = now - lastTime;    // Time interval between packets

        if (interval.GetSeconds() == 0) // handle first packet to avoid divide by zero
        {
            map_time_to_rxDr_bps[now] = 0;
            return;
        }

        WifiMacHeader macHeader;
        packet->PeekHeader(macHeader);
        static uint32_t aggregated_size = 0;
        static Time aggregated_start_time = Time(0);
        static uint16_t last_sequnce_number = 0;
        uint16_t sequence_number = macHeader.GetSequenceNumber();
        bool isFirstInMpdu = aggregated_start_time.IsZero() && macHeader.IsQosData();
        bool isParrtOfMpdu = sequence_number == last_sequnce_number && macHeader.IsQosData();

        if (isFirstInMpdu || isParrtOfMpdu)
        {
            // This packet is part of an A-MPDU
            aggregated_size += packet->GetSize() - IP4_OVERHEAD - UDP_OVERHEAD;
            if (aggregated_start_time.IsZero())
            {
                // This is the first packet in the A-MPDU
                aggregated_start_time = Simulator::Now();
            }
        }
        else
        {
            // This packet is not part of an A-MPDU
            if (!aggregated_start_time.IsZero())
            {
                // The previous packet was part of an A-MPDU
                // Calculate the data rate for the A-MPDU
                Time aggregate_interval = Simulator::Now() - aggregated_start_time;
                double dataRate = aggregated_size * 8 / aggregate_interval.GetSeconds();
                map_time_to_rxDr_bps[aggregated_start_time] = dataRate;

                // Reset the aggregated size and start time
                aggregated_size = 0;
                aggregated_start_time = Time(0);
            }

            // Calculate the data rate for this packet
            double dataRate =
                ((packet->GetSize() * 8) - IP4_OVERHEAD - UDP_OVERHEAD) / interval.GetSeconds();
            map_time_to_rxDr_bps[Simulator::Now()] = dataRate;
        }
        lastTime = now; // Update the time of the last packet
        if (macHeader.IsQosData())
        {
            last_sequnce_number =
                sequence_number; // Update the sequence number of aggregate packets
        }

        // uint32_t totalSize =
        //   packet->GetSize(); // Get the total size of the packet in bytes
        // uint32_t headerSize =
        //   +IP4_OVERHEAD + UDP_OVERHEAD; // Size of UDP header + size of IPv4
        //   header
        // uint32_t payloadSize = totalSize - headerSize; // Size of the UDP payload
        // // uint32_t payloadSize = PAYLOAD_SIZE; // Size of the UDP payload

        // double dataRate =
        //   payloadSize * 8 / interval.GetSeconds(); // Data rate in bits per
        //   second

        // map_time_to_rxDr_bps[now] = dataRate;
    }

    std::map<Time, double> convert_map_to_Mps()
    {
        std::map<Time, double> map_time_to_rxDr_Mbps;
        // static uint32_t count = 0;
        for (const auto& pair : map_time_to_rxDr_bps)
        {
            // if (count < 20)
            //     std::cout << "convert_map_to_Mps: " << pair.first.GetSeconds() << "seconds "
            //               << pair.second << "result:" << pair.second / 1000 / 1000 << std::endl;
            map_time_to_rxDr_Mbps[pair.first] = pair.second / 1000.0 / 1000.0;
        }
        return map_time_to_rxDr_Mbps;
    }

    void export_to_csv(const std::string& path)
    {
        exportMapToCSV(convert_map_to_Mps(), path + "/dr.csv", "DataRate_Mbps");
        // exportMapToCSV(map_time_to_rxDr_bps, path + "/dr.csv", "DataRate_Mbps");
    }
};

class rx_pwr_utils
{
  public:
    std::map<Time, double> map_rx_pwr;

    void SignalArrivalCallback(std::string context,
                               bool signalType,
                               uint32_t senderNodeId,
                               double rxPower,
                               Time duration)
    {
        map_rx_pwr[Simulator::Now()] = rxPower;
    }

    double get_avg_rx_pwr_dbm()
    {
        double sum = 0.0;
        for (const auto& entry : map_rx_pwr)
        {
            sum += entry.second;
        }
        return sum / map_rx_pwr.size();
    }

    void export_to_csv(const std::string& path)
    {
        exportMapToCSV(map_rx_pwr, path + "/pwr.csv", "RxPower");
    }
};

int
main(int argc, char* argv[])
{
    LogComponentEnable("WifiExperiment", LOG_LEVEL_ERROR);
    // LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_ALL);
    // LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_ALL);
    // LogComponentEnable("WifiHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("WifiPhy", LOG_LEVEL_ALL);
    // LogComponentEnable("NodeList", LOG_LEVEL_ALL);
    // LogComponentEnable("NetDevice", LOG_LEVEL_ALL);
    // LogComponentEnable("UdpSocketImpl", LOG_LEVEL_ALL);
    // LogComponentEnable("PacketSocket", LOG_LEVEL_ALL);
    // LogComponentEnable("PacketSink", LOG_LEVEL_ALL);
    // LogComponentEnable("OnOffApplication", LOG_LEVEL_ALL);
    // LogComponentEnable("Packet", LOG_LEVEL_ALL);

    double distance = 10.0;
    double simulationTime = 10.0;
    double txPower = 10.0;
    std::string csv_export_path = "debug/test";
    bool export_summary = false;
    bool export_rx_pwr = false;
    bool export_rx_dr = false;
    std::string propagationModel = "FriisPropagationLossModel";

    CommandLine cmd(__FILE__);
    cmd.AddValue("distance", "distance between nodes", distance);
    cmd.AddValue("simulationTime", "simulation time in seconds", simulationTime);
    cmd.AddValue("csv_export_path",
                 "path to export csv file, empty for no export",
                 csv_export_path);
    cmd.AddValue("propagationModel", "propagation model to use", propagationModel);
    cmd.AddValue("export_summary", "export summary to csv (true,false)", export_summary);
    cmd.AddValue("export_rx_pwr", "export rx power to csv (true,false)", export_rx_pwr);
    cmd.AddValue("export_rx_dr", "export rx data rate to csv (true,false)", export_rx_dr);

    cmd.Parse(argc, argv);

    // say hello so I know when compilation is done
    std::cout << std::setw(80) << "===starting wifi-experiment.cc===" << std::endl;

    // Create channel
    SpectrumWifiPhyHelper wifiPhy;
    Ptr<MultiModelSpectrumChannel> channel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<PropagationLossModel> lossModel;
    if (propagationModel == "FriisPropagationLossModel")
    {
        lossModel = CreateObject<FriisPropagationLossModel>();
    }
    else if (propagationModel == "FixedRssLossModel")
    {
        lossModel = CreateObject<FixedRssLossModel>();
        // Set the received signal strength
        lossModel->SetAttribute("Rss", DoubleValue(-50)); // in dBm
    }
    else if (propagationModel == "ThreeLogDistancePropagationLossModel")
    {
        lossModel = CreateObject<ThreeLogDistancePropagationLossModel>();
    }
    else if (propagationModel == "TwoRayGroundPropagationLossModel")
    {
        lossModel = CreateObject<TwoRayGroundPropagationLossModel>();
        // Set the height of the antennas
        lossModel->SetAttribute("HeightAboveZ", DoubleValue(1.5)); // in meters
    }
    else if (propagationModel == "NakagamiPropagationLossModel")
    {
        lossModel = CreateObject<NakagamiPropagationLossModel>();
        // Set the m-values
        // lossModel->SetAttribute("m0", DoubleValue(.5));
        // lossModel->SetAttribute("m1", DoubleValue(.5));
        // lossModel->SetAttribute("m2", DoubleValue(.5));
    }
    else
    {
        NS_FATAL_ERROR("No propagation model selected");
        return NO_PROPAGATION_MODEL;
    }
    channel->AddPropagationLossModel(lossModel);
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    StringValue wifi_channel_settings("{38, 40, BAND_5GHZ, 0}");
    channel->SetPropagationDelayModel(delayModel);
    wifiPhy.SetChannel(channel);
    wifiPhy.Set("ChannelSettings", wifi_channel_settings);
    wifiPhy.Set("TxPowerStart", DoubleValue(txPower));
    wifiPhy.Set("TxPowerEnd", DoubleValue(txPower));
    wifiPhy.Set("TxGain", DoubleValue(1.0));
    wifiPhy.Set("RxGain", DoubleValue(1.0));

    // Set up WiFi
    WifiHelper wifi;
    // Create SSID
    Ssid ssid = Ssid("ns-3-ssid");

    // Create wifi mac adhoc helper
    WifiMacHelper wifiMacHelper;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    wifiMacHelper.SetType("ns3::AdhocWifiMac", "Ssid", SsidValue(ssid));

    wifi.SetRemoteStationManager("ns3::MinstrelHtWifiManager"); // Ht needed for 802.11n
    AsciiTraceHelper ascii;

    // Install WiFi on nodes
    NodeContainer nodes;
    nodes.Create(2);
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMacHelper, nodes);

    for (NetDeviceContainer::Iterator i = devices.Begin(); i != devices.End();
         ++i) // apparently NetDeviceContainer do not support range-based for loop
              // because they chose to capitalize the End and Begin functions.
              // Something something legacy support. I hate it
    {
        (*i)->SetMtu(1500);
    }

    wifiPhy.EnableAsciiAll(ascii.CreateFileStream("wifi-experiment.tr"));
    // Set up PHY RX callback to observe signal strength
    // After creating and installing devices on nodes
    // rx_pwr_utils rx_pwr;
    // Config::Connect("/NodeList/1/DeviceList/0/Phy/RxOk",
    //                 MakeCallback(&rx_pwr_utils::RxOkCallback, &rx_pwr));
    // Config::Connect("/NodeList/1/DeviceList/0/Phy/RxError",
    //                 MakeCallback(&rx_pwr_utils::RxErrorCallback, &rx_pwr));

    // Set up mobility
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));      // Node 1 position
    positionAlloc->Add(Vector(distance, 0.0, 0.0)); // Node 2 position
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Set up Internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // Set up IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Set up UDP traffic
    uint16_t port = 9;
    uint16_t payloadSize_bytes = PAYLOAD_SIZE;
    ApplicationContainer serverApp;
    UdpServerHelper server(port);
    serverApp = server.Install(nodes.Get(1));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(simulationTime + 1));

    double data_rate_Mbps = 75.0;
    double data_rate_s = rate_to_s(data_rate_Mbps, payloadSize_bytes);
    UdpClientHelper client(interfaces.GetAddress(1), port);
    client.SetAttribute("MaxPackets", UintegerValue(4294967295U));
    client.SetAttribute("Interval", TimeValue(Seconds(data_rate_s))); // packets/s
    client.SetAttribute("PacketSize", UintegerValue(payloadSize_bytes));
    ApplicationContainer clientApp = client.Install(nodes.Get(0));
    clientApp.Start(Seconds(1.0));
    clientApp.Stop(Seconds(simulationTime + 1));

    // // Set up FlowMonitor to observe received data-rate
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    // // Set up PHY RX callback to observe signal strength
    // Config::Connect("/NodeList/*/DeviceList/*/Phy/RxBegin",
    // MakeCallback(&rx_pwr::operator(), new rx_pwr()));

    Simulator::Stop(Seconds(simulationTime));

    // Set up PHY RX callback to observe signal strength
    // After creating and installing devices on nodes
    rx_pwr_utils rx_pwr;
    rx_dr_utils rx_dr;
    // Config::Connect("/NodeList/*/DeviceList/*/Phy/State/RxOk",
    // MakeCallback(&rx_pwr_utils::RxOkCallback, &rx_pwr));
    // Config::Connect("/NodeList/*/DeviceList/*/Phy/State/RxError",
    // MakeCallback(&rx_pwr_utils::RxErrorCallback, &rx_pwr));
    Config::Connect("/NodeList/1/DeviceList/*/Phy/SignalArrival",
                    MakeCallback(&rx_pwr_utils::SignalArrivalCallback, &rx_pwr));
    Config::Connect("/NodeList/1/DeviceList/*/Phy/State/RxOk",
                    MakeCallback(&rx_dr_utils::RxOkCallback, &rx_dr));
    // Config::Connect("/NodeList/1/DeviceList/*/Phy/MonitorSnifferRx", TODO: figure out why 0
    // MakeCallback(&rx_dr_utils::MonitorSnifferRxCallback, &rx_dr));
    Simulator::Run();

    // Print FlowMonitor statistics
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
         i != stats.end();
         ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> "
                  << t.destinationAddress << ")\n";
        std::cout << "  Received data-rate: " << std::fixed << std::setprecision(2)
                  << i->second.rxBytes * 8.0 / simulationTime / 1000 / 1000
                  << " Mbps\n"; // simulation only starts at 2 seconds
    }

    std::cout << "Average Rx power: " << rx_pwr.get_avg_rx_pwr_dbm() << " dBm" << std::endl;

    if (csv_export_path != "")
    {
        if (export_summary)
        {
            std::ofstream csvFile;
            csvFile.open(csv_export_path + "/summary.csv");
            csvFile << "FlowId,SourceAddress,DestinationAddress,ReceivedDataRate,"
                       "AverageRxPower\n";

            for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
                 i != stats.end();
                 ++i)
            {
                Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
                double receivedDataRate =
                    i->second.rxBytes * 8.0 / simulationTime / 1000 / 1000; // Mbps
                csvFile << i->first << "," << t.sourceAddress << "," << t.destinationAddress << ","
                        << std::fixed << std::setprecision(2) << receivedDataRate << ","
                        << rx_pwr.get_avg_rx_pwr_dbm() << "\n";
            }

            csvFile.close();
        }
        if (export_rx_pwr)
        {
            rx_pwr.export_to_csv(csv_export_path);
        }
        if (export_rx_dr)
        {
            rx_dr.export_to_csv(csv_export_path);
        }
    }
}
