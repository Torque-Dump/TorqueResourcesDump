using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using MSLib;

namespace MasterServerConsole {
    class Program {
        static void Main(string[] args) {
            MasterServer theMS = new MasterServer();
            theMS.StartMasterServer();

            Console.ReadLine();
        }
    }
}
