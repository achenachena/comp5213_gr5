#include <ns3/applications-module.h>
#include <ns3/core-module.h>
#include <ns3/error-model.h>
#include <ns3/log.h>
#include <ns3/flow-monitor-helper.h>
#include <ns3/eps-bearer.h>
#include <ns3/internet-module.h>
#include <ns3/ipv4-global-routing-helper.h>
#include <ns3/ipv4-static-routing-helper.h>
#include <ns3/lte-helper.h>
#include <ns3/mobility-helper.h>
#include <ns3/point-to-point-epc-helper.h>
#include <ns3/network-module.h>
#include <ns3/point-to-point-channel.h>
#include <ns3/point-to-point-dumbbell.h>
#include <ns3/point-to-point-module.h>
#include <ns3/point-to-point-net-device.h>
#include <ns3/rng-seed-manager.h>
#include <ns3/system-path.h>

#include <cstdint>
#include <string>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpCompare");

struct RuntimeOptions
{
  RuntimeOptions ();

  std::string scenario;
  std::string tcpType;
  std::string queueSize;
  double simulationTime;   // seconds
  double warmupTime;       // seconds to discard in analysis
  uint32_t seed;           // ns-3 RNG stream
  double lossRate;         // used in wireless scenario
  double blockageDuration; // blockage duration for S4 (seconds)
  bool enableFlowMonitor;
};

RuntimeOptions::RuntimeOptions ()
    : scenario ("S1"),
      tcpType ("TcpCubic"),
      queueSize ("150p"),
      simulationTime (120.0),
      warmupTime (20.0),
      seed (1),
      lossRate (0.0),
      blockageDuration (0.2),
      enableFlowMonitor (true)
{
}

static std::string
CreateOutputDir (const RuntimeOptions &opts)
{
  std::string dir =
      "results/" + opts.scenario + "/" + opts.tcpType + "/run-" + std::to_string (opts.seed);
  SystemPath::MakeDirectories (dir.c_str ());
  return dir;
}

static void
ConfigureTcp (const std::string &tcpType)
{
  ns3::TypeId tid;
  std::string fullName = "ns3::" + tcpType;
  bool ok = ns3::TypeId::LookupByNameFailSafe (fullName, &tid);
  NS_ABORT_MSG_IF (!ok, "Unknown TCP type: " << tcpType);

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (tid));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (4 * 1024 * 1024));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (4 * 1024 * 1024));
  Config::SetDefault ("ns3::TcpSocketBase::WindowScaling", BooleanValue (true));
  Config::SetDefault ("ns3::TcpSocketBase::Timestamp", BooleanValue (true));
}

static void
CwndTracer (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "," << oldCwnd << "," << newCwnd << std::endl;
}

static void
InstallCwndTracer (Ptr<Node> node, Ptr<OutputStreamWrapper> stream)
{
  const std::string path = "/NodeList/" + std::to_string (node->GetId ()) +
                           "/$ns3::TcpL4Protocol/SocketList/*/CongestionWindow";
  Config::ConnectWithoutContext (path, MakeBoundCallback (&CwndTracer, stream));
}

static void
SerializeFlowMonitor (Ptr<FlowMonitor> monitor, const std::string &path)
{
  monitor->SerializeToXmlFile (path, true, true);
}

static void
SetOnOffRate (Ptr<OnOffApplication> app, const std::string &rate)
{
  if (app)
    {
      app->SetAttribute ("DataRate", DataRateValue (DataRate (rate)));
    }
}

static void
AssignIpv4Addresses (PointToPointDumbbellHelper &dumbbell)
{
  Ipv4AddressHelper leftIp ("10.1.1.0", "255.255.255.0");
  Ipv4AddressHelper rightIp ("10.2.1.0", "255.255.255.0");
  Ipv4AddressHelper routerIp ("10.3.1.0", "255.255.255.0");

  dumbbell.AssignIpv4Addresses (leftIp, rightIp, routerIp);
}

static void
InstallStacks (PointToPointDumbbellHelper &dumbbell)
{
  InternetStackHelper stack;
  dumbbell.InstallStack (stack);
}

static void
InstallBulkTransfers (PointToPointDumbbellHelper &dumbbell, double start, double stop)
{
  uint16_t port = 5000;
  for (uint32_t i = 0; i < dumbbell.LeftCount (); ++i)
    {
      Address sinkAddress (InetSocketAddress (dumbbell.GetRightIpv4Address (i), port + i));
      PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkAddress);
      ApplicationContainer sinkApp = sinkHelper.Install (dumbbell.GetRight (i));
      sinkApp.Start (Seconds (start));
      sinkApp.Stop (Seconds (stop));

      BulkSendHelper bulkSender ("ns3::TcpSocketFactory", sinkAddress);
      bulkSender.SetAttribute ("MaxBytes", UintegerValue (0));
      ApplicationContainer senderApp = bulkSender.Install (dumbbell.GetLeft (i));
      senderApp.Start (Seconds (start));
      senderApp.Stop (Seconds (stop));
    }
}

static void
InstallShortWebTraffic (Ptr<Node> client, Ptr<Node> server, Ipv4Address serverAddress, double start, double stop)
{
  uint16_t port = 9000;
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApp = sinkHelper.Install (server);
  sinkApp.Start (Seconds (start));
  sinkApp.Stop (Seconds (stop));

  OnOffHelper httpHelper ("ns3::TcpSocketFactory", Address (InetSocketAddress (serverAddress, port)));
  httpHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"));
  httpHelper.SetAttribute ("OffTime", StringValue ("ns3::ExponentialRandomVariable[Mean=0.8]"));
  httpHelper.SetAttribute ("PacketSize", UintegerValue (1200));
  httpHelper.SetAttribute ("DataRate", DataRateValue (DataRate ("10Mbps")));
  ApplicationContainer clientApp = httpHelper.Install (client);
  clientApp.Start (Seconds (start + 1.0));
  clientApp.Stop (Seconds (stop));
}

static void
BuildScenarioS1 (const RuntimeOptions &opts)
{
  PointToPointHelper access;
  access.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  access.SetChannelAttribute ("Delay", StringValue ("1ms"));

  PointToPointHelper bottleneck;
  bottleneck.SetDeviceAttribute ("DataRate", StringValue ("20Mbps"));
  bottleneck.SetChannelAttribute ("Delay", StringValue ("15ms"));
  bottleneck.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize", StringValue (opts.queueSize));

  PointToPointDumbbellHelper dumbbell (2, access, 2, access, bottleneck);
  InstallStacks (dumbbell);
  AssignIpv4Addresses (dumbbell);

  InstallBulkTransfers (dumbbell, 0.0, opts.simulationTime);

  AsciiTraceHelper ascii;
  const std::string outputDir = CreateOutputDir (opts);
  auto cwndStream = ascii.CreateFileStream (outputDir + "/cwnd.csv");
  Simulator::Schedule (Seconds (0.1), &InstallCwndTracer, dumbbell.GetLeft (0), cwndStream);

  FlowMonitorHelper flowmonHelper;
  Ptr<FlowMonitor> monitor;
  if (opts.enableFlowMonitor)
    {
      monitor = flowmonHelper.InstallAll ();
    }

  Simulator::Stop (Seconds (opts.simulationTime));
  Simulator::Run ();

  if (monitor)
    {
      SerializeFlowMonitor (monitor, outputDir + "/flowmon.xml");
    }

  Simulator::Destroy ();
}

static void
BuildScenarioS2 (const RuntimeOptions &opts)
{
  // Nodes: two sources, two sinks, and two routers forming the dumbbell backbone
  NodeContainer leftHosts;
  leftHosts.Create (2);
  NodeContainer rightHosts;
  rightHosts.Create (2);
  NodeContainer routers;
  routers.Create (2); // 0 = left router, 1 = right router

  PointToPointHelper fastAccess;
  fastAccess.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  fastAccess.SetChannelAttribute ("Delay", StringValue ("1ms"));

  PointToPointHelper slowAccess;
  slowAccess.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  slowAccess.SetChannelAttribute ("Delay", StringValue ("60ms")); // ~120 ms RTT

  PointToPointHelper rightAccess;
  rightAccess.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  rightAccess.SetChannelAttribute ("Delay", StringValue ("1ms"));

  PointToPointHelper bottleneck;
  bottleneck.SetDeviceAttribute ("DataRate", StringValue ("20Mbps"));
  bottleneck.SetChannelAttribute ("Delay", StringValue ("15ms"));
  bottleneck.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize", StringValue (opts.queueSize));

  NetDeviceContainer left0Devices = fastAccess.Install (leftHosts.Get (0), routers.Get (0));
  NetDeviceContainer left1Devices = slowAccess.Install (leftHosts.Get (1), routers.Get (0));
  NetDeviceContainer right0Devices = rightAccess.Install (routers.Get (1), rightHosts.Get (0));
  NetDeviceContainer right1Devices = rightAccess.Install (routers.Get (1), rightHosts.Get (1));
  NetDeviceContainer backboneDevices = bottleneck.Install (routers.Get (0), routers.Get (1));

  InternetStackHelper stack;
  stack.Install (leftHosts);
  stack.Install (rightHosts);
  stack.Install (routers);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer left0If = ipv4.Assign (left0Devices);
  ipv4.NewNetwork ();
  Ipv4InterfaceContainer left1If = ipv4.Assign (left1Devices);
  ipv4.NewNetwork ();
  Ipv4InterfaceContainer right0If = ipv4.Assign (right0Devices);
  ipv4.NewNetwork ();
  Ipv4InterfaceContainer right1If = ipv4.Assign (right1Devices);
  ipv4.NewNetwork ();
  Ipv4InterfaceContainer backboneIf = ipv4.Assign (backboneDevices);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Set up two long-lived TCP flows: leftHosts[i] -> rightHosts[i]
  Ipv4Address rightHostAddrs[2] = {
      right0If.GetAddress (1),
      right1If.GetAddress (1),
  };

  uint16_t portBase = 5000;
  for (uint32_t i = 0; i < 2; ++i)
    {
      Address sinkAddress (InetSocketAddress (rightHostAddrs[i], portBase + i));
      PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkAddress);
      ApplicationContainer sinkApp = sinkHelper.Install (rightHosts.Get (i));
      sinkApp.Start (Seconds (0.0));
      sinkApp.Stop (Seconds (opts.simulationTime));

      BulkSendHelper bulkHelper ("ns3::TcpSocketFactory", sinkAddress);
      bulkHelper.SetAttribute ("MaxBytes", UintegerValue (0));
      ApplicationContainer senderApp = bulkHelper.Install (leftHosts.Get (i));
      senderApp.Start (Seconds (0.0));
      senderApp.Stop (Seconds (opts.simulationTime));
    }

  // Short web-style cross traffic
  InstallShortWebTraffic (leftHosts.Get (0), rightHosts.Get (1), rightHostAddrs[1], 5.0, opts.simulationTime);
  InstallShortWebTraffic (rightHosts.Get (0), leftHosts.Get (1), left1If.GetAddress (0), 5.0, opts.simulationTime);

  AsciiTraceHelper ascii;
  const std::string outputDir = CreateOutputDir (opts);
  auto cwndStream = ascii.CreateFileStream (outputDir + "/cwnd.csv");
  Simulator::Schedule (Seconds (0.1), &InstallCwndTracer, leftHosts.Get (0), cwndStream);

  FlowMonitorHelper flowmonHelper;
  Ptr<FlowMonitor> monitor;
  if (opts.enableFlowMonitor)
    {
      monitor = flowmonHelper.InstallAll ();
    }

  Simulator::Stop (Seconds (opts.simulationTime));
  Simulator::Run ();

  if (monitor)
    {
      SerializeFlowMonitor (monitor, outputDir + "/flowmon.xml");
    }

  Simulator::Destroy ();
}

static void
BuildScenarioS3 (const RuntimeOptions &opts)
{
  NodeContainer nodes;
  nodes.Create (4); // sender - router1 - router2 - receiver

  PointToPointHelper access;
  access.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  access.SetChannelAttribute ("Delay", StringValue ("5ms"));

  PointToPointHelper bottleneck;
  bottleneck.SetDeviceAttribute ("DataRate", StringValue ("40Mbps"));
  bottleneck.SetChannelAttribute ("Delay", StringValue ("10ms"));
  bottleneck.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize", StringValue (opts.queueSize));

  NetDeviceContainer d01 = access.Install (nodes.Get (0), nodes.Get (1));
  NetDeviceContainer d12 = bottleneck.Install (nodes.Get (1), nodes.Get (2));
  NetDeviceContainer d23 = access.Install (nodes.Get (2), nodes.Get (3));

  if (opts.lossRate > 0.0)
    {
      Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
      em->SetAttribute ("ErrorRate", DoubleValue (opts.lossRate));
      em->SetAttribute ("ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET));
      d12.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    }

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.10.1.0", "255.255.255.0");
  ipv4.Assign (d01);
  ipv4.SetBase ("10.10.2.0", "255.255.255.0");
  Ipv4InterfaceContainer bottleneckIf = ipv4.Assign (d12);
  ipv4.SetBase ("10.10.3.0", "255.255.255.0");
  Ipv4InterfaceContainer rightIf = ipv4.Assign (d23);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  uint16_t port = 6000;
  Address sinkAddress (InetSocketAddress (rightIf.GetAddress (1), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkAddress);
  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get (3));
  sinkApp.Start (Seconds (0.0));
  sinkApp.Stop (Seconds (opts.simulationTime));

  BulkSendHelper bulk ("ns3::TcpSocketFactory", sinkAddress);
  bulk.SetAttribute ("MaxBytes", UintegerValue (0));
  ApplicationContainer bulkApp = bulk.Install (nodes.Get (0));
  bulkApp.Start (Seconds (0.0));
  bulkApp.Stop (Seconds (opts.simulationTime));

  // UDP cross-traffic to emulate wireless contention
  OnOffHelper udpCross ("ns3::UdpSocketFactory", InetSocketAddress (rightIf.GetAddress (1), 7000));
  udpCross.SetAttribute ("PacketSize", UintegerValue (1200));
  udpCross.SetAttribute ("DataRate", DataRateValue (DataRate ("15Mbps")));
  udpCross.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  udpCross.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  ApplicationContainer udpApp = udpCross.Install (nodes.Get (0));
  udpApp.Start (Seconds (5.0));
  udpApp.Stop (Seconds (opts.simulationTime));

  PacketSinkHelper udpSink ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 7000));
  ApplicationContainer udpSinkApp = udpSink.Install (nodes.Get (3));
  udpSinkApp.Start (Seconds (5.0));
  udpSinkApp.Stop (Seconds (opts.simulationTime));

  AsciiTraceHelper ascii;
  const std::string outputDir = CreateOutputDir (opts);
  auto cwndStream = ascii.CreateFileStream (outputDir + "/cwnd.csv");
  Simulator::Schedule (Seconds (0.1), &InstallCwndTracer, nodes.Get (0), cwndStream);

  FlowMonitorHelper flowmonHelper;
  Ptr<FlowMonitor> monitor;
  if (opts.enableFlowMonitor)
    {
      monitor = flowmonHelper.InstallAll ();
    }

  Simulator::Stop (Seconds (opts.simulationTime));
  Simulator::Run ();

  if (monitor)
    {
      SerializeFlowMonitor (monitor, outputDir + "/flowmon.xml");
    }

  Simulator::Destroy ();
}

static void
BuildScenarioS5 (const RuntimeOptions &opts)
{
  PointToPointHelper access, bottleneck;
  access.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  access.SetChannelAttribute ("Delay", StringValue ("2ms"));

  bottleneck.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  bottleneck.SetChannelAttribute ("Delay", StringValue ("20ms"));
  bottleneck.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize", StringValue (opts.queueSize));

  const uint32_t nFlows = 8;
  PointToPointDumbbellHelper dumbbell (nFlows, access, nFlows, access, bottleneck);
  InstallStacks (dumbbell);
  AssignIpv4Addresses (dumbbell);
  InstallBulkTransfers (dumbbell, 0.0, opts.simulationTime);

  AsciiTraceHelper ascii;
  const std::string outputDir = CreateOutputDir (opts);
  auto cwndStream = ascii.CreateFileStream (outputDir + "/cwnd.csv");
  Simulator::Schedule (Seconds (0.1), &InstallCwndTracer, dumbbell.GetLeft (0), cwndStream);

  FlowMonitorHelper flowmonHelper;
  Ptr<FlowMonitor> monitor;
  if (opts.enableFlowMonitor)
    {
      monitor = flowmonHelper.InstallAll ();
    }

  Simulator::Stop (Seconds (opts.simulationTime));
  Simulator::Run ();

  if (monitor)
    {
      SerializeFlowMonitor (monitor, outputDir + "/flowmon.xml");
    }

  Simulator::Destroy ();
}

static void
BuildScenarioS4 (const RuntimeOptions &opts)
{
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  NodeContainer gnbNodes;
  gnbNodes.Create (1);
  NodeContainer ueNodes;
  ueNodes.Create (1);

  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (20.0),
                                 "DeltaY", DoubleValue (0.0),
                                 "GridWidth", UintegerValue (1),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.Install (gnbNodes);

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (50.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (0.0),
                                 "DeltaY", DoubleValue (0.0),
                                 "GridWidth", UintegerValue (1),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.Install (ueNodes);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);

  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
  p2ph.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);

  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIfaces = ipv4h.Assign (internetDevices);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
      ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NetDeviceContainer enbDevices = lteHelper->InstallEnbDevice (gnbNodes);
  NetDeviceContainer ueDevices = lteHelper->InstallUeDevice (ueNodes);

  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIfaces = epcHelper->AssignUeIpv4Address (ueDevices);

  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Ipv4StaticRouting> ueStaticRouting =
          ipv4RoutingHelper.GetStaticRouting (ueNodes.Get (u)->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  for (uint32_t i = 0; i < ueDevices.GetN (); ++i)
    {
      lteHelper->Attach (ueDevices.Get (i), enbDevices.Get (0));
    }

  uint16_t videoPort = 10000;
  uint16_t tcpPort = 10001;

  // High bitrate video via TCP OnOff
  PacketSinkHelper videoSinkHelper ("ns3::TcpSocketFactory",
                                    InetSocketAddress (Ipv4Address::GetAny (), videoPort));
  ApplicationContainer videoSink = videoSinkHelper.Install (ueNodes.Get (0));
  videoSink.Start (Seconds (0.0));
  videoSink.Stop (Seconds (opts.simulationTime));

  OnOffHelper videoSourceHelper ("ns3::TcpSocketFactory",
                                 Address (InetSocketAddress (ueIpIfaces.GetAddress (0), videoPort)));
  videoSourceHelper.SetAttribute ("DataRate", DataRateValue (DataRate ("50Mbps")));
  videoSourceHelper.SetAttribute ("PacketSize", UintegerValue (1316)); // MPEG-TS like payload
  videoSourceHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  videoSourceHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  ApplicationContainer videoSource = videoSourceHelper.Install (remoteHost);
  videoSource.Start (Seconds (1.0));
  videoSource.Stop (Seconds (opts.simulationTime));

  Ptr<OnOffApplication> videoApp = DynamicCast<OnOffApplication> (videoSource.Get (0));

  // Background TCP bulk transfer
  PacketSinkHelper tcpSinkHelper ("ns3::TcpSocketFactory",
                                  InetSocketAddress (Ipv4Address::GetAny (), tcpPort));
  ApplicationContainer tcpSink = tcpSinkHelper.Install (remoteHost);
  tcpSink.Start (Seconds (0.0));
  tcpSink.Stop (Seconds (opts.simulationTime));

  BulkSendHelper bulkHelper ("ns3::TcpSocketFactory",
                             InetSocketAddress (internetIfaces.GetAddress (1), tcpPort));
  bulkHelper.SetAttribute ("MaxBytes", UintegerValue (0));
  ApplicationContainer bulkApp = bulkHelper.Install (ueNodes.Get (0));
  bulkApp.Start (Seconds (5.0));
  bulkApp.Stop (Seconds (opts.simulationTime));

  AsciiTraceHelper ascii;
  const std::string outputDir = CreateOutputDir (opts);
  auto cwndStream = ascii.CreateFileStream (outputDir + "/cwnd.csv");
  Simulator::Schedule (Seconds (0.1), &InstallCwndTracer, remoteHost, cwndStream);

  // Emulate temporary blockage by throttling the video stream
  Time blockStart = Seconds (30.0);
  Time blockDuration = Seconds (opts.blockageDuration);
  Simulator::Schedule (blockStart, &SetOnOffRate, videoApp, std::string ("1bps"));
  Simulator::Schedule (blockStart + blockDuration, &SetOnOffRate, videoApp, std::string ("50Mbps"));

  FlowMonitorHelper flowmonHelper;
  Ptr<FlowMonitor> monitor;
  if (opts.enableFlowMonitor)
    {
      monitor = flowmonHelper.InstallAll ();
    }

  Simulator::Stop (Seconds (opts.simulationTime));
  Simulator::Run ();

  if (monitor)
    {
      SerializeFlowMonitor (monitor, outputDir + "/flowmon.xml");
    }

  Simulator::Destroy ();
}

int
main (int argc, char *argv[])
{
  RuntimeOptions opts;

  CommandLine cmd;
  cmd.AddValue ("scenario", "Scenario identifier (S1, S2, S3, S4, S5)", opts.scenario);
  cmd.AddValue ("tcp", "TCP variant typeId suffix (e.g., TcpCubic, TcpNewReno)", opts.tcpType);
  cmd.AddValue ("queue", "Bottleneck queue MaxSize (e.g., 100p, 1MB)", opts.queueSize);
  cmd.AddValue ("time", "Simulation duration (s)", opts.simulationTime);
  cmd.AddValue ("warmup", "Warm-up duration ignored in analysis (s)", opts.warmupTime);
  cmd.AddValue ("run", "RNG run number", opts.seed);
  cmd.AddValue ("loss", "Packet loss rate for S3 (0.0-1.0)", opts.lossRate);
  cmd.AddValue ("blockage", "Blockage duration for S4 in seconds", opts.blockageDuration);
  cmd.AddValue ("flowMonitor", "Enable FlowMonitor output", opts.enableFlowMonitor);
  cmd.Parse (argc, argv);

  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (opts.seed);

  ConfigureTcp (opts.tcpType);

  if (opts.scenario == "S1")
    {
      BuildScenarioS1 (opts);
    }
  else if (opts.scenario == "S2")
    {
      BuildScenarioS2 (opts);
    }
  else if (opts.scenario == "S3")
    {
      BuildScenarioS3 (opts);
    }
  else if (opts.scenario == "S4")
    {
      BuildScenarioS4 (opts);
    }
  else if (opts.scenario == "S5")
    {
      BuildScenarioS5 (opts);
    }
  else
    {
      NS_FATAL_ERROR ("Unsupported scenario: " << opts.scenario);
    }

  return 0;
}
