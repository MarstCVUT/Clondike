require 'TargetMatcher'

# This load balancing strategy tried to utilize best the CPUs of all nodes
# It preffers to keep all remote nodes busy and keeps process local only
# if remote nodes are fully loaded
class CpuLoadBalancingStrategy
   
    def initialize(nodeRepository, membershipManager)
        @nodeRepository = nodeRepository
        @membershipManager = membershipManager
    end
    
    # Return ID of the node where the process shall be migrated
    # Returns nil, if no migration should be performed        
    def findMigrationTarget(pid, uid, name, args, envp, emigPreferred)        
        detachedNodes = @membershipManager.coreManager.detachedNodes
        
        # High cpu usage on local node => Emigrate if there is any other node
        selfCpuUsage = @nodeRepository.selfNode.nodeInfo ? @nodeRepository.selfNode.nodeInfo.cpuUsage : 0
        enforceEmigration = selfCpuUsage > 97
        preferLocal = selfCpuUsage < 95 && !emigPreferred
                
        emigrateThreshold = preferLocal ? 80 : 95
        emigrateThreshold = 101 if enforceEmigration # Any node is elibile target in this case
        
        bestTarget = findBestTarget(pid, uid, name, detachedNodes, emigrateThreshold)
        #puts "Best target #{bestTarget}. Self cpu load: #{selfCpuUsage}"
        bestTarget
    end    
    
    # No rebalancing
    def findRebalancing()
      return nil
    end
private 
    def findBestTarget(pid, uid, name, detachedNodes, emigrateThreshold)
        TargetMatcher.performMatch(pid, uid, name, detachedNodes) { |node|
            node.nodeInfo.cpuUsage
        }        
    end
end