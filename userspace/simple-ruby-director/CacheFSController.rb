# Class controlling mounting and unmounting of ccfs
# TODO: The controller should keep track of LRU set of mounted peers and eventually unmount some long time not used filesystems
class CacheFSController
  def initialize(interconnection)
    # Set of IP addresses of mounted cache file systems
    @mountedSystems = Set.new()
    interconnection.addReceiveHandler(ResetCacheFsMessage, ResetCacheFsHandler.new(self)) if ( interconnection )
  end

  # Resets state of mounting.. this effectivelly means a ccfs mounting will be performed anew on next immigration request (and all ccfs cache will be flushed)
  def reset()
    $log.debug("CacheFS being resetted.")
    @mountedSystems = Set.new()
  end
  
  # Called whenever a process is being immigrated
  def onImmigrationRequest(node, execName)
    mountCacheFSIfNotMounted(node.ipAddress)
    # It is not responsibility of this class to deny any immigration requests
    return true
  end
  
private
  def mountCacheFSIfNotMounted(ipAddress)
    return if @mountedSystems.include?(ipAddress)
    
    @mountedSystems.add(ipAddress)    
    delayedAsyncMount(ipAddress)
  end

  # The mount is delayed so that we wait till kernel mounts underlaying 9p/nfs filesystems
  # Hence it must be async, in order to prevent blocking of netlink handling thread
  def delayedAsyncMount(ipAddress)
    ExceptionAwareThread.new() {
      sleep 5
      mountCacheFS(ipAddress)
    }
  end
  
  def mountCacheFS(ipAddress)
    $log.debug ("Mounting ccfs for ip #{ipAddress}")
    pathsToMount = ["/usr/local", "/usr/lib", "/usr/lib32", "/usr/share", "/usr/bin", "/usr/sbin", "/lib", "/home/clondike/", "/etc/", "/bin", "/sbin"]
    prefix = "/mnt/clondike/#{ipAddress}-0-0"
    
    pathsToMount.each { |path|
      # first we try to unmout cached fs at the same location in order to prevent multiple overlapping mounts
      system("umount #{prefix + path}")
      system("mount -t ccfs #{prefix + path} #{prefix + path}")
    }    
      
    # TODO: This is hardcoded for current measurement, but should be generalized. It tries to mount a specified path 
    # so that only files older than current time are cached, the others remain uncached. Useful for kernel compilation testing
    restrictedMount = ["/usr/src/linux-2.6.33.1", "/mnt/ext/linux-2.6.32.5"]  
    restrictedMount.each { |path|
      system("umount #{prefix + path}")
      system("mount -t ccfs -o cache_filter=OLD #{prefix + path} #{prefix + path}")      
    }
  end    
end

###################### CLI RELATED SECTION ###############################

class ResetCcfsCliHandler
    def initialize(cacheFsController, interconnection)      
      @cacheFsController = cacheFsController
      @interconnection = interconnection
    end
    
    def handle(command)
	 @interconnection.dispatch(nil, ResetCacheFsMessage.new()) if @interconnection != nil
	 @cacheFsController.reset()
      
        return "Reset performed"
    end
end

def prepareResetCcfsCliHandlerParser()
    ccfsParser = CommandParser.new("resetCcfs")
    return ccfsParser
end

def registerAllCcfsParsers(parser)    
    parser.addCommandParser(prepareResetCcfsCliHandlerParser())
end

def registerAllCcfsHandlers(cacheFsController, interconnection, interpreter)
    interpreter.addHandler("resetCcfs", ResetCcfsCliHandler.new(cacheFsController, interconnection))
end

###################### Messages related section###############################

# Message used for distribution of execution plans
class ResetCacheFsMessage
end

class ResetCacheFsHandler
    def initialize(cacheFsController)
      @cacheFsController = cacheFsController
    end
    
    def handle(message)        
        @cacheFsController.reset
    end  
end

