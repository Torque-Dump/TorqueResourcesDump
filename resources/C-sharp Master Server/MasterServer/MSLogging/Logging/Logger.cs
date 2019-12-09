using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MSLib.Logging {
    public class Logger {

        public delegate void NewLogEntryHandler(string sMessage);

        public event NewLogEntryHandler NewLogEntry;

        public Logger() {
            this.Verbosity = 3;
        }

        public Logger(int iLoggingLevel) {
            this.Verbosity = iLoggingLevel;
        }

        public int Verbosity {
            get;
            set;
        }

        public void LogEntry(int iVerbosity, string sMessage) {
            if (iVerbosity <= this.Verbosity) {
                //First send it to the console
                Console.WriteLine(sMessage);

                //Now notify anyone listening in for log events
                this.FireNewLogEntryEvent(sMessage);
            }
        }

        private void FireNewLogEntryEvent(string sMessage) {
            if (this.NewLogEntry != null) {
                this.NewLogEntry(sMessage);
            }
        }
    }
}
