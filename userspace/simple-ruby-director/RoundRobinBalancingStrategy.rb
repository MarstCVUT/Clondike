class RoundRobinBalancingStrategy

    def initialize(nodeRepository, membershipManager, includeLocal = false)
        @nodeRepository = nodeRepository
        @membershipManager = membershipManager
	
	@includeLocal = includeLocal
    end
    
    # Return ID of the node where the process shall be migrated
    # Returns nil, if no migration should be performed        
    def findMigrationTarget(pid, uid, name, args, envp, emigPreferred)        
        return nil if !@membershipManager.coreManager # Not a core node?		

	# First ensure we have initialized list of current detached node candidates.. this list is iterativelly popped from
	# until it is empty and then a new iteration starts (this iterative approach is due to possible changes in detachedNodes set)
	if ( @detachedNodesCandidates == nil || @detachedNodesCandidates.empty? ) then
	  @detachedNodesCandidates = extractIndexes(@membershipManager.coreManager.detachedNodes)
	end

	return nil if @detachedNodesCandidates.empty?
	
        bestTarget = findBestTarget(pid, uid, name, args, envp, emigPreferred)
	
        $log.debug("Task #{pid} best target is #{bestTarget}")
        bestTarget
    end    
    
    def newTask(task)
        # Nothing to be done
    end

    def taskFork(task, parentTask)
        # Nothing to be done
    end

    # Callback from task repository
    def taskExit(task, exitCode)            
        # Nothing to be done
    end    
private
    def extractIndexes(detachedNodes)
	result = []
	# One of the balancing nodes could be local node
	result << nil if @includeLocal
	
      	0.upto(detachedNodes.length) { |i| 
	    result << i if detachedNodes[i]
	}		
	return result
    end

    def findBestTarget(pid, uid, name, args, envp, emigPreferred)
        return @detachedNodesCandidates.pop
    end        
end