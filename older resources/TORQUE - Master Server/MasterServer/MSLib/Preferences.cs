using System;
using System.IO;
using System.Xml.Serialization;

namespace MSLib {
    /// <summary>
    /// This class is used to load and save the server preferences
    /// </summary>
    [Serializable()]
    public class Preferences
    {

        #region CTOR
        public Preferences() {
            this.Name = "Master Server";
            this.Region = "2";
            this.Server_Port = 28002;
            this.Heatbeat_To = 180;
            this.Flush_Time = 1800;
            this.Variable_Seconds = 1;
            this.Variable_Count = 5;
            this.Ban_Time = 30;
            this.Verbosity = 3;
        }
        #endregion

        #region Properties
        /// <summary>
        /// This specifies the name of the Master Server.
        /// </summary>
        public string Name { get; set; }
        /// <summary>
        /// This specifies the region the Master Server is in. Default: Earth
        /// </summary>
        public string Region { get; set; }
        /// <summary>
        /// This specifies the port that the Daemon listens and sends on. Default: 28002
        /// </summary>
        public int Server_Port { get; set; }
        /// <summary>
        /// This specifies how long since the last heartbeat from a server before it is deleted. Default: 180 (3min)
        /// </summary>
        public int Heatbeat_To { get; set; }
        /// <summary>
        /// This specifies the time between RAM flushes, and keeps the flood control cache clear. Default: 1800 (30min)
        /// </summary>
        public int Flush_Time { get; set; }
        /// <summary>
        /// This coincides with the 'spamcount' variable. Basically, if the user exceeds spamcount packets in spamtime seconds, he is banned for bantime. Default: 1
        /// </summary>
        public int Variable_Seconds { get; set; }
        /// <summary>
        /// This coincides with the 'spamtime' variable. Basically, if the user exceeds spamcount packets in spamtime seconds, he is banned for bantime. Default: 5
        /// </summary>
        public int Variable_Count { get; set; }
        /// <summary>
        /// The length of time in seconds that a user is banned if he is cought flooding the Daemon. Default: 30
        /// </summary>
        public int Ban_Time { get; set; }
        /// <summary>
        /// Verbosity. Default: 3
        /// 0 - No Messages
        /// 1 - Critical Messages
        /// 2 - Most Messages
        /// 3 - All Messages
        /// </summary>
        public int Verbosity { get; set; }

        #endregion

        #region Methods
        public void SaveToXML(string sFilePath) {
            //serialize and persist it to it's file
            FileStream fs = null;
            try {
                XmlSerializer ser = new XmlSerializer(this.GetType());
                fs = File.Open(sFilePath, FileMode.OpenOrCreate, FileAccess.Write, FileShare.ReadWrite);
                ser.Serialize(fs, this);

            }
            catch (Exception ex) {
                throw new Exception("Could Not Serialize object to " + sFilePath, ex);
            }
            finally {
                fs.Close();
            }
        }

        public void LoadFromXML(string sFilePath) {
            Preferences thePref = new Preferences();
            XmlSerializer ser = new XmlSerializer(this.GetType());
            Stream reader = new FileStream(sFilePath, FileMode.Open);
            try {
                thePref = (Preferences)ser.Deserialize(reader);
            }
            catch { }
            finally {
                reader.Close();
            }
            this.Name = thePref.Name;
            MasterServer.EventLog.LogEntry(1, string.Format(" - Server Name: {0}.",this.Name));
            this.Region = thePref.Region;
            MasterServer.EventLog.LogEntry(1, string.Format(" - Server Region: {0}.", this.Region));
            this.Server_Port = thePref.Server_Port;
            MasterServer.EventLog.LogEntry(1, string.Format(" - Server Port: {0}.", this.Server_Port));
            this.Heatbeat_To = thePref.Heatbeat_To;
            MasterServer.EventLog.LogEntry(1, string.Format(" - Server Heartbeat: {0}.", this.Heatbeat_To));
            this.Flush_Time = thePref.Flush_Time;
            MasterServer.EventLog.LogEntry(1, string.Format(" - Server Flush Time: {0}.", this.Flush_Time));
            this.Variable_Seconds = thePref.Variable_Seconds;
            MasterServer.EventLog.LogEntry(1, string.Format(" - Server Variable Seconds: {0}.", this.Variable_Seconds));
            this.Variable_Count = thePref.Variable_Count;
            MasterServer.EventLog.LogEntry(1, string.Format(" - Server Variable Count: {0}.", this.Variable_Count));
            this.Ban_Time = thePref.Ban_Time;
            MasterServer.EventLog.LogEntry(1, string.Format(" - Server Ban Time: {0}.", this.Ban_Time));
            this.Verbosity = thePref.Verbosity;
            MasterServer.EventLog.LogEntry(1, string.Format(" - Server Verbosity: {0}.", this.Verbosity));

        }
        #endregion
    }

}
