-- announce this configuration
print "Loading node configuration for dtn://node1"

-- get the application object
local app = Application()

-- set uid and gid
app:setUser(0)
app:setGroup(0)

-- get the BundleCore object
core = BundleCore()
