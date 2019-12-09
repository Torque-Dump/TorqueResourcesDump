using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Net;
using System.Net.Sockets;

namespace MSLib.Messages {
    public class GameTypeRequest : UDPMessage {

        public GameTypeRequest(int size): base(size){

        }
        public GameTypeRequest(IPEndPoint ipRemoteAddress, string sMessage, Byte[] barrMessage)
            : base(ipRemoteAddress, sMessage, barrMessage) {
            this.InitializeReader();
        }

        public override List<UDPMessage> ProcessRequest() {
            UDPMessage theMessage = new UDPMessage(1600);
            theMessage.RemoteAddress = this.RemoteAddress;

            theMessage.stuffHeader((ushort)MessageTypes.MasterServerGameTypesResponse, 0, this.Session, this.Key);

            IList<string> lstGameTypes = MasterServer.Server_Store.GetGameTypes();
            IList<string> lstMissionTypes = MasterServer.Server_Store.GetMissionTypes();

            //Send some bogus game types for now
            theMessage.writeU8((byte)lstGameTypes.Count()); //This is the count of game types

            foreach (string s in lstGameTypes) {
                theMessage.writeCString(s);
            }
            
            //Send some bogus game types
            theMessage.writeU8((byte)lstMissionTypes.Count()); //This is the count of mission types

            foreach (string s in lstMissionTypes) {
                theMessage.writeCString(s);
            }

            List<UDPMessage> theList = new List<UDPMessage>();
            theList.Add(theMessage);

            return theList;
        }
    }
}
