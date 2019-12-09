using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Windows.Forms;

namespace MSLib {

    /// <summary>
    /// This class encapsulates the master servers functioning parts and is used
    /// to handle processing of inbound and outbound messages.
    /// </summary>
    public class MasterServer {
        #region Fields
        //Timers will be used to handle message processing
        System.Timers.Timer tmrInboundQue = new System.Timers.Timer();
        System.Timers.Timer tmrOutboundQue = new System.Timers.Timer();
        System.Timers.Timer tmrProcessExpirations = new System.Timers.Timer();

        //Inbound and outbound message queues
        internal static Queue qInboundMessages = new Queue();
        internal static Queue qOutboundMessages = new Queue();

        //Static members
        public static Preferences ServerPreferences = new Preferences();
        public static ServerStore Server_Store = new ServerStore();
        public static SessionHandler Sessions = new SessionHandler();
        public static SpamHandler Spammers = new SpamHandler();
        public static Logging.Logger EventLog = new MSLib.Logging.Logger();
        
        static IPEndPoint _ServerAddress;
        static UdpClient _UdpClient;
        static UDPState _UdpState;

        public static bool messageReceived = false;
        public static bool receiveMessages = true;
        #endregion

        #region CTOR
        /// <summary>
        /// Constructor
        /// </summary>
        public MasterServer() {
             //Set up the inbound processing
            tmrInboundQue.AutoReset = true;
            tmrInboundQue.Enabled = false;
            tmrInboundQue.Interval = 50;
            tmrInboundQue.Elapsed += new System.Timers.ElapsedEventHandler(tmrInboundQue_Elapsed);

            //Set up the outbound processing
            tmrOutboundQue.AutoReset = true;
            tmrOutboundQue.Enabled = false;
            tmrOutboundQue.Interval = 50;
            tmrOutboundQue.Elapsed += new System.Timers.ElapsedEventHandler(tmrOutboundQue_Elapsed);

            //Set up the session processing
            tmrProcessExpirations.AutoReset = true;
            tmrProcessExpirations.Enabled = false;
            tmrProcessExpirations.Interval = 1000;
            tmrProcessExpirations.Elapsed += new System.Timers.ElapsedEventHandler(tmrProcessExpirations_Elapsed);


        }
        #endregion

        #region Methods

        public void StartMasterServer() {
            EventLog.LogEntry(1, " - Welcome to the C# Master Server");
            EventLog.LogEntry(1, " - Initializing preference file.");
            this.LoadPreferences();

            EventLog.Verbosity = ServerPreferences.Verbosity;

            EventLog.LogEntry(1, " - Initializing networking.");
            if (_UdpClient == null) {
                _ServerAddress = new IPEndPoint(IPAddress.Any, MasterServer.ServerPreferences.Server_Port);
                _UdpClient = new UdpClient(_ServerAddress);
                _UdpState = new UDPState();
                _UdpState.EndPoint = _ServerAddress;
                _UdpState.UDPClient = _UdpClient;
            }
            
            //Start the timers
            tmrInboundQue.Start();
            tmrOutboundQue.Start();
            tmrProcessExpirations.Start();

            this.ReceiveMessages();
        }

        public void StopMasterServer() {
            //Just stop taking anymore messages but let the Queues finish
            MasterServer.EventLog.LogEntry(1, "stopping listening for messages");
            receiveMessages = false;

            tmrInboundQue.Stop();
        }

        /// <summary>
        /// Attempts to load a preference file for basic settings.
        /// If the file is not found one will be created with defaults.
        /// </summary>
        private void LoadPreferences() {
            if (File.Exists("Server.prf")) {
                MasterServer.EventLog.LogEntry(1, " - Loading preferences.");
                MasterServer.ServerPreferences.LoadFromXML("Server.prf");
                MasterServer.EventLog.LogEntry(1, " - Finished Loading preferences.");
            }
            else {
                MasterServer.EventLog.LogEntry(1, " - Unable to load preference file. Loading defaults.");
                MasterServer.EventLog.LogEntry(1, " - Attempting to write new preference file with defaults.");
                MasterServer.ServerPreferences.SaveToXML("Server.prf");
                MasterServer.EventLog.LogEntry(1, " - Created new preference file.");
            }
        }

        /// <summary>
        /// This begins the message receiving process.
        /// </summary>
        private void ReceiveMessages() {
            // Receive a message and write it to the console.
            MasterServer.EventLog.LogEntry(1, "listening for messages");
            _UdpClient.BeginReceive(new AsyncCallback(ReceiveCallback), _UdpState);

            // Do some work while we wait for a message. For this example,
            // we'll just sleep
            while (!messageReceived) {
                if (!receiveMessages)
                    break;

                Thread.Sleep(50);
                Application.DoEvents();
            }
        }


        /// <summary>
        /// Processes inbound messages from the queue and populates the out bound queue
        /// with any responses.
        /// </summary>
        /// <param name="theMessage"></param>
        public void ProcessInboundUDPMessage(UDPMessage theMessage) {
            MasterServer.EventLog.LogEntry(3, string.Format("Processing inbound {0}",Enum.GetName(typeof(Messages.MessageTypes), theMessage.PacketType)));

            List<UDPMessage> theResponses = theMessage.ProcessRequest();
            //Don't lock the queue if we don't need to
            if (theResponses.Count > 0) {
                lock (MasterServer.qOutboundMessages.SyncRoot) {
                    foreach (UDPMessage theResponse in theResponses) {
                        MasterServer.qOutboundMessages.Enqueue(theResponse);
                    }
                }
            }
        }


        /// <summary>
        /// Processes outbound messages from the queue and sends them to their destinations
        /// </summary>
        /// <param name="theMessage"></param>
        public void ProcessOutboundUDPMessage(UDPMessage theMessage) {
            MasterServer.EventLog.LogEntry(3, "Sending outbound message");
            MasterServer._UdpClient.Send(theMessage.RawMessage, theMessage.RawMessage.Length, theMessage.RemoteAddress);
        }
        #endregion

        #region Events
        /// <summary>
        /// Inbound Queue elapsed timer event. As messages are received they are placed into the
        /// Queue for processing. This pulls the messages from the queue and sends them for processing.
        /// This allows for messages to processed in time based batches if needed.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void tmrInboundQue_Elapsed(object sender, System.Timers.ElapsedEventArgs e) {
            //Stop the timer to process the current queue
            tmrInboundQue.Stop();

            if (MasterServer.qInboundMessages.SyncRoot != null) {
                lock (MasterServer.qInboundMessages.SyncRoot) {
                    Queue sqInboundMessages = (Queue)MasterServer.qInboundMessages;
                    while (sqInboundMessages.Count > 0) {
                        //Get the message
                        UDPMessage theMessage = (UDPMessage)sqInboundMessages.Dequeue();

                        //Process the message
                        this.ProcessInboundUDPMessage(theMessage);
                    }
                }
            }

            //Restart the timer
            tmrInboundQue.Start();
        }
        
        /// <summary>
        /// Outbound Queue elapsed timer event. As messages are received they are placed into the
        /// Queue for processing. This pulls the messages from the queue and sends them for processing.
        /// This allows for messages to processed in time based batches if needed.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void tmrOutboundQue_Elapsed(object sender, System.Timers.ElapsedEventArgs e) {
            //Stop the timer to process the current queue
            tmrOutboundQue.Stop();

            if (MasterServer.qOutboundMessages.SyncRoot != null) {
                lock (MasterServer.qOutboundMessages.SyncRoot) {
                    System.Collections.Queue sqOutboundMessages = (System.Collections.Queue)MasterServer.qOutboundMessages;
                    while (sqOutboundMessages.Count > 0) {
                        //Get the message
                        UDPMessage theMessage = (UDPMessage)sqOutboundMessages.Dequeue();

                        //Process the message
                        this.ProcessOutboundUDPMessage(theMessage);
                    }
                }
            }

            //Restart the timer
            tmrOutboundQue.Start();
        }
        
        /// <summary>
        /// This handles UDP messages that are received. It first determines if they are 
        /// spammers. If the message is not a spammer then it is added to the inbound queue
        /// and then listens for a new message.
        /// </summary>
        /// <param name="ar"></param>
        public static void ReceiveCallback(IAsyncResult ar) {
            UdpClient u = (UdpClient)((UDPState)(ar.AsyncState)).UDPClient;
            IPEndPoint e = (IPEndPoint)((UDPState)(ar.AsyncState)).EndPoint;

            Byte[] receiveBytes = u.EndReceive(ar, ref e);

            string receiveString = Encoding.UTF8.GetString(receiveBytes);

            MasterServer.EventLog.LogEntry(3, "Recieved message");

            //Handle spammers first
            bool bAllowPacket = true;
            int iID = Spammers.GetIDFromAddress(e);

            if (iID == -1) {
                //Keep the list from growing to large
                if (Spammers.SpamList.Count > 5000) {
                    MasterServer.EventLog.LogEntry(2, " - Out of Spam Handles! Flushing Cache...");
                    Spammers.FlushRam();
                }

                Spammer theSpammer = new Spammer();
                theSpammer.RemoteAddress = e;
                theSpammer.SpamCount = 1;
                theSpammer.BanTime = 0;

                Spammers.SpamList.Add(theSpammer);
            }
            else {
                Spammer theSpammer = Spammers.SpamList[iID];
                theSpammer.SpamCount -= (float)Spammers.SpamList[iID].DeltaTime / 1000 * MasterServer.ServerPreferences.Variable_Seconds;
                if (theSpammer.SpamCount > MasterServer.ServerPreferences.Variable_Count) {
                    theSpammer.SpamCount = 0;
                    theSpammer.BanTime = (float)MasterServer.ServerPreferences.Ban_Time;
                    bAllowPacket = false;
                }
                else if (theSpammer.BanTime != 0) {
                    theSpammer.SpamCount++;
                    if (theSpammer.SpamCount > MasterServer.ServerPreferences.Variable_Count) {
                        theSpammer.SpamCount = 0;
                        theSpammer.BanTime = (float)MasterServer.ServerPreferences.Ban_Time;
                        bAllowPacket = false;
                    }
                    else bAllowPacket = true;
                }
                else {
                    theSpammer.BanTime -= (float)theSpammer.DeltaTime / 1000;
                    bAllowPacket = false;
                    if (theSpammer.BanTime <= 0) {
                        theSpammer.BanTime = 0;
                        bAllowPacket = true;
                    }
                }
                if (theSpammer.SpamCount <= 0 && theSpammer.BanTime == 0) {
                    //free RAM for more spammers!
                    theSpammer.RemoteAddress = null;
                    theSpammer.SpamCount = 0;
                    theSpammer.BanTime = 0;
                    theSpammer.DeltaTime = 0;
                    theSpammer.Zeit = 0;
                }
            }

            if (bAllowPacket) {
                UDPMessage theMessage = Messages.MessageFactory.DetermineMessage(e, receiveString, receiveBytes);

                if (theMessage != null) {
                    lock (MasterServer.qInboundMessages.SyncRoot) {
                        MasterServer.qInboundMessages.Enqueue(theMessage);
                    }
                }
            }

            messageReceived = true;
            if (receiveMessages) {
                _UdpClient.BeginReceive(new AsyncCallback(ReceiveCallback), _UdpState);
            }

            while (true) {
                if (!receiveMessages)
                    break;

                Thread.Sleep(50);
                Application.DoEvents();
            }
        }
        
        /// <summary>
        /// Timer elapesed event that attempts to clean up old sessions and servers that have expired their heartbeat time
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void tmrProcessExpirations_Elapsed(object sender, System.Timers.ElapsedEventArgs e) {
            //Expire old sessions
            MasterServer.Sessions.DoProcessing();
            MasterServer.Server_Store.ProcessExpiredServers();
        }

        #endregion
    }
}
