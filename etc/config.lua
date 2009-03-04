-- announce this configuration
print "Loading node configuration for dtn://node1")

-- get the configuration object
local conf = Configuration()

-- set the local node URI
conf:setLocalURI( "dtn://node1" )

-- add local interfaces
net_emma = conf:addInterface( "emma", 4556, "127.0.0.1", "127.0.0.255" )
net_tcp = conf:addInterface( "tcp", 4556, "127.0.0.1" )

-- gps position (fixed or last known position)
conf:setPosition(52.261193, 10.520240)

-- gpsd connection
conf:setGPSDConnection( "127.0.0.1", 2947 )

-- static routing information
conf:addRoute( "dtn://*.ibr/recorder", "dtn://node2" )

-- static connections
conf:addConnection( net_tcp, "127.0.0.1", 4557, "dtn://node2" )

-- define a storage
conf:setStorage( "sqlite", "database.db" )
-- conf:addStorage( "memory" )
