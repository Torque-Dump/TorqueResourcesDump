using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MSLib {
    public class PacketConfig {
        private PacketConfig() { }
        /**
         * @brief Maximum size of a list packet.
         *
         * @todo Tweak if we experience packet fragmentation/truncation.
         */
        public const int LIST_PACKET_SIZE = 1500;

        /**
         * @brief Size of packet header (protocol-dependent)
         */
        public const int LIST_PACKET_HEADER = 12;


        /**
         * @brief Size of a single server element in a packet.
         *
         * 4 bytes/ip, 2 bytes/port
         */
        public const int LIST_PACKET_SERVER_SIZE = 6;

        /**
         * @brief Helper calculations for the max server calculations.
         *
         * This calculates the theoretical max.
         */
        private const int LIST_PACKET_MAX_SERVERS_ = ((int)((LIST_PACKET_SIZE - LIST_PACKET_HEADER) / LIST_PACKET_SERVER_SIZE));


        /**
         * @brief Actual max servers number for list packets.
         *
         * We cap the number of servers/packet at 254 due to U8 constraints. (see sendListResponse)
         *
         * Since the theoretical max could be higher.
         */
        public const int LIST_PACKET_MAX_SERVERS = (LIST_PACKET_MAX_SERVERS_ > 254 ? 254 : LIST_PACKET_MAX_SERVERS_);
    }
}
