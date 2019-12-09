using System;
using System.Collections.Generic;
using System.Net;

namespace MSLib.Messages {
    /// <summary>
    /// This class represents a Heartbeat message. It is also used to process the request and return
    /// the results of the request.
    /// </summary>
    public class Heartbeat : UDPMessage {

        #region CTOR
        /// <summary>
        /// This constructor is used to create an outbound message
        /// </summary>
        /// <param name="size">the size in bytes to allocate for the message</param>
        public Heartbeat(int size): base(size){}

        /// <summary>
        /// This constructor is used to create an inbound message.
        /// </summary>
        /// <param name="ipRemoteAddress">the address the package came from</param>
        /// <param name="sMessage">the string representation of the packet (useful for debugging)</param>
        /// <param name="barrMessage">the byte array of the message</param>
        public Heartbeat(IPEndPoint ipRemoteAddress, string sMessage, Byte[] barrMessage)
            : base(ipRemoteAddress, sMessage, barrMessage) {
            this.InitializeReader();
            }

        #endregion

        #region Methods
        /// <summary>
        /// Processes the message and returns the results
        /// </summary>
        /// <returns>Results of processing the message</returns>
        public override List<UDPMessage> ProcessRequest() {
            //Store the data
            MasterServer.Server_Store.HeartbeatServer(this.RemoteAddress);

            //Process the heartbeat (in addition) is to request info from the server.
            UDPMessage theMessage = new UDPMessage(64);
            Session theSession = MasterServer.Sessions.MakeSession(this.RemoteAddress);

            theMessage.stuffHeader((ushort)MessageTypes.GameMasterInfoRequest, 0, (ushort)theSession.SessionID, (ushort)theSession.Key);
            theMessage.RemoteAddress = this.RemoteAddress;

            List<UDPMessage> theList = new List<UDPMessage>();
            theList.Add(theMessage);
            
            return theList;
        }
        #endregion
    }
}
