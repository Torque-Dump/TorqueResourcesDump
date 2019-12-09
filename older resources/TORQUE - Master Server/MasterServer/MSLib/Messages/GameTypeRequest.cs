using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;

namespace MSLib.Messages {
    /// <summary>
    /// This class represents a GameTypeRequest message. It is also used to process the request and return
    /// the results of the request.
    /// </summary>
    public class GameTypeRequest : UDPMessage
    {
        #region CTOR
        /// <summary>
        /// This constructor is used to create an outbound message
        /// </summary>
        /// <param name="size">the size in bytes to allocate for the message</param>
        public GameTypeRequest(int size): base(size){

        }

        /// <summary>
        /// This constructor is used to create an inbound message.
        /// </summary>
        /// <param name="ipRemoteAddress">the address the package came from</param>
        /// <param name="sMessage">the string representation of the packet (useful for debugging)</param>
        /// <param name="barrMessage">the byte array of the message</param>
        public GameTypeRequest(IPEndPoint ipRemoteAddress, string sMessage, Byte[] barrMessage)
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
        #endregion
    }
}
