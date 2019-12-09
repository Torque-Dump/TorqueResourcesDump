using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Net;
using System.Net.Sockets;

namespace MSLib {
    public class UDPMessage {

        #region Fields
        BinaryReader _br;
        BinaryWriter _bw;
        MemoryStream _memStream = new MemoryStream();
        string _sMessage = "";
        Byte[] _barrRawMessage;

        #endregion

        #region CTOR
        public UDPMessage(int size) {
            this.ReadOnly = false;
            Byte[] barrMessage = new Byte[size];
            this.RawMessage = barrMessage;

            this.InitializeWriter();
        }
        public UDPMessage(IPEndPoint ipRemoteAddress, string sMessage, Byte[] barrMessage) {
            this.RemoteAddress = ipRemoteAddress;
            this.ReadOnly = true;
            this.Message = sMessage;            
            this.RawMessage = barrMessage;
            //this.InitializeReader();
            this.HasPacketIndex = false;
        }
        #endregion

        #region Properties
        /// <summary>
        /// The address the message came from or is to be sent to
        /// </summary>
        public IPEndPoint RemoteAddress {
            get;
            set;
        }

        /// <summary>
        /// String representation of the message from the raw data
        /// </summary>
        public string Message {
            get {
                string sReturn = "";
                if (this.ReadOnly) {
                    sReturn = this._sMessage;
                }
                else {
                    sReturn = UDPMessage.GetStringFromMemoryStream(this._memStream);
                }

                return sReturn;
            }
            private set {
                if (this.ReadOnly) {
                    this._sMessage = value;
                }
            }
        }

        /// <summary>
        /// The raw byte array of the message
        /// </summary>
        public Byte[] RawMessage {
            get {
                Byte[] barrReturn = new Byte[0];
                if (this.ReadOnly) {
                    barrReturn = this._barrRawMessage;
                }
                else {
                    barrReturn = this._memStream.ToArray();
                }
                return barrReturn;
            }
            private set {
                if (this.ReadOnly) {
                    this._barrRawMessage = value;
                }
            }
        }

        /// <summary>
        /// Is the package read only? If the message is created with
        /// the UDPMessage(int size) constructor then this will be false
        /// as it is an outbound message. The other constructor will set this
        /// value to true preventing any writing to.
        /// </summary>
        protected bool ReadOnly { get; private set; }

        /// <summary>
        /// This only a few messages have a packet index so this is used to determine
        /// if it should be parsed. The default value is false.
        /// </summary>
        public bool HasPacketIndex { get; set; }
        
        /// <summary>
        /// The packet type of this message. Used to determine how
        /// the message will be processed.
        /// </summary>
        public ushort PacketType { get; set; }

        /// <summary>
        /// Any flags that are needed in the processing of this message
        /// </summary>
        public ushort Flags { get; set; }

        /// <summary>
        /// The session id for this message
        /// </summary>
        public ushort Session { get; set; }

        /// <summary>
        /// The key id of this message
        /// </summary>
        public ushort Key { get; set; }

        /// <summary>
        /// Packet index of this message
        /// </summary>
        public ushort PacketIndex { get; set; }
        #endregion


        #region Methods
        /// <summary>
        /// This must be overridden in any inheriting classes with a 
        /// call to base.InitializeReader(); as the first line. This
        /// method will parse the standard header info from the message
        /// and populate the base properties.
        /// </summary>
        protected virtual void InitializeReader() {
            Stream s = new MemoryStream(this.RawMessage);
            this._br = new BinaryReader(s);

            //Find the packet type
            this.PacketType = this.readU8();
            MasterServer.EventLog.LogEntry(3, string.Format("UDPMessage PacketType: {0}", this.PacketType));

            //Flags
            this.Flags = this.readU8();
            MasterServer.EventLog.LogEntry(3, string.Format("UDPMessage Flags: {0}", this.Flags));

            //Session
            this.Session = this.readU16();
            MasterServer.EventLog.LogEntry(3, string.Format("UDPMessage Session: {0}", this.Session));

            //Key
            this.Key = this.readU16();
            MasterServer.EventLog.LogEntry(3, string.Format("UDPMessage Key: {0}", this.Key));

            //Packet Index
            if (this.HasPacketIndex) {
                try {
                    this.PacketIndex = this.readU8();
                    MasterServer.EventLog.LogEntry(3, string.Format("UDPMessage PacketIndex: {0}", this.PacketIndex));
                }
                catch {
                    //Not everything has a packet index
                    this.PacketIndex = 0;
                }
            }
        }

        /// <summary>
        /// This method initializes the binary writer for creating an outbound
        /// message.
        /// </summary>
        private void InitializeWriter() {
            this._bw = new BinaryWriter(this._memStream);
        }

        /// <summary>
        /// Must be overriden in inheriting classes to interface with the
        /// message processing system
        /// </summary>
        /// <returns></returns>
        public virtual List<UDPMessage> ProcessRequest() {
            //Return an empty list
            return new List<UDPMessage>();
        }

        // - Read/write byte values
        public void writeU8(byte b) {
            if (this.ReadOnly) {
                MasterServer.EventLog.LogEntry(1, "Attempted to write to a readonly packet!");
                return;
            }
            this._bw.Write(b);
        }
        public byte readU8() {
            if (!this.ReadOnly) {
                MasterServer.EventLog.LogEntry(1, "Attempted to read from a writeonly packet!\n");
                return 0x0;
            }
            return this._br.ReadByte();
        }

        // - Read/write ushort values
        public void writeU16(ushort b) {
            this._bw.Write(b);
        }
        public ushort readU16() {
            return this._br.ReadUInt16();
        }

        //	- read/write nullterm'ed strings
        public string readNullString() {
            StringBuilder sbReturn = new StringBuilder();
            char f = this._br.ReadChar();
            while (f != '\0') {
                sbReturn.Append(f);
                f = this._br.ReadChar();
            }
            //TODO: Not sure if the null termination is needed
            //Add the null terminator
            sbReturn.Append(Convert.ToChar(0x0));

            return sbReturn.ToString();
        }
        public void writeNullString(string s) {
            if (!s.Contains(Convert.ToChar(0x0).ToString())) {
                s += Convert.ToChar(0x0).ToString();
            }
            this._bw.Write(s);
        }

		//	- read/write length indicated strings
        public string readCString() {
            return this._br.ReadString();
        }
        public void writeCString(string s) {
            if (s.Length > 0xFF) {
                MasterServer.EventLog.LogEntry(1, "Attempted to write a string longer than 255 to a packet, truncating...");
                s = s.Substring(0, 254);
            }

            this._bw.Write(s);
        }

		//	- read/write s32's
        public int readS32() {
            return this._br.ReadInt32();
        }
        public void writeS32(int dat) {
            this._bw.Write(dat);
        }

        //	- read/write U32's
        public uint readU32() {
            return this._br.ReadUInt32();
        }
        public void writeU32(uint dat) {
            this._bw.Write(dat);
        }

        /// <summary>
        /// Used when creating outbound messages. Populates the header values
        /// for the message.
        /// </summary>
        /// <param name="type">The message type</param>
        /// <param name="flags">Processing flags</param>
        /// <param name="session">Session Id</param>
        /// <param name="key">Key value</param>
        public void stuffHeader(ushort type, ushort flags, ushort session, ushort key) {
            this.PacketType = type;
            this.Flags = flags;
            this.Session = session;
            this.Key = key;

            writeU8((byte)type); // Packet type
            writeU8((byte)flags); // Flags
            writeU16(session); // session
            writeU16(key); // key
        }

        /// <summary>
        /// Used to return a string from a memory stream
        /// </summary>
        /// <param name="m"></param>
        /// <returns></returns>
        public static string GetStringFromMemoryStream(MemoryStream m) {
            if (m == null || m.Length == 0)
                return null;

            m.Flush();
            m.Position = 0;
            StreamReader sr = new StreamReader(m);
            string s = sr.ReadToEnd();

            return s;
        }

        #endregion
    }
}
