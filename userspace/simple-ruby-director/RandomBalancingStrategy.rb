# Strategy that performs random load balancing to any available node
class RandomBalancingStrategy    
    
    def initialize(nodeRepository, membershipManager)
        @nodeRepository = nodeRepository
        @membershipManager = membershipManager
    end
    
    # Return ID of the node where the process shall be migrated
    # Returns nil, if no migration should be performed        
    def findMigrationTarget(pid, uid, name, args, envp, emigPreferred)
        detachedNodes = @membershipManager.coreManager.detachedNodes
        
        # +/- Uniform chance to choose any node, including local node (=no migration)
        # It ignores holes in detached managers array, they mean no migration as well
        target = rand(detachedNodes.size + 1)
        return nil if target == 0
        # Check if the detached managers exists
        return nil if !detachedNodes[target-1]
        target-1        
    end
    
    # No rebalancing
    def findRebalancing()
      return nil
    end    
end