/*
 * Copyright (c) 2013-2015 John Connor (BM-NC49AxAjcqVcF5jNPu85Rb8MJ2d9JqZt)
 *
 * This file is part of vanillacoin.
 *
 * vanillacoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <vector>

#include <coin/database_stack.hpp>
#include <coin/logger.hpp>
#include <coin/stack_impl.hpp>
#include <coin/status_manager.hpp>

using namespace coin;

database_stack::database_stack(
    boost::asio::io_service & ios, boost::asio::strand & s,
    stack_impl & owner
    )
    : io_service_(ios)
    , strand_(s)
    , stack_impl_(owner)
    , timer_(ios)
{
    // ...
}

void database_stack::start(const std::uint16_t & port, const bool & is_client)
{
#if (defined USE_DATABASE_STACK && USE_DATABASE_STACK)
    database::stack::configuration stack_config;
    
    /**
     * The bootstrap contacts.
     */
    std::vector< std::pair<std::string, std::uint16_t> > contacts;
    
    /**
     * Add the hard-coded bootstrap contacts.
     */
    contacts.push_back(std::make_pair("127.0.0.1", 40004));
    contacts.push_back(std::make_pair("162.219.176.251", 40004));
    contacts.push_back(std::make_pair("94.23.231.51", 56280));
    contacts.push_back(std::make_pair("pool01.vanillacoin.net", 56280));
    
    /**
     * Set the port.
     */
    stack_config.set_port(port);
    
    /**
     * Set the operation mode.
     */
    stack_config.set_operation_mode(
        is_client ? database::stack::configuration::operation_mode_interface :
        database::stack::configuration::operation_mode_storage
    );
    
    /**
     * Start the database::stack.
     */
    database::stack::start(stack_config);
    
    /**
     * Join the database::stack.
     */
    database::stack::join(contacts);
#endif // USE_DATABASE_STACK

    auto self(shared_from_this());
    
    /**
     * Start the timer.
     */
    timer_.expires_from_now(std::chrono::seconds(8));
    timer_.async_wait(strand_.wrap(
        std::bind(&database_stack::tick, self,
        std::placeholders::_1))
    );
}

void database_stack::stop()
{
    /**
     * Cancel the timer.
     */
    timer_.cancel();

#if (defined USE_DATABASE_STACK && USE_DATABASE_STACK)
    database::stack::stop();
#endif // USE_DATABASE_STACK
}

void database_stack::on_find(
    const std::uint16_t & transaction_id,
    const std::string & query
    )
{
    // ...
}

void database_stack::on_udp_receive(
    const char * addr, const std::uint16_t & port, const char * buf,
    const std::size_t & len
    )
{
    // ...
}

void database_stack::on_broadcast(
    const char * addr, const std::uint16_t & port,
    const char * buf, const std::size_t & len
    )
{
    // ...
}

void database_stack::tick(const boost::system::error_code & ec)
{
    if (ec)
    {
        // ...
    }
    else
    {
        auto self(shared_from_this());
        
        /**
         * Get the number of udp endpoints in the routing table.
         */
#if (defined USE_DATABASE_STACK && USE_DATABASE_STACK)
        auto udp_connections = endpoints().size();
#else
        auto udp_connections = 0;
#endif // USE_DATABASE_STACK
        
        log_info(
            "Database stack has " << udp_connections << " UDP connections."
        );
        
        /**
         * Allocate the status.
         */
        std::map<std::string, std::string> status;
        
        /**
         * Set the status message.
         */
        status["type"] = "network";
        
        /**
         * Set the value.
         */
        status["value"] = udp_connections > 0 ? "Connected" : "Connecting";
        
        /**
         * Set the network.udp.connections.
         */
        status["network.udp.connections"] = std::to_string(
            udp_connections
        );
        
        /**
         * Callback status.
         */
        stack_impl_.get_status_manager()->insert(status);

        /**
         * Start the timer.
         */
        timer_.expires_from_now(std::chrono::seconds(60));
        timer_.async_wait(strand_.wrap(
            std::bind(&database_stack::tick, self,
            std::placeholders::_1))
        );
    }
}
