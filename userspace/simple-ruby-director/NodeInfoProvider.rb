require 'NodeInfo'
require 'CpuUsageParser'

#Helper class that wraps node info and the node id (to identify source of the node info)
class NodeInfoWithId
    #Id of the node whose it this is
    attr_reader :nodeId
    #Info about the node
    attr_reader :nodeInfo
    
    def initialize(nodeId, nodeInfo)
        @nodeId = nodeId
        @nodeInfo = nodeInfo                
    end
    
    def to_s
        "Id: #{nodeId} Content: #{nodeInfo}"
    end
end

#This class is extraction NodeInfo about current node
#It has 2 operation modes
# - it pushes this data to the registered listeners
# - it provides data on request
class NodeInfoProvider
    def initialize(idResolver, immigratedTasksController)        
	@immigratedTasksController = immigratedTasksController
	@idResolver = idResolver
        @listeners = Set.new
        @cpuUsageParser = CpuUsageParser.new	
        # Collection of limiters for accepted tasks
        @acceptLimiters = []
    end
        
    def addListener(listener)
        @listeners.add(listener)                
    end
    
    def addLimiter(limiter)
        @acceptLimiters << limiter
    end
    
    def registerLocalTaskCountProvider(localTaskCountProvider)
      @localTaskCountProvider = localTaskCountProvider
    end

    def getCurrentInfoWithId
        NodeInfoWithId.new(getCurrentId, getCurrentInfo)
    end
    
    def getCurrentStaticInfo        
        architecture = `uname -m`.chop
        memory = frequency = nil       
        coresCount = 1
        IO.foreach("/proc/meminfo") { |line| memory = line.split[1] if line =~ /^MemTotal.*/ }
        IO.foreach("/proc/cpuinfo") { |line|
            coresCount = line.split(":")[1].to_i if line =~ /^cpu cores/
            frequency = line.split(":")[1].to_i if line =~ /^cpu MHz/
        }
        StaticNodeInfo.new(architecture, coresCount, frequency, memory)
    end

    def getCurrentId
	@idResolver.getCurrentId
    end
   
    def startNotifyThread
         @thread = Thread.new {             
             changeTracingThread() if !@listeners.empty?
         }
    end

    def waitForNotifyThread
        @thread.join
    end
    
private
     def getCurrentInfo
	 localTaskCount = -1
	 localTaskCount = @localTaskCountProvider.localTaskCount() if @localTaskCountProvider
         NodeInfo.new(getCurrentLoad, getCurrentCpuUsage, getCurrentMaximumAccept, @immigratedTasksController.immigratedTaskCount(), localTaskCount)
     end    

     # Finds minimal maximum-accept count. (nil = unlimited)
     def getCurrentMaximumAccept
         result = nil
         @acceptLimiters.each { |limiter|  
             maximum = limiter.maximumAcceptCount
             if ( maximum && (!result || result > maximum) )
                 result = maximum
             end
         }
         result
     end
     
     def getCurrentLoad
         result = nil
         File.open("/proc/loadavg", "r") { |aFile| 
             # First element of loadavg is current load
             result = aFile.readline.split().first.to_f
         }
         result
     end
     
     def getCurrentCpuUsage
         @cpuUsageParser.updateInfo()
         #(@cpuUsageParser.getSystemUsage + @cpuUsageParser.getUserUsage)
         100 - @cpuUsageParser.getIdle
     end
    
     
     def changeTracingThread
         return if @listeners.empty?

         while true
                 sleep(0.5)
                 newInfo = getCurrentInfo
                 @listeners.each do |listener| 
                     listener.notifyChange(NodeInfoWithId.new(getCurrentId(),newInfo))
                 end 
         end
     end
end