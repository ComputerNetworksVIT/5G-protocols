#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace ns3;

int main(int argc, char *argv[]) {
    Time::SetResolution(Time::NS);
    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);

    srand(time(0));

    NodeContainer routers;
    routers.Create(7);

    NodeContainer devices;
    devices.Create(24);

    PointToPointHelper routerToDevice;
    routerToDevice.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    routerToDevice.SetChannelAttribute("Delay", StringValue("1ms"));
    routerToDevice.SetQueue("ns3::DropTailQueue");

    NetDeviceContainer r2dDevices[24];
    for (int i = 0; i < 24; i++) {
        r2dDevices[i] = routerToDevice.Install(routers.Get(i/4), devices.Get(i));
    }

    PointToPointHelper routerToRouter;
    routerToRouter.SetDeviceAttribute("DataRate", StringValue("10Gbps"));
    routerToRouter.SetChannelAttribute("Delay", StringValue("2ms"));
    routerToRouter.SetQueue("ns3::DropTailQueue");

    NetDeviceContainer routerLinks[6];
    for (int i = 0; i < 6; i++) {
        routerLinks[i] = routerToRouter.Install(routers.Get(i), routers.Get(i + 1));
    }

    InternetStackHelper stack;
    stack.Install(routers);
    stack.Install(devices);

    Ipv4AddressHelper address;
    std::vector<Ipv4InterfaceContainer> interfaces;

    for (int i = 0; i < 24; i++) {
        std::ostringstream subnet;
        subnet << "10.1." << i+1 << ".0";
        address.SetBase(subnet.str().c_str(), "255.255.255.0");
        interfaces.push_back(address.Assign(r2dDevices[i]));
    }

    for (int i = 0; i < 6; i++) {
        std::ostringstream subnet;
        subnet << "10.2." << i+1 << ".0";
        address.SetBase(subnet.str().c_str(), "255.255.255.0");
        interfaces.push_back(address.Assign(routerLinks[i]));
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t basePort = 9000;
    for (int i = 0; i < 24; i++) {
        uint16_t port = basePort + i;
        int receiverIndex;
        do {
            receiverIndex = rand() % 24;
        } while (receiverIndex == i);

        OnOffHelper mmtc("ns3::UdpSocketFactory",
                         InetSocketAddress(interfaces[receiverIndex].GetAddress(1), port));
        mmtc.SetAttribute("DataRate", StringValue("50kbps"));
        mmtc.SetAttribute("PacketSize", UintegerValue(64));
        mmtc.SetAttribute("StartTime", TimeValue(Seconds(1.0 + (i*0.05))));
        mmtc.SetAttribute("StopTime", TimeValue(Seconds(12.0)));
        mmtc.Install(devices.Get(i));

        PacketSinkHelper sink("ns3::UdpSocketFactory",
                              InetSocketAddress(Ipv4Address::GetAny(), port));
        sink.Install(devices.Get(receiverIndex));
    }

    AnimationInterface anim("mmtc-vertical.xml");

    for (int i = 0; i < 7; i++) {
        anim.SetConstantPosition(routers.Get(i), 100, 100 + i*100);
        anim.UpdateNodeDescription(routers.Get(i), "Router" + std::to_string(i+1));
        anim.UpdateNodeColor(routers.Get(i), 255, 0, 0);
    }

    for (int i = 0; i < 24; i++) {
        int routerId = i / 4;
        anim.SetConstantPosition(devices.Get(i), 250 + (i%4)*50, 100 + routerId*100);
        anim.UpdateNodeDescription(devices.Get(i), "PC" + std::to_string(i+1));
        anim.UpdateNodeColor(devices.Get(i), 0, 255, 0);
    }

    Simulator::Stop(Seconds(13.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
