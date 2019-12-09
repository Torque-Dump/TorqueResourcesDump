using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MSLib.Logging {
    /// <summary>
    /// This class is used to log messages from the master server.
    /// </summary>
    public class Logger
    {
        #region Event and Handler delegates
        public delegate void NewLogEntryHandler(string sMessage);

        public event NewLogEntryHandler NewLogEntry;
        #endregion

        #region CTOR
        public Logger() {
            this.Verbosity = 3;
        }
        

        public Logger(int iLoggingLevel) {
            this.Verbosity = iLoggingLevel;
        }
        #endregion

        #region Properties
        /// <summary>
        /// Gets and sets the logging reporting level
        /// </summary>
        public int Verbosity {
            get;
            set;
        }
        #endregion

        #region Methods
        /// <summary>
        /// Adds a log entry the logging system
        /// </summary>
        /// <param name="iVerbosity">The logging level of the message</param>
        /// <param name="sMessage">The message to be logged</param>
        public void LogEntry(int iVerbosity, string sMessage) {
            if (iVerbosity <= this.Verbosity) {
                //First send it to the console
                Console.WriteLine(sMessage);

                //Now notify anyone listening in for log events
                this.FireNewLogEntryEvent(sMessage);
            }
        }

        /// <summary>
        /// Fires the NewLogEntry for anything that is listening
        /// </summary>
        /// <param name="sMessage"></param>
        private void FireNewLogEntryEvent(string sMessage) {
            if (this.NewLogEntry != null) {
                this.NewLogEntry(sMessage);
            }
        }
        #endregion
    }
}
