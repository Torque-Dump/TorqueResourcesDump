using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Net;
using System.Net.Sockets;

namespace MSLib.Messages {
    public class Heartbeat : UDPMessage {

        public Heartbeat(int size): base(size){

        }
        public Heartbeat(IPEndPoint ipRemoteAddress, string sMessage, Byte[] barrMessage)
            : base(ipRemoteAddress, sMessage, barrMessage) {
            this.InitializeReader();
        }

        public override List<UDPMessage> ProcessRequest() {
            //Store the data
            MasterServer.Server_Store.heartbeatServer(this.RemoteAddress);

            //Process the heartbeat (in addition) is to request info from the server.
            UDPMessage theMessage = new UDPMessage(64);
            ushort iSession = 0;
            ushort iKey = 0;
            Session theSession = MasterServer.Sessions.makeSession(this.RemoteAddress, iSession, iKey);

            theMessage.stuffHeader(10, 0, (ushort)theSession.SessionID, (ushort)theSession.Key);
            theMessage.RemoteAddress = this.RemoteAddress;

            List<UDPMessage> theList = new List<UDPMessage>();
            theList.Add(theMessage);
            
            return theList;
        }
    }
}
