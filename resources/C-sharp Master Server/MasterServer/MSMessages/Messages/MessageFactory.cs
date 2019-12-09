using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;

namespace MSLib.Messages {
    public class MessageFactory {

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
