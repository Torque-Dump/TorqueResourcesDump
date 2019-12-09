using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;

namespace MSLib {
    class TorqueIO {


        //extern char name[255];

        //extern char region[255];

        /**
         * @brief Parse a list request packet and reply.
         */
        void handleListRequest(IPEndPoint from, UDPMessage stream) {
            ServerFilter filter;

            /***********************************
             Read in the header
            ***********************************/

            char flags, packet_idx;
            int session, key;

            flags = stream.readU8();
            session = stream.readU16();
            key = stream.readU16();
            packet_idx = stream.readU8();

            Console.WriteLine("Received packet header...\n");
            Console.WriteLine("	- flags   = %x\n", flags);
            Console.WriteLine("	- session = %x\n", session);
            Console.WriteLine("	- key     = %d\n", key);
            Console.WriteLine("	- index   = %x\n", packet_idx);

            if (packet_idx != 0xFF) {
                // It's a packet resend request...

                // Deal with it, then quit...
                Console.WriteLine("Got request for packet resend.\n");
                return;
            }

            Console.WriteLine("Got request for initial query.\n");

            // Otherwise, it's a real honest-to-goodness query packet
            // so let's read it in and get cracking.

            /***********************************
             Read gameType/missionType
            ***********************************/

            filter.gameType = stream.readCString();
            filter.missionType = stream.readCString();

            /***********************************
             Read miscellaneous properties
            ***********************************/

            filter.minPlayers = stream.readU8();
            filter.maxPlayers = stream.readU8();
            filter.regions = stream.readU32();
            filter.version = stream.readU32();
            filter.filterFlags = stream.readU8();
            filter.maxBots = stream.readU8();
            filter.minCPUSpeed = stream.readU16();
            /************************************
             Read in the buddy list
            ************************************/

            filter.playerCount = stream.readU8();


            if (filter.minPlayers < 0) filter.minPlayers = 0;
            if (filter.maxPlayers < 0) filter.maxPlayers = 0;
            if (filter.maxPlayers < filter.minPlayers) filter.maxPlayers = filter.minPlayers;
            if (filter.regions < 0) filter.regions = 0;
            if (filter.version < 0) filter.version = 0;
            if (filter.maxBots < 0) filter.maxBots = 0;
            if (filter.minCPUSpeed < 0) filter.minCPUSpeed = 0;
            if (filter.playerCount < 0) filter.playerCount = 0;
            filter.playerList = new int[filter.playerCount];

            for (int x = 0; x < filter.playerCount; x++)
                filter.playerList[x] = stream.readU32();

            // Ok, what to do?

            // First, get a server result set.
            ServerResults query_results;
            query_results = store.queryServers(&filter);

            Console.WriteLine("Got %d results from a queryServers.\n", query_results.count);

            // Put the query result along with the session into the active-query
            // handler.
            sessions.startSession(from, session, key, query_results);

            // Now send EVERYTHING out! *maniacal laughter*
            ServerResults cur = query_results;
            int i = 0;

            while (cur != null) {
                sendListResponse(from, session, key, query_results, i++);
                cur = cur.next;
            }
        }

        /**
         * @brief Handle a TypesRequest packet.
         */
        void handleTypesRequest(IPEndPoint from, UDPMessage x) {
            // Get the type, session, and key.
            int tmp, session, key;

            tmp = x.readU8(); // flags
            session = x.readU16();
            key = x.readU16();

            // Send the types response.
            sendTypesResponse(from, session, key);
        }

        void handleInfoRequest(IPEndPoint from, UDPMessage x, ServerStoreRAM* store) {
            // Get the type, session, and key.
            int tmp, session, key;

            tmp = x.readU8(); // flags
            session = x.readU16();
            key = x.readU16();

            // Send the types response.
            sendInfoResponse(from, session, key, store);
        }

        /**
         * @brief Handle an InfoResponse packet.
         */
        void handleInfoResponse(IPEndPoint addy, UDPMessage p) {
            ServerInfo info;

            int flags, session, key;

            info.addy = addy;

            flags = p.readU8();
            session = p.readU16();
            key = p.readU16();

            info.gameKey[0] = 'T';
            info.gameKey[1] = 'G';
            info.gameKey[2] = 'E';
            info.gameKey[3] = ' ';
            info.gameType = p.readCString();
            info.missionType = p.readCString();
            info.maxPlayers = p.readU8();
            info.regions = p.readU32();
            info.version = p.readU32();
            info.infoFlags = p.readU8();
            info.numBots = p.readU8();
            info.CPUSpeed = p.readU32();
            info.playerCount = p.readU8();


            if (info.maxPlayers < 0) info.maxPlayers = 0;
            if (info.regions < 0) info.regions = 0;
            if (info.version < 0) info.version = 0;
            if (info.numBots < 0) info.numBots = 0;
            if (info.CPUSpeed < 0) info.CPUSpeed = 0;
            if (info.playerCount < 0) info.playerCount = 0;
            info.playerList = new int[info.playerCount];

            // Read in players
            for (int i = 0; i < info.playerCount; i++) {
                info.playerList[i] = p.readU32();
            }

            // Ok, all done! - store
            store.updateServer(addy, &info);
        }

        /**
         * @brief Deal with heartbeat packets.
         */
        void handleHeartbeat(IPEndPoint who, UDPMessage x) {
            // Store the data
            store.heartbeatServer(who);

            // The response to a heartbeat (in addition) is to request info from the server.
            UDPMessage irp = new UDPMessage(64); // Is 64 a good size?

            short s, k;
            sessions.makeSession(who, &s, &k);

            // Prep and send packet
            irp.stuffHeader(10, 0, s, k);
            transport.sendPacket(irp, who);

            irp = null;
        }

        /**
         * @brief Send a response to a TypesRequest.
         */
        void sendTypesResponse(IPEndPoint where, int session, int key) {
            UDPMessage reply = new UDPMessage(1600);
            reply.stuffHeader(MasterServerGameTypesResponse, 0, session, key);

            // Send some bogus game types
            reply.writeU8(0);

            // And some bogus mission types
            reply.writeU8(0);

            // Send that, too
            transport.sendPacket(reply, where);

            reply = null;
        }

        /**
         * @brief Send a response to a InfoRequest.
         */
        void sendInfoResponse(IPEndPoint where, int session, int key, ServerStoreRAM* store) {
            UDPMessage reply = new UDPMessage(1600);
            reply.stuffHeader(MasterServerInfoResponse, 0, session, key);

            // Send MServer Name
            reply.writeCString(name, 255);

            // Send MServer Region
            reply.writeCString(region, 255);

            // Send Number Servers
            reply.writeU16(store.numServers);

            // Send that, too
            transport.sendPacket(reply, where);

            reply = null;
        }


        /**
         *
         * On the sending of list packets:
         *
         * <ul>
         *	<li> We have 1500 bytes usable for the packet. (XXX: Are 1500b packets really reliable?)
         *	<li>We budget the first 12 bytes for header, leaving us 1488. It takes us 6 bytes/server.
         *	<li> We can send 248 servers/packet, then.
         * </ul>
         *
         * 	We never want to send more than 254 packets
         *  Because 255 == FF == the initial request indicator
         *
         *	So we can never give more than 62992 servers as a result. Note that this is quite
         *	a lot of servers; if we plan on more, then we can just make the packet index a
         *	U16 and issue an upgrade.
         *
         *	We customize all this behaviour with defines.
         */
        void sendListResponse(IPEndPoint where, int session, int key, ServerResults res, int which) {
            UDPMessage list = new UDPMessage(LIST_PACKET_SIZE);

            list.stuffHeader(8, 0, session, key);

            list.writeU8(which);	// packet index

            // Now get the relevant ServerResult

            ServerResults cur = res;
            ServerResults relevant;
            int i = 0, total = 0;

            // Calculate and find.
            while (cur) {
                total++;
                if (i == which) relevant = cur;
                cur = cur.next;
            }

            list.writeU8(total); // total packets to send.
            list.writeU16(relevant.count);  // count of servers in this packet

            // Now loop through the relevant chunk's servers...
            for (i = 0; i < relevant.count; i++) {
                // Write the quads
                list.writeU8(relevant.server[i].addy[0]);
                list.writeU8(relevant.server[i].addy[1]);
                list.writeU8(relevant.server[i].addy[2]);
                list.writeU8(relevant.server[i].addy[3]);

                // Write the port
                list.writeU16(relevant.server[i].port);
            }

            // All done, send.
            transport.sendPacket(list, where);

            delete list;
        }

    }
}
