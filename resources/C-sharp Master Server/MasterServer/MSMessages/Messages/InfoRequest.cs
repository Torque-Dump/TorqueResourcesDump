using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Net;
using System.Net.Sockets;

namespace MSLib.Messages {
    public class InfoRequest : UDPMessage {

        public InfoRequest(int size): base(size){

        }
        public InfoRequest(IPEndPoint ipRemoteAddress, string sMessage, Byte[] barrMessage)
            : base(ipRemoteAddress, sMessage, barrMessage) {
            this.InitializeReader();
        }

        public override List<UDPMessage> ProcessRequest() {
            UDPMessage theMessage = new UDPMessage(1600);
            theMessage.RemoteAddress = this.RemoteAddress;

            theMessage.stuffHeader((ushort)MessageTypes.MasterServerInfoResponse, 0, this.Session, this.Key);

            theMessage.writeCString(MasterServer.ServerPreferences.Name);
            theMessage.writeCString(MasterServer.ServerPreferences.Region);
            theMessage.writeU16((ushort)MasterServer.Server_Store.Count);

            List<UDPMessage> theList = new List<UDPMessage>();
            theList.Add(theMessage);

            return theList;
        }
    }
}
