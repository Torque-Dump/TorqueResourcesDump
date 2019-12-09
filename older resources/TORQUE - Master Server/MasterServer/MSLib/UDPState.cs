using System.Net;
using System.Net.Sockets;

namespace MSLib {
    class UDPState {
        public UDPState() { }

        public IPEndPoint EndPoint {
            get;
            set;
        }

        public UdpClient UDPClient {
            get;
            set;
        }
    }
}
