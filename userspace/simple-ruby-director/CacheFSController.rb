# Class controlling mounting and unmounting of ccfs
# TODO: The controller should keep track of LRU set of mounted peers and eventually unmount some long time not used filesystems
class CacheFSController
  def initialize
    # Set of IP addresses of mounted cache file systems
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
    restrictedMount = ["/usr/src/linux-2.6.33.1", "/usr/src/linux-2.6.32.5"]  
    restrictedMount.each { |path|
      system("umount #{prefix + path}")
      system("mount -t ccfs -o cache_filter=OLD #{prefix + path} #{prefix + path}")      
    }
  end
    
end