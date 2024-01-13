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
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/uinteger.h"
#include "ns3/wifi-helper.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-utils.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/spectrum-wifi-helper.h"

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

int
main(int argc, char* argv[])
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

    // // Create channel
    // YansWifiChannelHelper channelHelper = YansWifiChannelHelper::Default();
    // // channelHelper.AddPropagationLoss(propagationModel);
    // YansWifiPhyHelper wifiPhy;
    // wifiPhy.SetChannel(channelHelper.Create());
    // StringValue wifi_channel_settings("{38, 40, BAND_5GHZ, 0}");
    // wifiPhy.Set("ChannelSettings", wifi_channel_settings);
    // wifiPhy.Set("TxPowerStart", ns3::DoubleValue(txPower));
    // wifiPhy.Set("TxPowerEnd", ns3::DoubleValue(txPower));
    // wifiPhy.Set("TxGain", ns3::DoubleValue(1.0));
    // wifiPhy.Set("RxGain", ns3::DoubleValue(1.0));


    // Create channel
    SpectrumWifiPhyHelper wifiPhy;
    Ptr<MultiModelSpectrumChannel> channel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<PropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
    channel->AddPropagationLossModel(lossModel);
    Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
    channel->SetPropagationDelayModel(delayModel);
    wifiPhy.SetChannel(channel);
    wifiPhy.Set("TxPowerStart", DoubleValue(txPower));
    wifiPhy.Set("TxPowerEnd", DoubleValue(txPower));
    wifiPhy.Set("TxGain", DoubleValue(1.0));
    wifiPhy.Set("RxGain", DoubleValue(1.0));

    // Create SSID
    Ssid ssid = Ssid("ns-3-ssid");

    // Create wifi mac adhoc helper
    WifiMacHelper wifiMacHelper;
    wifiMacHelper.SetType("ns3::AdhocWifiMac", "Ssid", SsidValue(ssid));

    // Set up WiFi
    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager");
    AsciiTraceHelper ascii;

    // Install WiFi on nodes
    NodeContainer nodes;
    nodes.Create(2);
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMacHelper, nodes);
    wifi.SetStandard(WIFI_STANDARD_80211n);//this needs to be set after Install for some reason, otherwise "Can't find response rate for OfdmRate6Mbps"
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
    uint16_t port = 9; // Discard port (RFC 863)
    OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(interfaces.GetAddress(1), port));
    onoff.SetConstantRate(DataRate("75Mbps"), 1450);
    ApplicationContainer source_apps = onoff.Install(nodes.Get(0));
    source_apps.Start(Seconds(2.0));
    source_apps.Stop(Seconds(simulationTime));

    // Set up packet sink
    PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sink_apps = sink.Install(nodes.Get(1));
    sink_apps.Start(Seconds(1.0));
    sink_apps.Stop(Seconds(simulationTime));

    // Set up FlowMonitor to observe received data-rate
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    // Set up PHY RX callback to observe signal strength
    // Config::Connect("/NodeList/*/DeviceList/*/Phy/RxBegin",
    // MakeCallback(&rx_pwr::operator(), new rx_pwr()));
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
                  << i->second.rxBytes * 8.0 / (simulationTime - 2.0) / 1024 / 1024
                  << " Mbps\n"; // simulation only starts at 2 seconds
    }

    Simulator::Destroy();
}