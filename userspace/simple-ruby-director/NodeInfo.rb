#Node info represents an information about the node, that is shared in the cluster
#i.e. each Node has its NodeInfo, and this node info is distributed to other 
#interested nodes
class NodeInfo
    #Timestamp of creation of this info (in node local time)
    attr_reader :timestamp
    #Current load of the node
    attr_reader :load
    #Current usage of cpu.. can be in range 0 to 100
    attr_reader :cpuUsage
    # Current maximum accept count of the node
    attr_reader :maximumAccept
    
    # NOTE: The following 2 pieces of information may or may not be broadcasted in real world scenario.. the other nodes can benefit from those
    # on the other hand they cannot be trusted and node may want to keep them local.. we broadcast them in order to track those data in statistics, but it can be removed
    
    # Count of currently immigrated remoted tasks
    attr_reader :immigratedTasks
    # Count of local MIGRATEABLE tasks.. i.e. tasks that would be considered for migration, but were kept locally
    attr_reader :localTasks
    
    def initialize (load, cpuUsage, maximumAccept, immigratedTasks, localTasks)
        @load = load
        @cpuUsage = cpuUsage
        @maximumAccept = maximumAccept
	@immigratedTasks = immigratedTasks
	@localTasks = localTasks
        @timestamp = Time.now()
    end
    
    def to_s
        "Load: #{@load} Cpu usage: #{@cpuUsage} Timestamp: #{@timestamp} Max accept: #{@maximumAccept} Immigrated: #{@immigratedTasks} Local: #{@localTasks}"
    end    
end

#This is again shared part of the information about the node, but this part
#is not changing during the life-time of the node, so it needs to be obtained just once
class StaticNodeInfo
    #String representing architecture of the node
    attr_reader :architecture    
    #Number of computation cores of the node
    attr_reader :coresCount
    #Frequency of the cores
    attr_reader :frequency
    #Total memory on the node
    attr_reader :memory
    
    def initialize(architecture, coresCount, frequency, memory)
        @architecture = architecture
        @coresCount = coresCount
        @frequency = frequency
        @memory = memory
    end
    
    def to_s
      "Architecture: #{@architecture} Cores: #{@coresCount} Frequency: #{@frequency} Memory: #{@memory}"
    end
end