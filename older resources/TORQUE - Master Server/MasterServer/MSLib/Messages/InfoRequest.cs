using System;
using System.Collections.Generic;
using System.Net;

namespace MSLib.Messages {
    /// <summary>
    /// This class represents a InfoRequest message. It is also used to process the request and return
    /// the results of the request.
    /// </summary>
    public class InfoRequest : UDPMessage {

        #region CTOR
        /// <summary>
        /// This constructor is used to create an outbound message
        /// </summary>
        /// <param name="size">the size in bytes to allocate for the message</param>
        public InfoRequest(int size): base(size){}

        /// <summary>
        /// This constructor is used to create an inbound message.
        /// </summary>
        /// <param name="ipRemoteAddress">the address the package came from</param>
        /// <param name="sMessage">the string representation of the packet (useful for debugging)</param>
        /// <param name="barrMessage">the byte array of the message</param>
        public InfoRequest(IPEndPoint ipRemoteAddress, string sMessage, Byte[] barrMessage)
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
        #endregion
    }
}
