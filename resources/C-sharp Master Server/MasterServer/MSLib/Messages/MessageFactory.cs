using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;

namespace MSLib.Messages {
    /// <summary>
    /// This classes only purpose is to determine what type of message came in and create the necessary
    /// class to process the message. It cannot be instantiated directly
    /// </summary>
    public class MessageFactory {
        /// <summary>
        /// Private constructor to prevent the class from being instantiated.
        /// </summary>
        private MessageFactory() { }

        /// <summary>
        /// Creates and returns the proper message from the incomming UDP message packet.
        /// </summary>
        /// <param name="ipRemoteAddress">Who the message came from</param>
        /// <param name="sMessage">The string representation of the message (for debugging help)</param>
        /// <param name="barrMessage">The byte[] of the message packet</param>
        /// <returns></returns>
        public static UDPMessage DetermineMessage(IPEndPoint ipRemoteAddress, string sMessage, Byte[] barrMessage) {
            UDPMessage oReturn = null;
            
            Stream s = new MemoryStream(barrMessage);
            BinaryReader br = new BinaryReader(s);
            int iPacketType = br.PeekChar();
            br.Close();

            switch ((MessageTypes)iPacketType) {
                case MessageTypes.MasterServerGameTypesRequest: {
                        MasterServer.EventLog.LogEntry(2, "MasterServerGameTypesRequest given");
                        GameTypeRequest theRequest = new GameTypeRequest(ipRemoteAddress, sMessage, barrMessage);
                        oReturn = theRequest;
                    }
                    break;
                //case MessageTypes.MasterServerGameTypesResponse:

                //    break;
                case MessageTypes.MasterServerListRequest: {
                        MasterServer.EventLog.LogEntry(2, "MasterServerListRequest given");
                        ServerListRequest theRequest = new ServerListRequest(ipRemoteAddress, sMessage, barrMessage);
                        oReturn = theRequest;
                    }
                    break;
                //case MessageTypes.MasterServerListResponse:

                //    break;
                //case MessageTypes.GameMasterInfoRequest:

                //    break;
                case MessageTypes.GameMasterInfoResponse: {
                        MasterServer.EventLog.LogEntry(2, "GameMasterInfoResponse received");
                        InfoResponse theRequest = new InfoResponse(ipRemoteAddress, sMessage, barrMessage);
                        oReturn = theRequest;
                    }
                    break;
                //case MessageTypes.GamePingRequest:

                //    break;
                //case MessageTypes.GamePingResponse:

                //    break;
                //case MessageTypes.GameInfoRequest:

                //    break;
                //case MessageTypes.GameInfoResponse:

                //    break;
                case MessageTypes.GameHeartbeat: {
                        MasterServer.EventLog.LogEntry(2, "GameHeartbeat received");
                        Heartbeat theRequest = new Heartbeat(ipRemoteAddress, sMessage, barrMessage);
                        oReturn = theRequest;
                    }
                    break;
                case MessageTypes.MasterServerInfoRequest: {
                        MasterServer.EventLog.LogEntry(2, "MasterServerInfoRequest received");
                        InfoRequest theRequest = new InfoRequest(ipRemoteAddress, sMessage, barrMessage);
                        oReturn = theRequest;
                    }
                    break;
                //case MessageTypes.MasterServerInfoResponse:

                //    break;
                default:
                    //Unknown packet
                    MasterServer.EventLog.LogEntry(1, string.Format("!!! Unknown packet type: {0}", iPacketType.ToString()));
                    break;
            }

            return (UDPMessage)oReturn;
        }

    }

    /// <summary>
    /// The message id's of the packets that are processed.
    /// </summary>
    public enum MessageTypes {
        MasterServerGameTypesRequest	=2,
        MasterServerGameTypesResponse	=4,
        MasterServerListRequest			=6,
        MasterServerListResponse		=8,
        GameMasterInfoRequest			=10,
        GameMasterInfoResponse			=12,
        GamePingRequest					=14,
        GamePingResponse				=16,
        GameInfoRequest					=18,
        GameInfoResponse				=20,
        GameHeartbeat					=22,
        MasterServerInfoRequest         =24,
        MasterServerInfoResponse        =26
    }
}
