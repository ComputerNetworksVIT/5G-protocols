#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

int main(int argc, char *argv[]) {
    Time::SetResolution(Time::NS);
    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);

    NodeContainer routers;
    routers.Create(5);   
    NodeContainer devices;
    devices.Create(5);   

    PointToPointHelper routerToDevice;
    routerToDevice.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    routerToDevice.SetChannelAttribute("Delay", StringValue("1ms"));

    NetDeviceContainer r2dDevices[5];
    for (int i = 0; i < 5; i++) {
        r2dDevices[i] = routerToDevice.Install(routers.Get(i), devices.Get(i));
    }

    PointToPointHelper routerToRouter;
    routerToRouter.SetDeviceAttribute("DataRate", StringValue("10Gbps"));
    routerToRouter.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer routerLinks[4];
    for (int i = 0; i < 4; i++) {
        routerLinks[i] = routerToRouter.Install(routers.Get(i), routers.Get(i + 1));
    }

    InternetStackHelper stack;
    stack.Install(routers);
    stack.Install(devices);

    Ipv4AddressHelper address;
    std::vector<Ipv4InterfaceContainer> interfaces;

    for (int i = 0; i < 5; i++) {
        std::ostringstream subnet;
        subnet << "10.1." << i + 1 << ".0";
        address.SetBase(subnet.str().c_str(), "255.255.255.0");
        interfaces.push_back(address.Assign(r2dDevices[i]));
    }

    for (int i = 0; i < 4; i++) {
        std::ostringstream subnet;
        subnet << "10.2." << i + 1 << ".0";
        address.SetBase(subnet.str().c_str(), "255.255.255.0");
        interfaces.push_back(address.Assign(routerLinks[i]));
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t port = 9;
    OnOffHelper onoff("ns3::UdpSocketFactory",
                      InetSocketAddress(interfaces[1].GetAddress(1), port));
    onoff.SetAttribute("DataRate", StringValue("100Mbps"));
    onoff.SetAttribute("PacketSize", UintegerValue(1400));
    onoff.SetAttribute("StartTime", TimeValue(Seconds(1.0)));
    onoff.SetAttribute("StopTime", TimeValue(Seconds(10.0)));
    onoff.Install(devices.Get(0));

    PacketSinkHelper sink("ns3::UdpSocketFactory",
                          InetSocketAddress(Ipv4Address::GetAny(), port));
    sink.Install(devices.Get(1));

    AnimationInterface anim("embb-animation.xml");

    for (int i = 0; i < 5; i++) {
        anim.SetConstantPosition(routers.Get(i), 20 + i * 30, 50);
        anim.UpdateNodeDescription(routers.Get(i), "Router" + std::to_string(i + 1));
        anim.UpdateNodeColor(routers.Get(i), 255, 0, 0);

        anim.SetConstantPosition(devices.Get(i), 20 + i * 30, 20);
        anim.UpdateNodeDescription(devices.Get(i), "Device" + std::to_string(i + 1));
        anim.UpdateNodeColor(devices.Get(i), 0, 255, 0);
    }

    Simulator::Stop(Seconds(11.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
