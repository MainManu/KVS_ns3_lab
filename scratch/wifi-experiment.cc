#include "ns3/boolean.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/core-module.h"
#include "ns3/double.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor.h"
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
#include "ns3/wifi-phy.h"
#include "ns3/wifi-utils.h"
#include "ns3/yans-wifi-helper.h"

#include <iomanip>
// #include <tuple.h>
// #include <enum.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiExperiment");

// struct rx_pwr {
//     void operator()(Ptr<const Packet> packet, double snr, WifiMode mode, enum WifiPreamble
//     preamble) {
//         double signalStrengthDbm = RatioToDb(snr) + 30;
//         std::cout << "Signal strength: " << signalStrengthDbm << " dBm" << std::endl;
//     }
// };

double
rate_to_s(double rate_Mbps, uint32_t packetSize_bytes)
{
    return (packetSize_bytes * 8.0) / (rate_Mbps * 1024.0 * 1024.0);
}

class rx_pwr_utils
{
public:
    double DbmFromW(double w)
    {
        return 10.0 * std::log10(w) + 30.0;
    }

    void RxOkCallback(std::string context,
                      Ptr<const Packet> packet,
                      double snr,
                      WifiMode mode,
                      enum WifiPreamble preamble)
    {
        double rxPowerDbm = DbmFromW(snr * WToDbm(1.0));
        std::cout << context << " Received packet with power " << rxPowerDbm << " dBm\n";
    }

    void RxErrorCallback(std::string context, Ptr<const Packet> packet, double snr)
    {
        double rxPowerDbm = DbmFromW(snr * WToDbm(1.0));
        std::cout << context << " Received packet with errors, power " << rxPowerDbm << " dBm\n";
    }
};

int main(int argc, char *argv[])
{
    LogComponentEnable("WifiExperiment", LOG_LEVEL_ALL);
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
    std::string propagationModel = "ns3::FriisPropagationLossModel";

    CommandLine cmd(__FILE__);
    cmd.AddValue("distance", "distance between nodes", distance);
    cmd.AddValue("simulationTime", "simulation time in seconds", simulationTime);
    // cmd.AddValue("propagationModel", "propagation model to use", propagationModel);
    cmd.Parse(argc, argv);

    // say hello so I know when compilation is done
    std::cout << std::setw(80) << "===starting wifi-experiment.cc===" << std::endl;

    // Create channel
    SpectrumWifiPhyHelper wifiPhy;
    Ptr<MultiModelSpectrumChannel> channel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<PropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
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
    uint16_t payloadSize_bits = 1450;
    ApplicationContainer serverApp;
    UdpServerHelper server(port);
    serverApp = server.Install(nodes.Get(1));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(simulationTime + 1));

    double data_rate_Mbps = 75.0;
    double data_rate_s = rate_to_s(data_rate_Mbps, payloadSize_bits);
    UdpClientHelper client(interfaces.GetAddress(1), port);
    client.SetAttribute("MaxPackets", UintegerValue(4294967295U));
    client.SetAttribute("Interval", TimeValue(Seconds(data_rate_s))); // packets/s
    client.SetAttribute("PacketSize", UintegerValue(payloadSize_bits));
    ApplicationContainer clientApp = client.Install(nodes.Get(0));
    clientApp.Start(Seconds(1.0));
    clientApp.Stop(Seconds(simulationTime + 1));

    // // Set up FlowMonitor to observe received data-rate
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    // // Set up PHY RX callback to observe signal strength
    // // Config::Connect("/NodeList/*/DeviceList/*/Phy/RxBegin",
    // // MakeCallback(&rx_pwr::operator(), new rx_pwr()));

    // Run simulation
    Simulator::Stop(Seconds(simulationTime));
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
                  << i->second.rxBytes * 8.0 / simulationTime / 1024 / 1024
                  << " Mbps\n"; // simulation only starts at 2 seconds
    }

    Simulator::Destroy();
}